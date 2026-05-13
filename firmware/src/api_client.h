#pragma once
#include <Arduino.h>

struct SessionDetail {
    int pid;
    int uptime_min;
};

struct ClaudeStatus {
    int quota_percent = -1;
    int sessions_active = 0;
    SessionDetail sessions[8];
    int reset_minutes = -1;
    char plan[16] = "unknown";
    bool valid = false;
    unsigned long fetch_time = 0;
};

class ApiClient {
public:
    void begin(const char* host, uint16_t port);
    bool fetch(ClaudeStatus& status);

private:
    char _url[128];
};
