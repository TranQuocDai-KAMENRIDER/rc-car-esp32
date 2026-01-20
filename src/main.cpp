#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "protocol.h"
#include "motor_driver.h" // <--- Gọi thợ máy vào làm việc

// MAC của Xe
uint8_t macCuaXe[] = {0x32, 0xAE, 0xAE, 0xAE, 0xAE, 0x02};

ControlPacket myData;
unsigned long lastRecvTime = 0; // Biến Failsafe

// Callback khi nhận tin
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    if (len == sizeof(myData)) {
        memcpy(&myData, incomingData, sizeof(myData));
        lastRecvTime = millis();
    }
}

void setup() {
    Serial.begin(115200);

    // 1. Khởi động phần cứng Motor (Servo & ESC)
    motor_init(); 

    // 2. Khởi động Kết nối
    WiFi.mode(WIFI_STA);
    esp_wifi_set_mac(WIFI_IF_STA, &macCuaXe[0]);
    
    if (esp_now_init() != ESP_OK) {
        Serial.println("Lỗi ESP-NOW!");
        return;
    }
    esp_now_register_recv_cb(OnDataRecv);
    
    Serial.println("SYSTEM READY!");
}

void loop() {
    // FAILSAFE: Mất sóng quá 1 giây -> Dừng xe
    if (millis() - lastRecvTime > 1000) {
        // Gửi lệnh Dừng (0) và Thẳng (90)
        motor_control(0, 90);
    } else {
        // Có sóng -> Chạy theo lệnh tay cầm
        motor_control(myData.throttle, myData.steering);
    }
    
    delay(20); 
}
