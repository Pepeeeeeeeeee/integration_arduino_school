#include "APIClient.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_system.h>

static APIClient apiClient;


APIClient* getAPIClient() {
    return &apiClient;
}

static void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.mode(WIFI_STA);
  WiFi.begin(apiClient.wifiSsid.c_str(), apiClient.wifiPass.c_str());
  Serial.print("[WiFi] Connecting");
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < apiClient.wifiConnectTimeoutMs) {
    delay(300);
    Serial.print(".");
  }
  wl_status_t st = WiFi.status();
  Serial.printf("\n[WiFi] status=%d ip=%s\n", (int)st, WiFi.localIP().toString().c_str());
}

void APIClient_init(APIClient client) {
    apiClient = client;
    ensureWiFi();
}


void APIClient_setDeviceUID(String uid) {
    apiClient.deviceUid = uid;
}

static String httpErrStr(int code) {
  if (code >= 0) return String(code);
  switch (code) {
    case HTTPC_ERROR_CONNECTION_REFUSED: return "CONNECTION_REFUSED";
    case HTTPC_ERROR_SEND_HEADER_FAILED: return "SEND_HEADER_FAILED";
    case HTTPC_ERROR_SEND_PAYLOAD_FAILED:return "SEND_PAYLOAD_FAILED";
    case HTTPC_ERROR_NOT_CONNECTED:      return "NOT_CONNECTED";
    case HTTPC_ERROR_CONNECTION_LOST:    return "CONNECTION_LOST";
    case HTTPC_ERROR_NO_HTTP_SERVER:     return "NO_HTTP_SERVER";
    case HTTPC_ERROR_READ_TIMEOUT:       return "READ_TIMEOUT";
    default:                             return String("UNKNOWN(") + code + ")";
  }
}

static String buildPayload(Measurement* measurement) {
    return String("{") +
        "\"device_name\":\"" + apiClient.deviceUid + "\"," +
        "\"temperature\":"   + String(measurement->temperatureC, 1) + "," +
        "\"humidity\":"  + String(measurement->humidityPct, 1) + "," +
        "\"luminosity\":"      + String(measurement->lightLux, 1) + "," +
        "\"noise_levels\":" + String(measurement->noiseDb, 1) +
    "}";
}

bool sendMeasurementOnce(Measurement* measurement) {
  ensureWiFi();

  String payload = buildPayload(measurement);

  HTTPClient http;
  Serial.printf("[HTTP] POST %s\n", apiClient.apiUrl.c_str());
  if (!http.begin(apiClient.apiUrl.c_str())) {
    Serial.println("[HTTP] begin() failed");
    return false;
  }
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Key", apiClient.apiKey.c_str());

  int code = http.POST(payload);
  Serial.printf("[HTTP] -> %s\n", httpErrStr(code).c_str());

  if (code > 0) {
    String body = http.getString();
    Serial.printf("[HTTP] Body: %s\n", body.c_str());
  }
  http.end();

  // Success is any 2xx or 3xx (adjust if you only expect 200)
  return code >= 200 && code < 400;
}

void printMacAddress() {
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
}