#include "rx5808.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "sys/unistd.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "hwvers.h"
#include "led.h"
#include "nvs_flash.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "RX5808";

// SPI Bus Mutex - protects soft SPI from concurrent access
static SemaphoreHandle_t spi_mutex = NULL;
// Serialises all adc_oneshot_read() calls so that the Core-1 RSSI burst and
// the Core-0 LVGL keypad reader never race on the same adc1_handle.
// adc_oneshot_read() is not thread-safe per IDF documentation.
static SemaphoreHandle_t adc_mutex = NULL;

// Hardware SPI device handle for RX5808 (DMA-accelerated)
static spi_device_handle_t rx5808_spi = NULL;
static bool rx5808_spi_initialized = false;


#define Synthesizer_Register_A 				              0x00  
#define Synthesizer_Register_B 				              0x01  
#define Synthesizer_Register_C 				              0x02
#define Synthesizer_Register_D 				              0x03
#define VCO_Switch_Cap_Control_Register 		        0x04
#define DFC_Control_Register 				                0x05
#define _6M_Audio_Demodulator_Control_Register 			0x06
#define _6M5_Audio_Demodulator_Control_Register 	  0x07
#define Receiver_control_Register_1 				        0x08
#define Receiver_control_Register_2 				        0x09
#define Power_Down_Control_Register                 0x0A 
#define State_Register                              0x0F

// Performance optimization settings
#define RSSI_FILTER_SIZE 4              // Moving average filter size (reduces jitter)
#define RX5808_FREQ_SETTLING_TIME_MS 50 // Reduced from 150ms for faster channel switching

// ExpressLRS Backpack Detection
#define BACKPACK_DETECTION_ENABLED 1     // Set to 0 to disable backpack detection
#define BACKPACK_CHECK_INTERVAL_MS 500   // Check for backpack activity every 500ms  



//adc1_channel_t adc_dma_chan[]={RX5808_RSSI0_CHAN,RX5808_RSSI1_CHAN,VBAT_ADC_CHAN,KEY_ADC_CHAN};

// 彻底关断接收机
bool RX5808_Shutdown = false;
//uint16_t adc_convert_temp0[32][3];
//uint16_t adc_convert_temp1[32][3];
uint16_t adc_converted_value[3]={1024,1024,1024};

// RSSI filtering buffers for smoother readings
static uint16_t rssi0_filter_buffer[RSSI_FILTER_SIZE] = {0};
static uint16_t rssi1_filter_buffer[RSSI_FILTER_SIZE] = {0};
// Each buffer has its own independent write-index so that calling
// Rx5808_Get_Precentage0() and Rx5808_Get_Precentage1() in any order
// never advances the other buffer's cursor out of phase.
static uint8_t rssi0_filter_index = 0;
static uint8_t rssi1_filter_index = 0;

// Band X (User Favorites) custom frequency storage
// These are modifiable copies of Band X frequencies
static uint16_t Band_X_Custom_Freq[8] = {5740,5760,5780,5800,5820,5840,5860,5880};
static bool Band_X_Loaded = false;

// NVS keys for Band X
#define NVS_NAMESPACE_BANDX "band_x"
// Current format (since v1.8.x): single blob storing uint16_t[8]
#define NVS_KEY_BANDX_FREQS "x_freqs"
// Legacy per-channel keys (pre-v1.8.x) — kept only for one-time migration on upgrade
#define NVS_KEY_BANDX_CH1 "x_ch1"
#define NVS_KEY_BANDX_CH2 "x_ch2"
#define NVS_KEY_BANDX_CH3 "x_ch3"
#define NVS_KEY_BANDX_CH4 "x_ch4"
#define NVS_KEY_BANDX_CH5 "x_ch5"
#define NVS_KEY_BANDX_CH6 "x_ch6"
#define NVS_KEY_BANDX_CH7 "x_ch7"
#define NVS_KEY_BANDX_CH8 "x_ch8"

// ExpressLRS Backpack Detection State
static bool backpack_detected = false;
static uint16_t expected_frequency = 5800;  // Track what frequency we last set
static uint32_t last_freq_set_time_ms = 0;
static uint32_t backpack_detected_time_ms = 0;

