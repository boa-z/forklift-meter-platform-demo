#ifndef METER_SYNTHETIC_H
#define METER_SYNTHETIC_H
#include "runtime/hmi_runtime.h"
typedef enum
{
    DEMO_NORMAL,
    DEMO_WARNING,
    DEMO_STALE,
    DEMO_OFFLINE,
    DEMO_ERROR,
    DEMO_UNKNOWN
} demo_scenario_t;
void meter_synthetic_step(hmi_runtime_t *runtime, uint32_t now_ms, demo_scenario_t scenario);
void meter_synthetic_values(hmi_runtime_t *runtime, uint32_t now_ms, float speed, float soc,
                            float steering);
#endif
