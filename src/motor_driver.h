#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include <Arduino.h>

// Hàm khởi tạo Servo và ESC (Chạy 1 lần ở setup)
void motor_init();

// Hàm điều khiển xe (Chạy liên tục ở loop)
// throttle: -255 (Lùi) đến 255 (Tiến)
// steering: 0 (Trái) đến 180 (Phải)
void motor_control(int16_t throttle, int16_t steering);

#endif
