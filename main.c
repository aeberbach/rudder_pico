#include <stdio.h>
#include <pico/stdlib.h>
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/dma.h"

// tusb stuff
#include "bsp/board_api.h"
#include "tusb.h"
#include "usb_descriptors.h"

#include "rudder.h"

// I2C setup
#define I2C_PORT i2c0
#define I2C_FREQ 400000
#define ADS1115_I2C_ADDR 0x48

const uint8_t SDA_PIN = 20;
const uint8_t SCL_PIN = 21;

#define RUDDER_ADC_CHANNEL          0
#define RUDDER_ADC_GPIO             26
#define RIGHT_BRAKE_ADC_CHANNEL     1
#define RIGHT_BRAKE_ADC_GPIO        27
#define LEFT_BRAKE_ADC_CHANNEL      2
#define LEFT_BRAKE_ADC_GPIO         28

// prototypes
void hid_task(void);

volatile uint16_t adc_pending;
volatile uint16_t adc_received;
volatile uint16_t adc_raw;

void adc_isr() {
    adc_raw = adc_fifo_get();
    adc_received = adc_pending;
}

int main() {
    stdio_init_all();

    adc_init();

    adc_gpio_init(RUDDER_ADC_GPIO); 
    adc_gpio_init(LEFT_BRAKE_ADC_GPIO); 
    adc_gpio_init(RIGHT_BRAKE_ADC_GPIO); 

    adc_pending = RUDDER_ADC_GPIO;
    adc_select_input(RUDDER_ADC_CHANNEL);
    adc_received = 0;

    rudder_init();

	/* Write to FIFO length 1, and retain the ERR bit. */
	adc_fifo_setup(true, false, 1, false, false);
	adc_set_clkdiv(9600);
	irq_set_exclusive_handler(ADC_IRQ_FIFO, adc_isr);

    adc_irq_set_enabled(true);
   	irq_set_enabled(ADC_IRQ_FIFO, true);
	adc_run(true);

    // // init device stack on configured roothub port
    tusb_rhport_init_t dev_init = {.role = TUSB_ROLE_DEVICE,
                                   .speed = TUSB_SPEED_AUTO};
    tusb_init(BOARD_TUD_RHPORT, &dev_init);

    if (board_init_after_tusb) {
        board_init_after_tusb();
    }

    // loop
    // get an adc value and save it to the correct value when ISR indicates it is ready
    // service USB reports
    while(1) {
        // adc value is ready for reading, says the ISR
        if (adc_received != 0) {
            adc_run(false);
            switch(adc_received) {
                case RUDDER_ADC_GPIO:
                    rudder_new_value(adc_raw);
                    adc_pending = LEFT_BRAKE_ADC_GPIO;
                    adc_select_input(LEFT_BRAKE_ADC_CHANNEL);
                    break;
                case LEFT_BRAKE_ADC_GPIO:
                    brake_left_new_value(adc_raw);
                    adc_pending = RIGHT_BRAKE_ADC_GPIO;
                    adc_select_input(RIGHT_BRAKE_ADC_CHANNEL);
                    break;
                case RIGHT_BRAKE_ADC_GPIO:
                    brake_right_new_value(adc_raw);
                    adc_pending = RUDDER_ADC_GPIO;
                    adc_select_input(RUDDER_ADC_CHANNEL);
                    break;
            }
            adc_received = 0;
            adc_run(true);
        }
        tud_task();
        hid_task();
    }
}