volatile int8_t channel_count = 0;
volatile int8_t Chx_count = 0;
volatile uint8_t Rx5808_channel;
volatile uint16_t Rx5808_RSSI_Ad_Min0=0;
volatile uint16_t Rx5808_RSSI_Ad_Max0=4095;
volatile uint16_t Rx5808_RSSI_Ad_Min1=0;
volatile uint16_t Rx5808_RSSI_Ad_Max1=4095;
volatile uint16_t Rx5808_OSD_Format=0;
volatile uint16_t Rx5808_Language=1;
volatile uint16_t Rx5808_Signal_Source=0;
volatile uint16_t Rx5808_LED_Brightness=100;
// ELRS Backpack: No longer a simple toggle - binding managed through page_setup.c UI
volatile uint16_t Rx5808_CPU_Freq=3;  // AUTO by default (0=80MHz, 1=160MHz, 2=240MHz, 3=AUTO)
volatile uint16_t Rx5808_GUI_Update_Rate=1;  // 70ms (14Hz) by default (0=100ms/10Hz, 1=70ms/14Hz, 2=50ms/20Hz, 3=40ms/25Hz, 4=20ms/50Hz, 5=10ms/100Hz)

const char Rx5808_ChxMap[7] = {'A', 'B', 'E', 'F', 'R', 'L', 'X'};
const uint16_t Rx5808_Freq[7][8]=
{
	{5865,5845,5825,5805,5785,5765,5745,5725},	    //A	CH1-8 (BOSCAM_A)
    {5733,5752,5771,5790,5809,5828,5847,5866},		//B	CH1-8 (BOSCAM_B)
    {5705,5685,5665,5645,5885,5905,5925,5945},		//E	CH1-8 (BOSCAM_E) - restored original frequencies
    {5740,5760,5780,5800,5820,5840,5860,5880},		//F	CH1-8 (FATSHARK)
    {5658,5695,5732,5769,5806,5843,5880,5917},		//R	CH1-8 (RACEBAND)
    {5362,5399,5436,5473,5510,5547,5584,5621},		//L	CH1-8 (LOWBAND)
    {5658,5695,5732,5769,5806,5843,5880,5917}		//X	CH1-8 (USER FAVORITES - default to RACEBAND)
};

static adc_oneshot_unit_handle_t adc1_handle = NULL;



void RX5808_RSSI_ADC_Init(void)
{
    /* --- IDF v5.x oneshot ADC driver --- */
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id  = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc1_handle));

    adc_oneshot_chan_cfg_t ch_cfg = {
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, RX5808_RSSI0_CHAN, &ch_cfg));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, RX5808_RSSI1_CHAN, &ch_cfg));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, VBAT_ADC_CHAN,     &ch_cfg));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, KEY_ADC_CHAN,      &ch_cfg));
}

/**
 * @brief Read a raw ADC sample from any configured ADC1 channel.
 *        Used by other drivers (e.g. lvgl keypad) that share the oneshot unit.
 * @param channel  ADC1 channel number (adc_channel_t cast to int)
 * @return raw 12-bit ADC reading, or 0 on error
 */
int RX5808_ADC_Read_Raw(int channel)
{
    int raw = 0;
    if (adc_mutex != NULL) xSemaphoreTake(adc_mutex, portMAX_DELAY);
    adc_oneshot_read(adc1_handle, (adc_channel_t)channel, &raw);
    if (adc_mutex != NULL) xSemaphoreGive(adc_mutex);
    return raw;
}

/**
 * @brief Initialize hardware SPI for RX5808 (DMA-accelerated)
 */
static void RX5808_Init_Hardware_SPI(void) {
    esp_err_t ret;
    
    // VSPI bus configuration for RX5808
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,                      // RX5808 is write-only
        .mosi_io_num = RX5808_MOSI,             // GPIO 23
        .sclk_io_num = RX5808_SCLK,             // GPIO 18
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4,                   // Only need 4 bytes (32 bits)
    };
    
    // RX5808 device configuration
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1*1000*1000,          // 1MHz (RX5808 max is ~2MHz, be conservative)
        .mode = 0,                              // SPI mode 0 (CPOL=0, CPHA=0)
        .spics_io_num = RX5808_CS,              // GPIO 5
        .queue_size = 4,                        // Queue up to 4 transactions
        .pre_cb = NULL,
        .post_cb = NULL,
        .flags = SPI_DEVICE_BIT_LSBFIRST,       // RX5808 expects LSB first
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
    };
    
    // Initialize VSPI bus (SPI3)
    ret = spi_bus_initialize(VSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize VSPI bus: %s", esp_err_to_name(ret));
        return;
    }
    
    // Attach RX5808 to the SPI bus
    ret = spi_bus_add_device(VSPI_HOST, &devcfg, &rx5808_spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add RX5808 to SPI bus: %s", esp_err_to_name(ret));
        return;
    }
    
    rx5808_spi_initialized = true;
    ESP_LOGI(TAG, "RX5808 hardware SPI initialized (DMA-accelerated, 1MHz)");
}


