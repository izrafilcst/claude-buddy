#pragma once
#ifdef DEBUG_MODE
#include <string.h>
#include "api_client.h"

#ifndef DEBUG_INTERVAL_MS
#define DEBUG_INTERVAL_MS 10000
#endif

struct DebugData {
    const char* label;
    int  quota_percent;
    int  sessions_active;
    int  pids[3];
    int  uptimes[3];
    int  reset_minutes;
    const char* plan;
    bool valid;
};

static const DebugData DEBUG_DATA[] = {
    { "RADIANT",   10, 1, {1001, 0,    0   }, {45,  0,  0}, 320, "pro",     true  },
    { "CONTENT",   40, 2, {1001, 1002, 0   }, {120, 20, 0}, 200, "pro",     true  },
    { "NORMAL",    58, 1, {1003, 0,    0   }, {90,  0,  0}, 130, "pro",     true  },
    { "TIRED",     72, 1, {1004, 0,    0   }, {240, 0,  0},  80, "pro",     true  },
    { "EXHAUSTED", 88, 3, {1005, 1006, 1007}, {480, 60, 15}, 20, "pro",     true  },
    { "OFFLINE",   -1, 0, {0,    0,    0   }, {0,   0,  0},  -1, "unknown", true  },
    { "CONFUSED",  -1, 0, {0,    0,    0   }, {0,   0,  0},  -1, "unknown", false },
};

static const int DEBUG_SCENARIO_COUNT =
    (int)(sizeof(DEBUG_DATA) / sizeof(DEBUG_DATA[0]));

inline ClaudeStatus debugBuildStatus(int idx) {
    const DebugData& d = DEBUG_DATA[idx];
    ClaudeStatus s;
    s.quota_percent   = d.quota_percent;
    s.sessions_active = d.sessions_active;
    s.reset_minutes   = d.reset_minutes;
    s.valid           = d.valid;
    s.fetch_time      = 0;
    strncpy(s.plan, d.plan, sizeof(s.plan) - 1);
    s.plan[sizeof(s.plan) - 1] = '\0';
    int n = d.sessions_active < 3 ? d.sessions_active : 3;
    for (int i = 0; i < n; i++) {
        s.sessions[i].pid        = d.pids[i];
        s.sessions[i].uptime_min = d.uptimes[i];
    }
    return s;
}

#endif // DEBUG_MODE
