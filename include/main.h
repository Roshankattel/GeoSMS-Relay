#ifndef MAIN_H
#define MAIN_H

#include "httpReq.h"

// Set serial for AT commands (to SIM module)
#define SerialAT Serial1

#ifdef DUMP_AT_COMMANDS // if enabled it requires the streamDebugger lib
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

struct SmsData
{
    char sender[32];
    char timestamp[32];
    char message[512];
    uint64_t msgId;
};

SmsData receivedData;

QueueHandle_t smsDataQueue;
uint64_t msgCount = 0;
uint64_t lastReceivedTime = 0;
bool sendData = false;

#endif