void RX5808_Init()
{
    // Create SPI mutex for bus protection
    spi_mutex = xSemaphoreCreateMutex();
    if (spi_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create SPI mutex!");
    }
    // Create ADC mutex — must exist before RX5808_RSSI_ADC_Init() and before
    // the RSSI task starts, so any early RX5808_ADC_Read_Raw() call is safe.
    adc_mutex = xSemaphoreCreateMutex();
    if (adc_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create ADC mutex!");
    }
    
    // Initialize hardware SPI for RX5808 (DMA-accelerated)
    RX5808_Init_Hardware_SPI();

    // GPIO setup (CS and switches still need GPIO control)
	gpio_set_direction(RX5808_SWITCH0, GPIO_MODE_OUTPUT);
	gpio_reset_pin(RX5808_SWITCH1);	
	gpio_set_direction(RX5808_SWITCH1, GPIO_MODE_OUTPUT);	

	gpio_set_level(RX5808_SWITCH0, 1);
	gpio_set_level(RX5808_SWITCH1, 0);
	
	Send_Register_Data(Synthesizer_Register_A,0x00008);

	Send_Register_Data(Power_Down_Control_Register,0x10DF3);

	// Initialize Band X from NVS
	RX5808_Init_Band_X();

	RX5808_Set_Freq(RX5808_Get_Current_Freq());

	RX5808_RSSI_ADC_Init();	

    // xTaskCreate((TaskFunction_t)DMA2_Stream0_IRQHandler, // 任务函数
	// 		"rx5808_task",//任务名称，没什么用
	// 		1024,//任务堆栈大小
	// 		NULL,//传递给任务函数的参数
	// 		5,//任务优先级
	// 		NULL//任务句柄
	// 		);

	// Move RSSI ADC sampling to Core 1 for better load distribution (v1.7.1)
	xTaskCreatePinnedToCore( (TaskFunction_t)DMA2_Stream0_IRQHandler,
	                          "rx5808_rssi", 
							  1536,  // Increased stack for Core 1
							  NULL,
							   5,
							    NULL, 
								1 );  // Core 1
}

void RX5808_Pause() {
	RX5808_Shutdown = true;
	gpio_set_level(RX5808_SWITCH0, 0);
	gpio_set_level(RX5808_SWITCH1, 0);
}
void RX5808_Resume() {
	RX5808_Shutdown = false;
	gpio_set_level(RX5808_SWITCH0, 1);
	gpio_set_level(RX5808_SWITCH1, 0);
}
void Soft_SPI_Send_One_Bit(uint8_t bit)
{
	gpio_set_level(RX5808_SCLK, 0);
	usleep(20);
    gpio_set_level(RX5808_MOSI, ((bit&0x01)==1));
	usleep(30);
	gpio_set_level(RX5808_SCLK, 1);
	usleep(20);
}

