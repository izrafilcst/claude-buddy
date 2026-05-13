#include "api_client.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

void ApiClient::begin(const char* host, uint16_t port) {
    snprintf(_url, sizeof(_url), "http://%s:%d/status", host, port);
}

bool ApiClient::fetch(ClaudeStatus& status) {
    if (WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    http.setTimeout(5000);
    if (!http.begin(_url)) {
        http.end();
        return false;
    }

    int code = http.GET();
    if (code != 200) {
        http.end();
        status.valid = false;
        return false;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    if (deserializeJson(doc, payload)) {
        return false;
    }

    status.quota_percent = doc["quota_percent"] | -1;
    status.sessions_active = doc["sessions_active"] | 0;
    status.reset_minutes = doc["reset_minutes"] | -1;

    const char* plan = doc["plan"] | "unknown";
    strncpy(status.plan, plan, sizeof(status.plan) - 1);
    status.plan[sizeof(status.plan) - 1] = '\0';

    JsonArray details = doc["sessions_details"];
    int i = 0;
    for (JsonObject s : details) {
        if (i >= 8) break;
        status.sessions[i].pid = s["pid"] | 0;
        status.sessions[i].uptime_min = s["uptime_min"] | 0;
        i++;
    }

    status.valid = true;
    status.fetch_time = millis();
    return true;
}
