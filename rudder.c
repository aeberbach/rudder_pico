#include <stdio.h>
#include <math.h>

#include "rudder.h"

#define RAW_VALUE_COUNT     10

// max and min values observed on the rudder axes
#define RUDDER_ADC_MAX      4096
#define RUDDER_ADC_MIN      830

// max and min values observed on the brake axes
#define BRAKE_ADC_MAX       4030
#define BRAKE_ADC_MIN       740

// values reported by ADC
uint16_t rudder;
uint16_t rudder_raw_values[RAW_VALUE_COUNT];
uint16_t brake_left_raw_values[RAW_VALUE_COUNT];
uint16_t brake_right_raw_values[RAW_VALUE_COUNT];

// functions for internal use
uint16_t adc_clamped_value(uint16_t val, uint16_t min, uint16_t max);
uint16_t rudder_raw_value();
uint16_t brake_left_raw_value();
uint16_t brake_right_raw_value();
int fitted_raw_brake_value(int raw);

// Public interface

// fill the rudder raws values array with midpoint
void rudder_init() {
    uint16_t init_val = (RUDDER_ADC_MAX - RUDDER_ADC_MIN) / 2;
    for(int i = 0; i < RAW_VALUE_COUNT; i++) {
        rudder_raw_values[i] = init_val;
    }
}

// write a new value into the rudder raw values array, moving index to
// to position for next value. Index wraps to zero after last.
void rudder_new_value(uint16_t val) {
    static int index = 0;
    uint16_t clamped_val = adc_clamped_value(val, RUDDER_ADC_MIN, RUDDER_ADC_MAX);
    rudder_raw_values[index++] = clamped_val;

    // wrap around to zero if the array is full
    if (index == RAW_VALUE_COUNT) {
        index = 0;
    }
}

// return a rudder position value from -127..127
int rudder_value() {
    uint16_t raw_value = rudder_raw_value();
    int rudder_absolute = (int)(255.0 * (float)(raw_value / (float)(RUDDER_ADC_MAX)));
    int value = 127 - rudder_absolute;

    // printf("Returning rudder value %d\n", value);
    return value;
}

// Making the brake response more linear was important. Using a very rough marking on the 
// pedal I was able to record the following values for various pedal positions:
//
// | ADC raw value | Rudder deflection |
// |---------------|-------------------|
// | 747			| 0.1  				|
// | 762			| 0.2  				|
// | 792			| 0.3  				|
// | 840			| 0.4  				|
// | 948			| 0.5  				|
// | 1090			| 0.6  				|
// | 1420			| 0.7  				|
// | 2070			| 0.8  				|
// | 2900			| 0.9  				|
// | 4026			| 1.0  				|
//
// then using an online curve fitting tool (https://www.standardsapplied.com/nonlinear-curve-fitting-calculator.html)
// I picked an algorithm that looked like a reasonable fit for the curve and one not particularly
// difficult to calculate, using only the math.h log() (natural logarithm) function. The function given is:
// y = 0.1746403809 * ln(x - 724.7640485) - 0.437040687

// the result is clamped to the range 0.0..1.0 and then scaled to -127..127 for the brake value returned to the host.
int fitted_raw_brake_value(int raw) {
    float adjusted = 0.1746403809 * log((double)raw - 724.7640485) - 0.437040687;
    float clamped_adjusted = MAX(MIN(adjusted, 1.0), 0.0);

    // the minimum joystick axis value is -127, maximum is 127. The brake at rest should
    // report -127 and as it is applied that value increases. An offset of 0..255 is added
    // based on the relative position of the brake pedal.
    int brake_value = (int)(-127.0 + (clamped_adjusted * 255.0));

    // printf("Brake left clamped_adjusted %f end value %d\n", clamped_adjusted, brake_value);

    return brake_value;
}

// write a new value into the brake left raw values array, moving index to
// to position for next value. Index wraps to zero after last.
void brake_left_new_value(uint16_t val) {
    static int index = 0;

    brake_left_raw_values[index++] = adc_clamped_value(val, BRAKE_ADC_MIN, BRAKE_ADC_MAX);

    // wrap around to zero if the array is full
    if (index == RAW_VALUE_COUNT) {
        index = 0;
    }
}

// return a brake position value from -127..127
int brake_left_value() {
    int raw = (double)brake_left_raw_value();
    int fitted_value = fitted_raw_brake_value(raw);

    // printf("Brake left %d\n", fitted_value);

    return fitted_value;
}

void brake_right_new_value(uint16_t val) {
    static int index = 0;

    brake_right_raw_values[index++] = adc_clamped_value(val, BRAKE_ADC_MIN, BRAKE_ADC_MAX);

    // wrap around to zero if the array is full
    if (index == RAW_VALUE_COUNT) {
        index = 0;
    }
}

// return a brake position value from -127..127
int brake_right_value() {
    int raw = (double)brake_right_raw_value();
    int fitted_value = fitted_raw_brake_value(raw);

    // printf("Brake right %d\n", fitted_value);

    return fitted_value;
}

// internal use
uint16_t adc_clamped_value(uint16_t val, uint16_t min, uint16_t max) {
    return MAX(min, MIN(max, val));
}

// return the average of all values in rudder raw values
uint16_t rudder_raw_value() {
    uint16_t sum = 0;
    for(int i = 0; i < RAW_VALUE_COUNT; i++) {
        sum += rudder_raw_values[i];
    }
    return sum/RAW_VALUE_COUNT;
}

// return the average of all values in left brake raw values
uint16_t brake_left_raw_value() {
    uint16_t sum = 0;
    for(int i = 0; i < RAW_VALUE_COUNT; i++) {
        sum += brake_left_raw_values[i];
    }
    return sum/RAW_VALUE_COUNT;
}

// return the average of all values in right brake raw values
uint16_t brake_right_raw_value() {
    uint16_t sum = 0;
    for(int i = 0; i < RAW_VALUE_COUNT; i++) {
        sum += brake_right_raw_values[i];
    }
    return sum/RAW_VALUE_COUNT;
}