void Send_Register_Data(uint8_t addr,uint32_t data)   
{
    // Serialise all SPI accesses with a single mutex (fix M).
    //
    // The hardware SPI path uses spi_device_queue_trans() followed by
    // spi_device_get_trans_result().  Although spi_device_queue_trans() is
    // internally queue-safe, the queue/get pair is NOT atomic: a second task
    // calling queue_trans while the first task is inside get_trans_result
    // could steal the transaction result.  Since v1.8.x introduced a
    // dedicated diversity_task that calls RX5808_Set_Freq() concurrently with
    // the ELRS backpack task, the mutex must wrap both the hardware and soft-
    // SPI branches.
    //
    // Note: spi_mutex is created at the very start of RX5808_Init(), before
    // any tasks that call Send_Register_Data() are spawned, so it is always
    // valid here.
    if (spi_mutex != NULL) {
        xSemaphoreTake(spi_mutex, portMAX_DELAY);
    }

  if (rx5808_spi_initialized && rx5808_spi != NULL) {
      // Hardware SPI with DMA (v1.7.1)
      // RX5808 protocol: 4-bit addr + 1-bit R/W + 20-bit data = 25 bits (LSB first)
      // Pack into 32 bits (4 bytes) with LSB first
      
      uint32_t spi_data = 0;
      spi_data |= (addr & 0x0F);              // 4-bit address at bits 3-0
      spi_data |= (1 << 4);                   // 1-bit write flag at bit 4
      spi_data |= (data & 0xFFFFF) << 5;      // 20-bit data at bits 24-5
      // Bits 31-25 are padding (ignored by RX5808 when CS goes high)
      
      // Prepare SPI transaction
      spi_transaction_t trans = {
          .flags = 0,
          .length = 25,                       // 25 bits (actual data length)
          .tx_buffer = &spi_data,
          .rx_buffer = NULL,
      };
      
      // Queue transaction (non-blocking with DMA)
      esp_err_t ret = spi_device_queue_trans(rx5808_spi, &trans, portMAX_DELAY);
      if (ret != ESP_OK) {
          ESP_LOGE(TAG, "SPI queue transaction failed: %s", esp_err_to_name(ret));
      }
      
      // Wait for transaction to complete (DMA handles the transfer)
      spi_transaction_t *rtrans;
      ret = spi_device_get_trans_result(rx5808_spi, &rtrans, portMAX_DELAY);
      if (ret != ESP_OK) {
          ESP_LOGE(TAG, "SPI get result failed: %s", esp_err_to_name(ret));
      }
  } else {
      // Fallback to bit-banged SPI if hardware SPI fails
      gpio_set_level(RX5808_CS, 0);
      uint8_t read_write=1;   //1 write     0 read
     
	  for(uint8_t i=0;i<4;i++)
	      Soft_SPI_Send_One_Bit(((addr>>i)&0x01));
      Soft_SPI_Send_One_Bit(read_write&0x01);
	  for(uint8_t i=0;i<20;i++)
	      Soft_SPI_Send_One_Bit(((data>>i)&0x01));
	  
	  gpio_set_level(RX5808_CS, 1);
	  gpio_set_level(RX5808_SCLK, 0);
	  gpio_set_level(RX5808_MOSI, 0);
  }

    if (spi_mutex != NULL) {
        xSemaphoreGive(spi_mutex);
    }
}

void RX5808_Set_Freq(uint16_t Fre)   
{
#if BACKPACK_DETECTION_ENABLED
	// Check if backpack is active - if so, skip SPI control
	if (backpack_detected) {
		ESP_LOGW(TAG, "Backpack active - skipping frequency change to %d MHz", Fre);
		return;
	}
#endif

	uint16_t F_LO=(Fre-479)>>1;
	uint16_t N;
	uint16_t A;
	
	N=F_LO/32;    
	A=F_LO%32;    

	Send_Register_Data(Synthesizer_Register_B,N<<7|A);
	
	// Track expected frequency and timestamp
	expected_frequency = Fre;
	last_freq_set_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
	
	// Wait for RX5808 to settle on new frequency (optimized for faster switching)
	vTaskDelay(RX5808_FREQ_SETTLING_TIME_MS / portTICK_PERIOD_MS);
}

void Rx5808_Set_Channel(uint8_t ch)
{
	if(ch>47)
		return ;
    Rx5808_channel=ch;
	Chx_count=Rx5808_channel/8;
	channel_count=Rx5808_channel%8;
}

void RX5808_Set_RSSI_Ad_Min0(uint16_t value)
{
    Rx5808_RSSI_Ad_Min0=value;
}

void RX5808_Set_RSSI_Ad_Max0(uint16_t value)
{
    Rx5808_RSSI_Ad_Max0=value;
}
void RX5808_Set_RSSI_Ad_Min1(uint16_t value)
{
    Rx5808_RSSI_Ad_Min1=value;
}
void RX5808_Set_RSSI_Ad_Max1(uint16_t value)
{
    Rx5808_RSSI_Ad_Max1=value;
}

void RX5808_Set_OSD_Format(uint16_t value)
{
    Rx5808_OSD_Format = value;
}

void RX5808_Set_Language(uint16_t value)
{
    Rx5808_Language = value;
}

