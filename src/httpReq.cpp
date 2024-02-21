#include "httpReq.h"
#include "azParams.h"

const char *server_url = "https://" IOT_HOSTNAME "/devices/" DEVICE_ID "/messages/events?api-version=2020-03-13";

bool httpRequest(String &httpRequestData)
{

    // Initialize HTTPS
    modem.https_begin();

    // Set GET URL
    if (!modem.https_set_url(server_url))
    {
        Serial.println("Failed to set the URL. Please check the validity of the URL!");
        return false;
    }

    modem.https_add_header("Authorization", SAS_TOKEN);
    modem.https_add_header("Accept-Encoding", "gzip, deflate, br");
    modem.https_set_accept_type("application/json");
    modem.https_set_user_agent("TinyGSM/LilyGo-A7670");

    int httpCode = modem.https_post(httpRequestData);

    /*Printing once the HTTPS request is completed*/
    Serial.println(F("\nPOST REQUEST DATA::"));
    Serial.println(httpRequestData);

    if (httpCode < 200 || httpCode >= 300)
    {
        Serial.print("\n[HTTP] POST...Failed, code:" + String(httpCode));
        return false;
    }
    Serial.println("\n[HTTP] POST... Success, Code:" + String(httpCode));

    /*Get HTTPS header information*/
    // String header = modem.https_header();
    // Serial.print("\nHTTP Header : ");
    // Serial.println(header);

    /*Get HTTPS response*/
    // String body = modem.https_body();
    // Serial.print("\nHTTP body : ");
    // Serial.println(body);

    return true;
}
