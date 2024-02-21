#include "httpReq.h"
#include <ArduinoHttpClient.h>

TinyGsmClient client(modem);
HttpClient http(client, server, port);

bool httpRequest(String &httpRequestData)
{

    debugln(F("\nPOST REQUEST DATA::"));
    debugln(httpRequestData);

    http.setTimeout(3000); // Set timeout to 3 seconds
    http.beginRequest();
    http.post(resource);
    http.sendHeader("Content-Type", "application/json");
    http.sendHeader("Content-Length", String(httpRequestData.length()));
    http.beginBody();
    http.print(httpRequestData);
    http.endRequest();
    int httpResponseCode = http.responseStatusCode();
    String resPayload = http.responseBody();
    http.stop();

    debugln("\nPOST REQUEST RESPONSE:");
    debugln(resPayload);
    if (httpResponseCode != 200)
    {
        debugln("\n[HTTP] POST...Failed, code:" + String(httpResponseCode));
        return false;
    }
    debugln("\n[HTTP] POST... Success, Code:" + String(httpResponseCode));
    return true;
}
