#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H
#include "protocol.h"

// Định nghĩa các trạng thái của xe
enum CarState { STATE_DISCONNECTED, STATE_MANUAL, STATE_AUTO };

void updateCarState(ControlPacket *data);
CarState getCurrentState();
void runAutoLogic(float distance, int &outThrottle, int &outSteering);

#endif