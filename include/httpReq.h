#ifndef HTTPSREQ_H
#define HTTPSREQ_H

#include <TinyGsmClient.h>
#include "config.h"

// Layers stack
extern TinyGsm modem;

bool httpRequest(String &httpRequestData);

#endif