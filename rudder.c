#include <stdio.h>
#include <pico/stdlib.h>
#include "hardware/gpio.h"
#include "ads1115.h"
#include "registers.h"

// I2C setup
#define I2C_PORT i2c0
#define I2C_FREQ 400000
#define ADS1115_I2C_ADDR 0x48
const uint8_t SDA_PIN = 20;
const uint8_t SCL_PIN = 21;

// global adc struct
struct ads1115_adc adc;

// these values correspond to ADS1115 MUX register values, i.e. 0x00 = left rudder channel
typedef enum {
    ADC_RUDDER_LEFT = ADS1115_MUX_SINGLE_0,
    ADC_RUDDER_RIGHT = ADS1115_MUX_SINGLE_1,
    ADC_BRAKE_LEFT = ADS1115_MUX_SINGLE_2,
    ADC_BRAKE_RIGHT = ADS1115_MUX_SINGLE_3,
} ADC_CHANNEL;

typedef enum {
    CONVERTING,
    READY
} ADC_STATE;

// ADC channel currently of interest
volatile ADC_CHANNEL adc_channel;
volatile ADC_STATE adc_state;

// values reported by ADC
uint16_t rudder_left;
uint16_t rudder_right;
uint16_t brake_left;
uint16_t brake_right;

// callback for ADC complete
void gpio_callback(uint gpio, uint32_t events) {
    adc_state = READY;
}

#define ADC_IRQ_PIN 2
void setup_i2c(void);
void adc_process(void);
ADC_CHANNEL adc_next(ADC_CHANNEL current_channel);

int main() {
    stdio_init_all();
    setup_i2c();

    printf("Rudder Hall Effect values via interrupt\n");
 
    // setup GPIO pin to receive conversion ready interrupt
    gpio_init(ADC_IRQ_PIN);
    gpio_set_irq_enabled_with_callback(ADC_IRQ_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);

    // ADS1115 Config Register
    // 15   14  13  12  11  10  9   8       7   6   5   4           3           2           1   0
    // OS   MUX[2:0]    PGA[2:0]    MODE    DR[2:0]     COMP_MODE   COMP_POL    COMP_LAT    COMP_QUE[1:0]

    // Initialise ADS1115; get initial value
    ads1115_init(I2C_PORT, ADS1115_I2C_ADDR, &adc);

    // MUX: start set to first channel
    ads1115_set_input_mux(ADC_RUDDER_LEFT, &adc);
    // PGA: full-scale is 4.096v
    ads1115_set_pga(ADS1115_PGA_4_096, &adc);
    // MODE: single shot (each read is individually triggered)
    ads1115_set_operating_mode(ADS1115_MODE_SINGLE_SHOT, &adc);
    // DR: go real fast
    ads1115_set_data_rate(ADS1115_RATE_860_SPS, &adc);
    // COMP_MODE: not used when conversion ready pin active
    // COMP_POL: set active high (trigger on falling edge)
    ads1115_set_comparator_polarity(ADS1115_COMPARATOR_POLARITY_HI, &adc);
    // COMP_LAT: not used when conversion ready pin active
    // COMP_QUE: write 00 to use conversion ready pin; also sets LO_THRESH, HI_THRESH
    ads1115_use_conversion_rdy(&adc);
    // Write the configuration for this to have an effect.
    ads1115_write_config(&adc);

    printf("ADS1115 config complete: now %u\n", adc);

    // set first ADC channel
    adc_channel = ADC_RUDDER_LEFT;

    // wait for OS 
    while ((adc.config & ADS1115_STATUS_MASK) == 0);

    ads1115_begin_conversion(&adc);

    while(1) {
        if (adc_state == READY) {
            // printf("adc_state is READY, time to process\n");
            adc_process();
        }
    }
}

// Process means get the value from the ADC and then switch the channel to the next
// sensor and begin conversion again.
void adc_process(void) {
    switch (adc_channel) {
        case ADC_RUDDER_LEFT:
            ads1115_read_last_conversion(&rudder_left, &adc);
            // printf("RUDDER_LEFT = %u\n", rudder_left);
            break;
        case ADC_RUDDER_RIGHT:
            ads1115_read_last_conversion(&rudder_right, &adc);
            // printf("RUDDER_RIGHT = %u\n", rudder_right);
            break;
        case ADC_BRAKE_LEFT:
            ads1115_read_last_conversion(&brake_left, &adc);
            // printf("BRAKE_LEFT = %u\n", brake_left);
            break;
        case ADC_BRAKE_RIGHT:
            ads1115_read_last_conversion(&brake_right, &adc);
            // printf("BRAKE_RIGHT = %u\n", brake_right);
            break;
        default: 
            // printf("adc_channel not handled in adc_process()!\n");  
            break;
    }
    // printf("Advancing adc_channel now %u\n", adc_channel);  
    adc_channel = adc_next(adc_channel);

    // set to conmverting so as not to execute adc_process until another value is ready
    adc_state = CONVERTING;
    // set and write new mux value and trigger conversion

    ads1115_set_input_mux(adc_channel, &adc);
    ads1115_write_config(&adc);
    ads1115_begin_conversion(&adc);
}

ADC_CHANNEL adc_next(ADC_CHANNEL current_channel) {
    ADC_CHANNEL next_channel;
    switch (current_channel) {
        case ADC_RUDDER_LEFT:
            return ADC_RUDDER_RIGHT;
        case ADC_RUDDER_RIGHT:
            return ADC_BRAKE_LEFT;
        case ADC_BRAKE_LEFT:
            return ADC_BRAKE_RIGHT;
        case ADC_BRAKE_RIGHT:
            return ADC_RUDDER_LEFT;
    }
    return next_channel;
}

void setup_i2c() {
    printf("Doing I2C setup port=%u, freq=%u, SDA=%d, SCL=%d\n", I2C_PORT, I2C_FREQ, SDA_PIN, SCL_PIN);
    i2c_init(I2C_PORT, I2C_FREQ);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
}
