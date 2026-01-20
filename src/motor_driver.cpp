#include "motor_driver.h"
#include <ESP32Servo.h>
// CẤU HÌNH CHÂN & THÔNG SỐ (Toàn bộ phần cứng nằm ở đây)
#define PIN_SERVO   6
#define PIN_ESC     4

// --- SERVO ---
const int SERVO_LEFT   = 1000; 
const int SERVO_CENTER = 1500; 
const int SERVO_RIGHT  = 2000; 
const int SERVO_DB     = 10;   

// --- ESC ---
const int ESC_NEU      = 1500;
const int ESC_FWD_MAX  = 2000; 
const int ESC_REV_MAX  = 1000; 
const int ESC_DB       = 20;

// --- SMART BRAKE ---
const int16_t FWD_ACTIVE_TH = 60;   
const int16_t BRAKE_TRIGGER = -20;  
const int BRAKE_PULSE_US    = 1000; 
const uint32_t BRAKE_MS     = 250;  
const uint32_t NEU_MS       = 200;  

// Các biến nội bộ (Static để không bị file khác truy cập bậy)
static Servo steeringServo;
static Servo escServo;

enum Phase : uint8_t { DIRECT=0, BRAKING, HOLD_NEU };
static Phase phase = DIRECT;
static unsigned long phaseUntil = 0;

// Hàm tiện ích MapClamp (Chỉ dùng nội bộ file này)
static int mapClamp(long x, long in_min, long in_max, long out_min, long out_max) {
    long v = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    if (out_min < out_max) {
        if (v < out_min) v = out_min;
        if (v > out_max) v = out_max;
    } else {
        if (v > out_min) v = out_min;
        if (v < out_max) v = out_max;
    }
    return (int)v;
}

// Implementation
void motor_init() {
    steeringServo.attach(PIN_SERVO);
    escServo.attach(PIN_ESC);

    // Quy trình ARM ESC
    Serial.println("[MOTOR] Arming ESC...");
    escServo.writeMicroseconds(ESC_NEU);
    steeringServo.writeMicroseconds(SERVO_CENTER);
    delay(3000); // Chờ ESC nhận tín hiệu
    Serial.println("[MOTOR] Ready!");
}

void motor_control(int16_t throttleIn, int16_t steerIn) {
    // 1. XỬ LÝ LÁI
    int steerUs = mapClamp(steerIn, 0, 180, SERVO_LEFT, SERVO_RIGHT);
    if (abs(steerUs - SERVO_CENTER) <= SERVO_DB) steerUs = SERVO_CENTER;
    steeringServo.writeMicroseconds(steerUs);

    // 2. XỬ LÝ ESC VỚI SMART BRAKE
    unsigned long now = millis();

    // A. Máy trạng thái Phanh
    if (phase != DIRECT) {
        if (now < phaseUntil) {
            int out = (phase == BRAKING) ? BRAKE_PULSE_US : ESC_NEU;
            escServo.writeMicroseconds(out);
            return;
        } else {
            if (phase == BRAKING) {
                phase = HOLD_NEU;
                phaseUntil = now + NEU_MS;
                escServo.writeMicroseconds(ESC_NEU);
                return;
            } else {
                phase = DIRECT;
            }
        }
    }

    // B. Logic kích hoạt Phanh
    static int16_t prevThr = 0;
    bool wasForward = (prevThr > FWD_ACTIVE_TH);
    bool pullBack   = (throttleIn < BRAKE_TRIGGER);

    if (wasForward && pullBack) {
        phase = BRAKING;
        phaseUntil = now + BRAKE_MS;
        escServo.writeMicroseconds(BRAKE_PULSE_US);
        prevThr = throttleIn;
        return;
    }

    // C. Điều khiển bình thường
    int escUs = ESC_NEU;
    if (throttleIn >= 0) {
        escUs = mapClamp(throttleIn, 0, 255, ESC_NEU, ESC_FWD_MAX);
    } else {
        escUs = mapClamp(throttleIn, -255, 0, ESC_REV_MAX, ESC_NEU);
    }

    if (abs(escUs - ESC_NEU) <= ESC_DB) escUs = ESC_NEU;
    escServo.writeMicroseconds(escUs);
    prevThr = throttleIn;
}
