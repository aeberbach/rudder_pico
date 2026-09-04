#ifndef __RUDDER_H__
#define __RUDDER_H__

#include <pico/stdlib.h>

void rudder_init();
void rudder_new_value(uint16_t val);
int rudder_value();
void brake_left_new_value(uint16_t val);
int brake_left_value();
void brake_right_new_value(uint16_t val);
int brake_right_value();

#endif // __RUDDER_H__