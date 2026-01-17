#include <Arduino.h>
#include "state_machine.h"

CarState currentState = STATE_DISCONNECTED;
unsigned long lastPacketTime = 0;

// Cập nhật trạng thái khi nhận được gói tin
void updateCarState(ControlPacket *data) {
    unsigned long now = millis();
    
    // Nếu Checksum sai -> Bỏ qua gói tin này
    if (data->checksum != calculateChecksum(data)) return;

    lastPacketTime = now; // Lưu thời gian nhận tin cuối cùng
    
    if (data->mode == 1) currentState = STATE_AUTO;
    else currentState = STATE_MANUAL;
}

// Lấy trạng thái hiện tại (Kiểm tra mất kết nối)
CarState getCurrentState() {
    // Nếu quá 1 giây (1000ms) không nhận được tin -> Coi như mất sóng
    if (millis() - lastPacketTime > 1000) return STATE_DISCONNECTED;
    return currentState;
}

// LOGIC TỰ LÁI (Dùng cảm biến siêu âm)
void runAutoLogic(float distance, int &outThrottle, int &outSteering) {
    if (distance < 30.0) { 
        // Gần vật cản (< 30cm): Lùi lại và bẻ hết lái
        outThrottle = -150; // Tốc độ lùi vừa phải
        outSteering = 0;    // Bẻ lái sang trái tối đa
    } else {
        // Đường thoáng: Đi tới chậm rãi
        outThrottle = 100;
        outSteering = 90;   // Đi thẳng
    }
}