void RX5808_Set_Signal_Source(uint16_t value)
{
    Rx5808_Signal_Source = value;
}

void RX5808_Set_LED_Brightness(uint16_t value)
{
	if (value > 100) value = 100;
	Rx5808_LED_Brightness = value;
	led_set_brightness((uint8_t)value);
}

// ELRS Backpack functions removed - binding now managed through ELRS_Backpack API in page_setup.c

void RX5808_Set_CPU_Freq(uint16_t value)
{
	if (value > 3) value = 3;  // Default to AUTO if invalid
    Rx5808_CPU_Freq = value;
	ESP_LOGI(TAG, "CPU Frequency set to: %s", value == 0 ? "80MHz" : (value == 1 ? "160MHz" : (value == 2 ? "240MHz" : "AUTO")));
}

void RX5808_Set_GUI_Update_Rate(uint16_t value)
{
    if (value > 5) value = 1;  // Default to 70ms if invalid
    Rx5808_GUI_Update_Rate = value;
    const char* rates_str[] = {"10Hz/100ms", "14Hz/70ms", "20Hz/50ms", "25Hz/40ms", "50Hz/20ms", "100Hz/10ms"};
    ESP_LOGI(TAG, "GUI/Diversity Update Rate set to: %s", rates_str[value]);
}

uint16_t Rx5808_Get_Channel()
{
	return Rx5808_channel;
}

uint16_t RX5808_Get_RSSI_Ad_Min0()
{
    return  Rx5808_RSSI_Ad_Min0;
}

uint16_t RX5808_Get_RSSI_Ad_Max0()
{
    return Rx5808_RSSI_Ad_Max0;
}
uint16_t RX5808_Get_RSSI_Ad_Min1()
{
    return Rx5808_RSSI_Ad_Min1;
}
uint16_t RX5808_Get_RSSI_Ad_Max1()
{
    return Rx5808_RSSI_Ad_Max1;
}

uint16_t RX5808_Get_OSD_Format()
{
    return Rx5808_OSD_Format;
}


uint16_t RX5808_Get_Language()
{
    return Rx5808_Language;
}
uint16_t RX5808_Get_Signal_Source()
{
    return Rx5808_Signal_Source;
}

uint16_t RX5808_Get_LED_Brightness()
{
	return Rx5808_LED_Brightness;
}

// ELRS Backpack functions removed - binding now managed through ELRS_Backpack API in page_setup.c

uint16_t RX5808_Get_CPU_Freq()
{
    return Rx5808_CPU_Freq;
}

uint16_t RX5808_Get_GUI_Update_Rate()
{
    return Rx5808_GUI_Update_Rate;
}

bool RX5808_Calib_RSSI(uint16_t min0,uint16_t max0,uint16_t min1,uint16_t max1)
{
    //if((min0>=max0)||(min1>=max1))
	//return false;

	if((min0+RX5808_CALIB_RSSI_ADC_VALUE_THRESHOULD<=max0)&&(min1+RX5808_CALIB_RSSI_ADC_VALUE_THRESHOULD<=max1))
	return true;

	return false; 
}

float Rx5808_Calculate_RSSI_Precentage(uint16_t value, uint16_t min, uint16_t max)
{
  if(max<=min)
  return 0;
  float precent=((float)((value - min)*100)) / (float)(max - min);	  
	if(precent>=99.0f)
		precent=99.0f;
	if(precent<=0)
		precent=0;
   return precent;   
}

// Apply moving average filter to RSSI value for smoother readings.
// index must be a pointer to the per-buffer write cursor (rssi0/1_filter_index).
static uint16_t filter_rssi(uint16_t new_value, uint16_t *buffer, uint8_t *index) {
    buffer[*index] = new_value;
    *index = (*index + 1) % RSSI_FILTER_SIZE;
    
    uint32_t sum = 0;
    for(int i = 0; i < RSSI_FILTER_SIZE; i++) {
        sum += buffer[i];
    }
    return sum / RSSI_FILTER_SIZE;
}

float Rx5808_Get_Precentage0()
{
    // Apply smoothing filter before calculating percentage
    uint16_t filtered_value = filter_rssi(adc_converted_value[0], rssi0_filter_buffer, &rssi0_filter_index);
    return Rx5808_Calculate_RSSI_Precentage(filtered_value, Rx5808_RSSI_Ad_Min0, Rx5808_RSSI_Ad_Max0);
}

