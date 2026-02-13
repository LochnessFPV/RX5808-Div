#include "elrs_backpack.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "rx5808.h"
#include "string.h"

static const char *TAG = "ELRS_BACKPACK";
static QueueHandle_t uart_queue;
static bool backpack_enabled = false;

// CRC8 DVB-S2 calculation (used by CRSF protocol)
uint8_t elrs_crc8_dvb_s2(uint8_t crc, uint8_t a)
{
    crc ^= a;
    for (int i = 0; i < 8; i++) {
        if (crc & 0x80) {
            crc = (crc << 1) ^ 0xD5;
        } else {
            crc = crc << 1;
        }
    }
    return crc;
}

// Calculate CRC for CRSF frame
uint8_t elrs_calculate_crc(uint8_t *data, uint8_t length)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < length; i++) {
        crc = elrs_crc8_dvb_s2(crc, data[i]);
    }
    return crc;
}

// Send CRSF frame via UART
void elrs_send_crsf_frame(crsf_frame_t *frame)
{
    if (!backpack_enabled) return;
    
    uint8_t buffer[ELRS_RX_BUFFER_SIZE];
    uint8_t idx = 0;
    
    buffer[idx++] = CRSF_SYNC_BYTE;
    buffer[idx++] = frame->frame_size;
    buffer[idx++] = frame->type;
    
    memcpy(&buffer[idx], frame->payload, frame->frame_size - 2);
    idx += frame->frame_size - 2;
    
    // Calculate and append CRC
    uint8_t crc = elrs_calculate_crc(&buffer[2], frame->frame_size - 1);
    buffer[idx++] = crc;
    
    uart_write_bytes(ELRS_UART_NUM, buffer, idx);
    
    ESP_LOGI(TAG, "Sent CRSF frame: type=0x%02X, size=%d", frame->type, frame->frame_size);
}

// Convert RX5808 band/channel to frequency
uint16_t elrs_band_channel_to_freq(uint8_t band, uint8_t channel)
{
    if (band >= 6 || channel >= 8) {
        return 5800; // Default fallback
    }
    return Rx5808_Freq[band][channel];
}

// Convert frequency to RX5808 band/channel
void elrs_freq_to_band_channel(uint16_t freq, uint8_t *band, uint8_t *channel)
{
    uint16_t min_diff = 0xFFFF;
    *band = 0;
    *channel = 0;
    
    for (uint8_t b = 0; b < 6; b++) {
        for (uint8_t c = 0; c < 8; c++) {
            uint16_t diff = (freq > Rx5808_Freq[b][c]) ? 
                           (freq - Rx5808_Freq[b][c]) : 
                           (Rx5808_Freq[b][c] - freq);
            if (diff < min_diff) {
                min_diff = diff;
                *band = b;
                *channel = c;
            }
        }
    }
}

// Send VRX configuration response
void elrs_send_vrx_config(vrx_config_t *config)
{
    crsf_frame_t frame;
    frame.device_addr = CRSF_ADDRESS_VTX;
    frame.type = CRSF_FRAMETYPE_MSP_RESP;
    
    // MSP response structure
    uint8_t idx = 0;
    frame.payload[idx++] = CRSF_ADDRESS_CRSF_TRANSMITTER; // Destination
    frame.payload[idx++] = CRSF_ADDRESS_VTX;              // Origin
    frame.payload[idx++] = MSP_VTX_CONFIG;                 // MSP command
    frame.payload[idx++] = 0;                              // Status (OK)
    
    // VRX config payload
    frame.payload[idx++] = config->band;
    frame.payload[idx++] = config->channel;
    frame.payload[idx++] = config->power;
    frame.payload[idx++] = config->pit_mode;
    frame.payload[idx++] = config->freq & 0xFF;           // Freq low byte
    frame.payload[idx++] = (config->freq >> 8) & 0xFF;    // Freq high byte
    
    frame.frame_size = idx + 2; // +2 for type and CRC
    
    elrs_send_crsf_frame(&frame);
}

// Process incoming MSP command
void elrs_process_msp_command(uint8_t *payload, uint8_t length)
{
    if (length < 3) return;
    
    uint8_t dest_addr = payload[0];
    uint8_t origin_addr = payload[1];
    uint8_t msp_cmd = payload[2];
    
    ESP_LOGI(TAG, "MSP Command: 0x%02X from 0x%02X", msp_cmd, origin_addr);
    
    vrx_config_t config;
    
    switch (msp_cmd) {
        case MSP_VTX_CONFIG:
            // Request for current VRX config
            config.band = Chx_count;
            config.channel = channel_count;
            config.power = 0;
            config.pit_mode = 0;
            config.freq = Rx5808_Freq[Chx_count][channel_count];
            elrs_send_vrx_config(&config);
            break;
            
        case MSP_SET_VTX_CONFIG:
            // Set new VRX config
            if (length >= 9) {
                uint8_t new_band = payload[3];
                uint8_t new_channel = payload[4];
                uint16_t new_freq = payload[7] | (payload[8] << 8);
                
                ESP_LOGI(TAG, "Setting VRX: Band=%d, Ch=%d, Freq=%d", 
                        new_band, new_channel, new_freq);
                
                // Update RX5808
                if (new_freq > 0) {
                    // Use frequency directly
                    RX5808_Set_Freq(new_freq);
                    elrs_freq_to_band_channel(new_freq, &new_band, &new_channel);
                    Chx_count = new_band;
                    channel_count = new_channel;
                } else if (new_band < 6 && new_channel < 8) {
                    // Use band/channel
                    Chx_count = new_band;
                    channel_count = new_channel;
                    Rx5808_Set_Channel(new_band * 8 + new_channel);
                }
                
                // Send confirmation
                config.band = Chx_count;
                config.channel = channel_count;
                config.power = 0;
                config.pit_mode = 0;
                config.freq = Rx5808_Freq[Chx_count][channel_count];
                elrs_send_vrx_config(&config);
            }
            break;
            
        default:
            ESP_LOGW(TAG, "Unknown MSP command: 0x%02X", msp_cmd);
            break;
    }
}

