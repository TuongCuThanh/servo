#ifndef __SERVO_H__
#define __SERVO_H__

#include <stdint.h>
#include "driver/ledc.h"

// Định nghĩa GPIO sử dụng cho servo
#define GPIO_OUTPUT_IO_0    18
#define SERVO_MIN_PULSEWIDTH 1650
#define SERVO_MAX_PULSEWIDTH 4850
#define SERVO_MAX_DEGREE 90 

// Khởi tạo biến toàn cục cho LEDC timer và channel
#define LEDC_HS_TIMER          LEDC_TIMER_0
#define LEDC_HS_MODE           LEDC_HIGH_SPEED_MODE
#define LEDC_HS_CH0_CHANNEL    LEDC_CHANNEL_0
#define LEDC_HS_CH0_GPIO       (18)

void servo_init(void);
uint32_t servo_per_degree_init(uint32_t degree_of_rotation);

extern uint32_t targetAngle;

#endif // __SERVO_H__