float Rx5808_Get_Precentage1()
{
    // Apply smoothing filter before calculating percentage
    uint16_t filtered_value = filter_rssi(adc_converted_value[1], rssi1_filter_buffer, &rssi1_filter_index);
    return Rx5808_Calculate_RSSI_Precentage(filtered_value, Rx5808_RSSI_Ad_Min1, Rx5808_RSSI_Ad_Max1);
}

float Get_Battery_Voltage()
{
	//return (float)adc_converted_value[2]/4095*53.3375;
    return (float)adc_converted_value[2]/4095*6.8;
}



void DMA2_Stream0_IRQHandler(void)     
{
	//static uint16_t count=0;

	while(1)
	{
    uint32_t sum0=0,sum1=0;

    int _adc_raw;
    // Hold adc_mutex for the entire burst so RX5808_ADC_Read_Raw() (Core 0,
    // LVGL keypad) cannot interleave and get a 0 back from a busy unit.
    if (adc_mutex != NULL) xSemaphoreTake(adc_mutex, portMAX_DELAY);
    for(int i=0;i<16;i++)
    {
        adc_oneshot_read(adc1_handle, RX5808_RSSI0_CHAN, &_adc_raw); sum0 += _adc_raw;
        adc_oneshot_read(adc1_handle, RX5808_RSSI1_CHAN, &_adc_raw); sum1 += _adc_raw;
    }
    // Note: static cur/pre receiver_best locals removed — only used in old simple-diversity
    // else-branch which has been deleted (see below).
    adc_converted_value[0] = sum0 >> 4;
    adc_converted_value[1] = sum1 >> 4;
    adc_oneshot_read(adc1_handle, VBAT_ADC_CHAN, &_adc_raw);
    adc_converted_value[2] = (uint16_t)_adc_raw;
    if (adc_mutex != NULL) xSemaphoreGive(adc_mutex);
		
	int sig_src = Rx5808_Signal_Source;
	// 关断则都为0
	if(RX5808_Shutdown) {
		sig_src = 3;
	}
	if(sig_src==1) {
		gpio_set_level(RX5808_SWITCH1, 1);
		gpio_set_level(RX5808_SWITCH0, 0);

	}
	else if(sig_src==2) {
		gpio_set_level(RX5808_SWITCH0, 1);
		gpio_set_level(RX5808_SWITCH1, 0);
	}
	else if(sig_src==3) {
		gpio_set_level(RX5808_SWITCH0, 0);
		gpio_set_level(RX5808_SWITCH1, 0);
	}
	// sig_src == 0 (Auto): diversity.c task owns GPIO switching via
	// diversity_perform_switch(). Do NOT write SWITCH0/SWITCH1 here —
	// the old simple algorithm ran with inverted polarity vs diversity.c
	// and caused them to fight each other on every 25 ms cycle.
	//	(count==100)?(count=0):(++count);
	vTaskDelay(25 / portTICK_PERIOD_MS);  // Changed from 10ms to 25ms to reduce ADC polling rate (thermal optimization)
	}
		
}

// ============================================================================
// ExpressLRS Backpack Detection Functions
// ============================================================================

/**
 * @brief Check for ExpressLRS backpack activity
 * 
 * This function should be called periodically (every 500ms) to detect if an
 * ExpressLRS VRX backpack is controlling the RX5808 via the shared SPI bus.
 * 
 * Detection method:
 * - Monitors RSSI stability patterns
 * - If ESP32 hasn't set frequency recently but RSSI is stable, assume backpack control
 * - Auto-recovery: If backpack appears gone, try to reclaim SPI bus
 */
void RX5808_Check_Backpack_Activity(void)
{
#if BACKPACK_DETECTION_ENABLED
	uint32_t current_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
	
	// If we haven't tried to set frequency in last 2 seconds
	if ((current_time_ms - last_freq_set_time_ms) > 2000) {
		
		// Check RSSI stability - if both receivers show signal, backpack might be controlling
		uint16_t rssi0 = adc_converted_value[0];
		uint16_t rssi1 = adc_converted_value[1];
		
		// If RSSI values are significantly above noise floor, assume external control
		if ((rssi0 > 200 || rssi1 > 200) && !backpack_detected) {
			backpack_detected = true;
			backpack_detected_time_ms = current_time_ms;
			ESP_LOGW(TAG, "ExpressLRS Backpack detected - ESP32 control suspended");
		}
		
	} else {
		// We recently tried to set frequency, so we're in control
		if (backpack_detected && (current_time_ms - backpack_detected_time_ms) > 5000) {
			// Been in backpack mode for 5 seconds, try to reclaim
			backpack_detected = false;
			ESP_LOGI(TAG, "Attempting to reclaim SPI bus from backpack");
			// Re-apply our expected frequency
			RX5808_Set_Freq(expected_frequency);
		}
	}
#endif
}

