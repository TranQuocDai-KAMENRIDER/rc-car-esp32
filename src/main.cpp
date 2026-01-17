#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <ESP32Servo.h>
#include "protocol.h"
#include "steering_logic.h"
#include "state_machine.h"
// 1. PIN MAPPING
// Cảm biến siêu âm HC-SR04
#define PIN_TRIG    15
#define PIN_ECHO    16

// Servo lái
#define PIN_SERVO   6

// Động cơ (Driver L298N)
// Motor Trái (IN1, IN2)
#define PIN_IN1     4
#define PIN_IN2     5
// Motor Phải (IN3, IN4)
#define PIN_IN3     7
#define PIN_IN4     8

// 2. KHAI BÁO BIẾN & ĐỐI TƯỢNG
uint8_t macCuaXe[] = {0xA0, 0x11, 0x11, 0x11, 0x11, 0x11}; // MAC của Xe
ControlPacket myData;
Servo steeringServo;

// Hàm đo khoảng cách (trả về cm)
float getDistance() {
    digitalWrite(PIN_TRIG, LOW); delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);
    
    long duration = pulseIn(PIN_ECHO, HIGH, 25000); // Timeout 25ms (~4m)
    if (duration == 0) return 100.0; // Không thấy gì -> coi như xa
    return duration * 0.034 / 2;
}

// Hàm điều khiển 1 động cơ (L298N)
// speed: -255 (Lùi max) đến 255 (Tiến max)
void controlMotor(int pinA, int pinB, int speed) {
    if (speed > 0) {
        analogWrite(pinA, speed);
        analogWrite(pinB, 0);
    } else if (speed < 0) {
        analogWrite(pinA, 0);
        analogWrite(pinB, -speed); // Đổi dấu thành dương
    } else {
        analogWrite(pinA, 0);
        analogWrite(pinB, 0);
    }
}

// Hàm cập nhật toàn bộ phần cứng
void setHardware(int leftSpeed, int rightSpeed, int servoAngle) {
    // 1. Điều khiển Servo
    steeringServo.write(servoAngle);

    // 2. Điều khiển Động cơ
    controlMotor(PIN_IN1, PIN_IN2, leftSpeed);
    controlMotor(PIN_IN3, PIN_IN4, rightSpeed);

    // Debug ra Serial để kiểm tra
    Serial.printf("L: %d | R: %d | Servo: %d\n", leftSpeed, rightSpeed, servoAngle);
}

// Callback khi nhận dữ liệu ESP-NOW
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    if (len == sizeof(myData)) {
        memcpy(&myData, incomingData, sizeof(myData));
        updateCarState(&myData);
    }
}

void setup() {
    Serial.begin(115200);

    // Cấu hình chân
    pinMode(PIN_TRIG, OUTPUT); pinMode(PIN_ECHO, INPUT);
    pinMode(PIN_IN1, OUTPUT); pinMode(PIN_IN2, OUTPUT);
    pinMode(PIN_IN3, OUTPUT); pinMode(PIN_IN4, OUTPUT);

    // Khởi động Servo
    steeringServo.attach(PIN_SERVO);

    // Khởi động Wifi & ESP-NOW
    WiFi.mode(WIFI_STA);
    esp_wifi_set_mac(WIFI_IF_STA, &macCuaXe[0]); // Ép MAC A0:11...
    
    if (esp_now_init() != ESP_OK) {
        Serial.println("Lỗi ESP-NOW!");
        return;
    }
    esp_now_register_recv_cb(OnDataRecv);
    
    Serial.println("XE S3-N16R8 DA SAN SANG!");
}

void loop() {
    // 1. Xác định trạng thái (Manual / Auto / Mất sóng)
    CarState state = getCurrentState();
    
    int finalL = 0, finalR = 0, finalSteer = 90;

    switch (state) {
        case STATE_DISCONNECTED:
            // Mất sóng -> Dừng xe ngay lập tức
            finalL = 0; finalR = 0; finalSteer = 90;
            Serial.println("!!! MAT KET NOI !!!");
            break;

        case STATE_MANUAL:
            // Lái tay -> Lấy dữ liệu từ gói tin
            finalSteer = myData.steering;
            calculateMotorSpeeds(myData.throttle, myData.steering, finalL, finalR);
            break;

        case STATE_AUTO:
            // Tự lái -> Dùng cảm biến siêu âm
            int autoThrot, autoSteer;
            float dist = getDistance();
            Serial.printf("Distance: %.1f cm | ", dist);
            
            runAutoLogic(dist, autoThrot, autoSteer);
            
            // Tính toán lại motor dựa trên logic tự lái
            calculateMotorSpeeds(autoThrot, autoSteer, finalL, finalR);
            finalSteer = autoSteer;
            break;
    }

    // 2. Ra lệnh cho phần cứng
    setHardware(finalL, finalR, finalSteer);

    delay(20); // Chu kỳ 20ms
}