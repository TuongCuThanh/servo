#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"
#include "driver/ledc.h"
#include "servo.h"

// Định nghĩa biến góc mục tiêu
uint32_t targetAngle = 0;

/**
 * @brief Sử dụng hàm này để tính toán độ rộng xung cho mỗi độ quay
 *
 * @param  degree_of_rotation góc quay theo độ mà servo cần quay đến
 *
 * @return
 *     - độ rộng xung đã được tính toán
 */

// định nghĩa constrain dùng để giới hạn góc quay 
uint32_t constrain(uint32_t x, uint32_t a, uint32_t b) {
    if (x < a) return a;
    if (x > b) return b;
    return x;
}

// Hàm để thiết lập góc của servo
uint32_t servo_per_degree_init(uint32_t degree_of_rotation) {
    // Đảm bảo góc quay nằm trong khoảng từ 0 đến SERVO_MAX_DEGREE
    degree_of_rotation = constrain(degree_of_rotation, 0, SERVO_MAX_DEGREE);

    // Tính toán pulse width dựa trên góc quay
    uint32_t cal_pulsewidth = SERVO_MIN_PULSEWIDTH + (degree_of_rotation * (SERVO_MAX_PULSEWIDTH - SERVO_MIN_PULSEWIDTH) / SERVO_MAX_DEGREE);
    
    // Thiết lập chu kỳ làm việc cho LEDC
    ledc_set_duty(LEDC_HS_MODE, LEDC_HS_CH0_CHANNEL, cal_pulsewidth);
    ledc_update_duty(LEDC_HS_MODE, LEDC_HS_CH0_CHANNEL);
    return cal_pulsewidth;
}

void servo_init(void) {
    // Cấu hình bộ đếm thời gian cho LEDC
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_16_BIT,
        .freq_hz = 50,  // Tần số 50 Hz cho điều khiển servo
        .speed_mode = LEDC_HS_MODE,
        .timer_num = LEDC_HS_TIMER
    };
    ledc_timer_config(&ledc_timer);

    // Cấu hình kênh LEDC
    ledc_channel_config_t ledc_channel = {
        .channel    = LEDC_HS_CH0_CHANNEL,
        .duty       = 0,
        .gpio_num   = LEDC_HS_CH0_GPIO,
        .speed_mode = LEDC_HS_MODE,
        .timer_sel  = LEDC_HS_TIMER
    };
    ledc_channel_config(&ledc_channel);
}