/**
 * @brief Manually set backpack detection state
 * @param detected true if backpack is controlling, false otherwise
 */
void RX5808_Set_Backpack_Detected(bool detected)
{
	if (detected != backpack_detected) {
		backpack_detected = detected;
		if (detected) {
			ESP_LOGI(TAG, "Backpack control enabled");
		} else {
			ESP_LOGI(TAG, "Backpack control disabled - ESP32 resume");
			// Restore our frequency setting
			RX5808_Set_Freq(expected_frequency);
		}
	}
}

/**
 * @brief Check if backpack is currently detected
 * @return true if backpack is controlling RX5808, false if ESP32 has control
 */
bool RX5808_Is_Backpack_Detected(void)
{
	return backpack_detected;
}

/**
 * @brief Get expected frequency (what ESP32 last tried to set)
 * @return Frequency in MHz
 */
uint16_t RX5808_Get_Expected_Frequency(void)
{
	return expected_frequency;
}

/**
 * @brief Force clear backpack detection (for manual control recovery)
 */
void RX5808_Clear_Backpack_Detection(void)
{
	if (backpack_detected) {
		ESP_LOGI(TAG, "Manually clearing backpack detection");
		backpack_detected = false;
		last_freq_set_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
		// Re-apply frequency
		RX5808_Set_Freq(expected_frequency);
	}
}

/**
 * @brief Initialize Band X - load from NVS or use defaults
 */
void RX5808_Init_Band_X(void)
{
	if (!Band_X_Loaded) {
		// NVS flash must be initialized before any nvs_open() call.
		// nvs_flash_init() is idempotent: safe to call multiple times.
		// If the NVS partition is full or has a version mismatch, erase and reinit.
		esp_err_t ret = nvs_flash_init();
		if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
			ESP_LOGW(TAG, "NVS partition problem (%s), erasing and reinitialising", esp_err_to_name(ret));
			ESP_ERROR_CHECK(nvs_flash_erase());
			ret = nvs_flash_init();
		}
		ESP_ERROR_CHECK(ret);

		RX5808_Load_Band_X_From_NVS();
		Band_X_Loaded = true;
		ESP_LOGI(TAG, "Band X initialized");
	}
}

/**
 * @brief Set custom frequency for a Band X channel
 * @param channel Channel index (0-7)
 * @param freq Frequency in MHz (5362-5945 MHz typical range)
 */
void RX5808_Set_Band_X_Freq(uint8_t channel, uint16_t freq)
{
	if (channel >= 8) {
		ESP_LOGW(TAG, "Invalid Band X channel: %d", channel);
		return;
	}
	
	// Validate frequency range (typical FPV range)
	if (freq < 5300 || freq > 5950) {
		ESP_LOGW(TAG, "Frequency %d MHz out of typical range", freq);
	}
	
	Band_X_Custom_Freq[channel] = freq;
	ESP_LOGI(TAG, "Band X CH%d set to %d MHz", channel + 1, freq);
}

/**
 * @brief Get custom frequency for a Band X channel
 * @param channel Channel index (0-7)
 * @return Frequency in MHz
 */
uint16_t RX5808_Get_Band_X_Freq(uint8_t channel)
{
	if (channel >= 8) {
		ESP_LOGW(TAG, "Invalid Band X channel: %d", channel);
		return 5800; // Default fallback
	}
	return Band_X_Custom_Freq[channel];
}

/**
 * @brief Save Band X frequencies to NVS
 */
