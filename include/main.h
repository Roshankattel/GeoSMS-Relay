#ifndef MAIN_H
#define MAIN_H

#define TINY_GSM_RX_BUFFER 2048 // Set RX buffer to 2Kb
#include <ArduinoJson.h>
#include "httpReq.h"

// Set serial for AT commands (to SIM module)
#define SerialAT Serial1

// Define the serial console for debug prints, if needed
// #define DUMP_AT_COMMANDS
#ifdef DUMP_AT_COMMANDS
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

QueueHandle_t smsDataQueue;
uint64_t msgCount = 0;

#endif