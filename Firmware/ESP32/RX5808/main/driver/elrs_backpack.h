#ifndef __ELRS_BACKPACK_H
#define __ELRS_BACKPACK_H

#include <stdint.h>
#include <stdbool.h>

// CRSF Protocol Defines
#define CRSF_SYNC_BYTE                  0xC8
#define CRSF_ADDRESS_BROADCAST          0x00
#define CRSF_ADDRESS_CRSF_TRANSMITTER   0xEE
#define CRSF_ADDRESS_VTX                0x03

// CRSF Frame Types
#define CRSF_FRAMETYPE_MSP_REQ          0x7A
#define CRSF_FRAMETYPE_MSP_RESP         0x7B
#define CRSF_FRAMETYPE_MSP_WRITE        0x7C

// MSP Commands for VRX
#define MSP_SET_VTX_CONFIG              88
#define MSP_VTX_CONFIG                  89

// VRX Band/Channel Mapping (matches RX5808 bands)
#define VRX_BAND_A                      0
#define VRX_BAND_B                      1
#define VRX_BAND_E                      2
#define VRX_BAND_F                      3
#define VRX_BAND_R                      4
#define VRX_BAND_L                      5

// Configuration
#define ELRS_UART_NUM                   UART_NUM_1
#define ELRS_UART_BAUD_RATE             420000  // Standard CRSF baud rate
#define ELRS_UART_TX_PIN                GPIO_NUM_17  // Adjust based on your hardware
#define ELRS_UART_RX_PIN                GPIO_NUM_16  // Adjust based on your hardware
#define ELRS_RX_BUFFER_SIZE             256

// CRSF Frame Structure
typedef struct {
    uint8_t device_addr;
    uint8_t frame_size;
    uint8_t type;
    uint8_t payload[64];
    uint8_t crc;
} __attribute__((packed)) crsf_frame_t;

// VRX Configuration Structure
typedef struct {
    uint8_t band;       // 0-5: A, B, E, F, R, L
    uint8_t channel;    // 0-7: Channel 1-8
    uint8_t power;      // VTX power (not used for VRX)
    uint8_t pit_mode;   // Pit mode (not used for VRX)
    uint16_t freq;      // Frequency in MHz
} vrx_config_t;

// Function Declarations
void elrs_backpack_init(void);
void elrs_backpack_task(void *param);
void elrs_send_vrx_config(vrx_config_t *config);
void elrs_process_msp_command(uint8_t *payload, uint8_t length);
uint8_t elrs_crc8_dvb_s2(uint8_t crc, uint8_t a);
void elrs_send_crsf_frame(crsf_frame_t *frame);
uint16_t elrs_band_channel_to_freq(uint8_t band, uint8_t channel);
void elrs_freq_to_band_channel(uint16_t freq, uint8_t *band, uint8_t *channel);

#endif // __ELRS_BACKPACK_H
