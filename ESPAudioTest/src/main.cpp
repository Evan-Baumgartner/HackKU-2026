#include <Arduino.h>
#include <driver/i2s.h>

// Pin definitions - adjust if needed
#define I2S_WS    4
#define I2S_SCK   6
#define I2S_SD    21

#define SAMPLE_RATE     16000
#define SAMPLE_BUFFER   1024

static int32_t dc_offset = 0;

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate          = SAMPLE_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,  // SPH0645 outputs 32-bit frames
    .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,  // SEL pin tied to GND = left channel
    .communication_format = I2S_COMM_FORMAT_STAND_MSB,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 4,
    .dma_buf_len          = SAMPLE_BUFFER,
    .use_apll             = false,
    .tx_desc_auto_clear   = false,
    .fixed_mclk           = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num   = I2S_SCK,
    .ws_io_num    = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = I2S_SD,
  };

  esp_err_t err;

  err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("i2s_driver_install failed: %d\n", err);
    while (1);
  }

  err = i2s_set_pin(I2S_NUM_0, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("i2s_set_pin failed: %d\n", err);
    while (1);
  }

  // Clear the DMA buffer
  i2s_zero_dma_buffer(I2S_NUM_0);
  Serial.println("I2S initialized OK");
}

int16_t removeDCOffset(int32_t raw_sample) {
  dc_offset = dc_offset + ((raw_sample - dc_offset) >> 10);
  int32_t filtered = (raw_sample - dc_offset) >> 11;
  
  if (filtered > 32767) filtered = 32767;
  if (filtered < -32767) filtered = -32767;
  
  return (int16_t)filtered;
}

void readAudio(int16_t* buf, size_t samples) {
  int32_t raw[samples];
  size_t bytes_read = 0;

  i2s_read(I2S_NUM_0, raw, sizeof(raw), &bytes_read, portMAX_DELAY);

  size_t samples_read = bytes_read / sizeof(int32_t);

  for (size_t i = 0; i < samples_read; i++) {
    buf[i] = removeDCOffset(raw[i]);
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  delay(2000);
  setupI2S();
}

void loop() {
  int16_t samples[SAMPLE_BUFFER];
  readAudio(samples, SAMPLE_BUFFER);

  for (int i = 0; i < SAMPLE_BUFFER; i++) {
  Serial.printf(">audio:%d\n", samples[i]);
  }
  
}