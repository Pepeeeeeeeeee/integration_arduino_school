#ifndef APIClient_h
#define APIClient_h

#include <HTTPClient.h>
#include <esp_system.h>


struct APIClient{
    String wifiSsid;
    String wifiPass;
    String apiUrl;
    String apiKey;

    String deviceUid;

    uint32_t wifiConnectTimeoutMs;
};

struct Measurement {
    float temperatureC;
    float humidityPct;
    float lightLux;
    float noiseDb;
    time_t timestamp;
};


void APIClient_init(APIClient client);
void APIClient_setDeviceUID(String uid);

APIClient* getAPIClient();

bool sendMeasurementOnce(Measurement* measurement);
void printMacAddress();

#endif