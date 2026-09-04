#include <stdio.h>

#include "rudder.h"

#define RAW_VALUE_COUNT     10
#define ADC_MAX             4096
#define ADC_CLAMP_MAX       4000
#define ADC_MIN             400
#define ADC_CLAMP_MIN       500

// values reported by ADC
uint16_t rudder;
uint16_t rudder_raw_values[RAW_VALUE_COUNT];
uint16_t brake_left_raw_values[RAW_VALUE_COUNT];
uint16_t brake_right_raw_values[RAW_VALUE_COUNT];

// functions for internal use
uint16_t adc_clamped_value(uint16_t val);
uint16_t rudder_raw_value();
uint16_t brake_left_raw_value();
uint16_t brake_right_raw_value();

// Public interface

// fill the rudder raws values array with midpoint
void rudder_init() {
    uint16_t init_val = (ADC_MAX - ADC_MIN) / 2;
    for(int i = 0; i < RAW_VALUE_COUNT; i++) {
        rudder_raw_values[i] = init_val;
    }
}

// write a new value into the rudder raw values array, moving index to
// to position for next value. Index wraps to zero after last.
void rudder_new_value(uint16_t val) {
    static int index = 0;
    uint16_t clamped_val = adc_clamped_value(val);
    rudder_raw_values[index++] = clamped_val;

    // wrap around to zero if the array is full
    if (index == RAW_VALUE_COUNT) {
        index = 0;
    }
}

// return a rudder position value from -127..127
int rudder_value() {
    uint16_t raw_value = rudder_raw_value();
    int rudder_absolute = (int)(255.0 * (float)(raw_value - ADC_MIN) / (float)(ADC_MAX - ADC_MIN));
    int value = 127 - rudder_absolute;
    return value;
}

// write a new value into the brake left raw values array, moving index to
// to position for next value. Index wraps to zero after last.
void brake_left_new_value(uint16_t val) {
    static int index = 0;

    uint16_t clamped_val = adc_clamped_value(val / 2);
    brake_left_raw_values[index++] = clamped_val;

    // wrap around to zero if the array is full
    if (index == RAW_VALUE_COUNT) {
        index = 0;
    }
}

// return a brake position value from -127..127
int brake_left_value() {
    int brake_left = 127 - (255 * (brake_left_raw_value() - ADC_MIN) / (ADC_MAX - ADC_MIN));
    if (brake_left < -127) {
        printf("NOPE LEFT %d\n", brake_left);
    }
    return brake_left;
}

void brake_right_new_value(uint16_t val) {
    static int index = 0;

    brake_right_raw_values[index++] = adc_clamped_value(val / 2);

    // wrap around to zero if the array is full
    if (index == RAW_VALUE_COUNT) {
        index = 0;
    }
}

// return a brake position value from -127..127
int brake_right_value() {
    int brake_right = 127 - (255 * (brake_right_raw_value() - ADC_MIN) / (ADC_MAX - ADC_MIN));
    if (brake_right < -127) {
        printf("NOPE RIGHT %d\n", brake_right);
    }
    return brake_right;
}

// internal use

uint16_t adc_clamped_value(uint16_t val) {
    return MAX(ADC_CLAMP_MIN, MIN(ADC_CLAMP_MAX, val));
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