// Parse CRSF frame from UART data
void elrs_parse_crsf_frame(uint8_t *data, uint8_t length)
{
    if (length < 4) return; // Minimum frame size
    
    uint8_t idx = 0;
    
    // Look for sync byte
    while (idx < length && data[idx] != CRSF_SYNC_BYTE) {
        idx++;
    }
    
    if (idx >= length - 3) return;
    
    uint8_t frame_size = data[idx + 1];
    uint8_t frame_type = data[idx + 2];
    
    if (idx + frame_size + 2 > length) return; // Incomplete frame
    
    // Verify CRC
    uint8_t calc_crc = elrs_calculate_crc(&data[idx + 2], frame_size - 1);
    uint8_t recv_crc = data[idx + 2 + frame_size - 1];
    
    if (calc_crc != recv_crc) {
        ESP_LOGW(TAG, "CRC mismatch: calc=0x%02X, recv=0x%02X", calc_crc, recv_crc);
        return;
    }
    
    // Process frame based on type
    switch (frame_type) {
        case CRSF_FRAMETYPE_MSP_REQ:
        case CRSF_FRAMETYPE_MSP_WRITE:
            elrs_process_msp_command(&data[idx + 3], frame_size - 2);
            break;
            
        default:
            ESP_LOGD(TAG, "Unhandled frame type: 0x%02X", frame_type);
            break;
    }
}

// UART receive task
void elrs_backpack_task(void *param)
{
    uart_event_t event;
    uint8_t *data = (uint8_t *)malloc(ELRS_RX_BUFFER_SIZE);
    
    ESP_LOGI(TAG, "ExpressLRS Backpack task started");
    backpack_enabled = true;
    
    while (1) {
        if (xQueueReceive(uart_queue, (void *)&event, pdMS_TO_TICKS(100))) {
            switch (event.type) {
                case UART_DATA:
                    if (event.size > 0) {
                        int len = uart_read_bytes(ELRS_UART_NUM, data, 
                                                 event.size, pdMS_TO_TICKS(100));
                        if (len > 0) {
                            elrs_parse_crsf_frame(data, len);
                        }
                    }
                    break;
                    
                case UART_FIFO_OVF:
                    ESP_LOGW(TAG, "UART FIFO overflow");
                    uart_flush_input(ELRS_UART_NUM);
                    xQueueReset(uart_queue);
                    break;
                    
                case UART_BUFFER_FULL:
                    ESP_LOGW(TAG, "UART ring buffer full");
                    uart_flush_input(ELRS_UART_NUM);
                    xQueueReset(uart_queue);
                    break;
                    
                default:
                    break;
            }
        }
    }
    
    free(data);
    vTaskDelete(NULL);
}

// Initialize ExpressLRS Backpack UART
void elrs_backpack_init(void)
{
    ESP_LOGI(TAG, "Initializing ExpressLRS Backpack");
    
    // Configure UART parameters
    uart_config_t uart_config = {
        .baud_rate = ELRS_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    
    // Install UART driver
    ESP_ERROR_CHECK(uart_driver_install(ELRS_UART_NUM, 
                                       ELRS_RX_BUFFER_SIZE * 2, 
                                       ELRS_RX_BUFFER_SIZE * 2, 
                                       20, 
                                       &uart_queue, 
                                       0));
    
    ESP_ERROR_CHECK(uart_param_config(ELRS_UART_NUM, &uart_config));
    
    ESP_ERROR_CHECK(uart_set_pin(ELRS_UART_NUM, 
                                ELRS_UART_TX_PIN, 
                                ELRS_UART_RX_PIN, 
                                UART_PIN_NO_CHANGE, 
                                UART_PIN_NO_CHANGE));
    
    // Create UART receive task
    xTaskCreate(elrs_backpack_task, 
                "elrs_backpack", 
                4096, 
                NULL, 
                10, 
                NULL);
    
    ESP_LOGI(TAG, "ExpressLRS Backpack initialized (TX=%d, RX=%d, Baud=%d)", 
             ELRS_UART_TX_PIN, ELRS_UART_RX_PIN, ELRS_UART_BAUD_RATE);
}
