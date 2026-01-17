#include <Arduino.h>
#include "steering_logic.h"

void calculateMotorSpeeds(int throttle, int steering, int &leftSpeed, int &rightSpeed) {
    // 1. Chuyển đổi góc lái (0-180) sang tỉ lệ phần trăm (-100 đến 100)
    // 0 -> -100 (Trái), 90 -> 0 (Thẳng), 180 -> 100 (Phải)
    int turnFactor = map(steering, 0, 180, -100, 100);

    // 2. Công thức trộn xung (Differential Steering)
    // Logic: Khi rẽ trái, bánh trái quay chậm, bánh phải quay nhanh và ngược lại
    int left = throttle + turnFactor;
    int right = throttle - turnFactor;

    // 3. Giới hạn giá trị trong khoảng cho phép của PWM (-255 đến 255)
    leftSpeed = constrain(left, -255, 255);
    rightSpeed = constrain(right, -255, 255);
}