void RX5808_Save_Band_X_To_NVS(void)
{
	// NVS flash is guaranteed to be initialised by RX5808_Init_Band_X() on boot.
	// Do NOT call nvs_flash_init()/nvs_flash_erase() here — that would wipe all NVS data.
	nvs_handle_t nvs_handle;
	esp_err_t err = nvs_open(NVS_NAMESPACE_BANDX, NVS_READWRITE, &nvs_handle);
	
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to open NVS for Band X save: %s", esp_err_to_name(err));
		return;
	}
	
	// Store all 8 custom frequencies as a single 16-byte blob (fix L).
	// This reduces NVS write amplification from 8 writes to 1 and makes
	// the save atomic so a power-loss mid-save cannot corrupt a subset of channels.
	err = nvs_set_blob(nvs_handle, NVS_KEY_BANDX_FREQS,
	                   Band_X_Custom_Freq, sizeof(Band_X_Custom_Freq));
	if (err != ESP_OK) {
		ESP_LOGW(TAG, "Failed to save Band X blob: %s", esp_err_to_name(err));
	}
	
	err = nvs_commit(nvs_handle);
	if (err == ESP_OK) {
		ESP_LOGI(TAG, "Band X frequencies saved to NVS (blob)");
	} else {
		ESP_LOGE(TAG, "Failed to commit Band X to NVS: %s", esp_err_to_name(err));
	}
	
	nvs_close(nvs_handle);
}

/**
 * @brief Load Band X frequencies from NVS
 */
void RX5808_Load_Band_X_From_NVS(void)
{
	nvs_handle_t nvs_handle;
	// Need READWRITE so that we can re-save after a legacy migration in the same open
	esp_err_t err = nvs_open(NVS_NAMESPACE_BANDX, NVS_READWRITE, &nvs_handle);
	
	if (err != ESP_OK) {
		ESP_LOGI(TAG, "No saved Band X data, using defaults");
		return;
	}
	
	// Try current blob format first (written since v1.8.x, fix L)
	size_t blob_size = sizeof(Band_X_Custom_Freq);
	err = nvs_get_blob(nvs_handle, NVS_KEY_BANDX_FREQS, Band_X_Custom_Freq, &blob_size);
	if (err == ESP_OK && blob_size == sizeof(Band_X_Custom_Freq)) {
		ESP_LOGI(TAG, "Band X frequencies loaded from NVS (blob)");
		nvs_close(nvs_handle);
		return;
	}
	
	// Migration path: firmware prior to v1.8.x stored 8 separate u16 keys.
	// Read them, migrate the data into the blob format, then save so subsequent
	// boots use the faster, atomic blob approach.
	ESP_LOGI(TAG, "Band X blob not found — trying legacy per-key format (one-time migration)");
	const char* legacy_keys[8] = {
		NVS_KEY_BANDX_CH1, NVS_KEY_BANDX_CH2, NVS_KEY_BANDX_CH3, NVS_KEY_BANDX_CH4,
		NVS_KEY_BANDX_CH5, NVS_KEY_BANDX_CH6, NVS_KEY_BANDX_CH7, NVS_KEY_BANDX_CH8
	};

	bool any_loaded = false;
	for (uint8_t i = 0; i < 8; i++) {
		uint16_t freq;
		if (nvs_get_u16(nvs_handle, legacy_keys[i], &freq) == ESP_OK) {
			Band_X_Custom_Freq[i] = freq;
			any_loaded = true;
		}
	}
	nvs_close(nvs_handle);

	if (any_loaded) {
		ESP_LOGI(TAG, "Band X loaded from legacy keys — converting to blob format");
		// Save immediately using the new blob path so next boot is fast
		RX5808_Save_Band_X_To_NVS();
	}
}

/**
 * @brief Check if currently on Band X 
 * @return true if Band X is selected
 */
bool RX5808_Is_Band_X(void)
{
	return (Chx_count == 6); // Band X is index 6 (A=0, B=1, E=2, F=3, R=4, L=5, X=6)
}

/**
 * @brief Get current frequency based on band and channel selection
 * @return Frequency in MHz (uses Band X custom frequencies when applicable)
 */
uint16_t RX5808_Get_Current_Freq(void)
{
	// For Band X, use custom frequencies
	if (RX5808_Is_Band_X()) {
		return RX5808_Get_Band_X_Freq(channel_count);
	}
	
	// For standard bands, use the frequency table
	if (Chx_count < 6 && channel_count < 8) {
		return Rx5808_Freq[Chx_count][channel_count];
	}
	
	// Fallback
	return 5800;
}


