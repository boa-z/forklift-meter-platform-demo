#include "services/settings_app.h"
#include "generated/demo_catalog.h"
#include "product/product.h"
#include <math.h>
#include <stddef.h>
static bool above(const hmi_snapshot_t *s, hmi_signal_id_t id, float limit)
{
    hmi_value_t value = hmi_snapshot_read(s, id);
    return value.state == HMI_VALUE_VALID && value.value > limit;
}
static bool below(const hmi_snapshot_t *s, hmi_signal_id_t id, float limit)
{
    hmi_value_t value = hmi_snapshot_read(s, id);
    return value.state == HMI_VALUE_VALID && value.value < limit;
}
static bool any_state(const hmi_snapshot_t *s, hmi_value_state_t state)
{
    for (size_t i = 0; i < s->catalog->signal_count; ++i)
        if (s->signals[i].state == state)
            return true;
    return false;
}
static bool threshold(const hmi_snapshot_t *s, uint16_t parameter_id, float *out)
{
    return hmi_snapshot_parameter(s, parameter_id, out);
}
void meter_demo_evaluate(hmi_snapshot_t *s, uint32_t now_ms)
{
    (void)now_ms;
    demo_settings_publish(s);
    float limit = 0;
    hmi_snapshot_fault_set(s, DEMO_FAULT_LOW_CHARGE,
                             threshold(s, DEMO_PARAMETER_BATTERY_WARNING_LEVEL, &limit) &&
                                 below(s, METER_SOC, limit));
    hmi_snapshot_fault_set(s, DEMO_FAULT_OVERTEMPERATURE,
                             threshold(s, DEMO_PARAMETER_MOTOR_TEMP_ADVISORY, &limit) &&
                                 above(s, METER_MOTOR_TEMP, limit));
    hmi_snapshot_fault_set(s, DEMO_FAULT_LOW_VOLTAGE, below(s, METER_BATTERY_VOLTAGE, 40));
    hmi_snapshot_fault_set(s, DEMO_FAULT_SENSOR_FAULT, any_state(s, HMI_VALUE_ERROR));
    hmi_snapshot_fault_set(s, DEMO_FAULT_COMMUNICATION_FAULT,
                             any_state(s, HMI_VALUE_STALE) || !s->connected);
    hmi_snapshot_fault_set(s, DEMO_FAULT_LOAD_ADVISORY,
                             threshold(s, DEMO_PARAMETER_LOAD_ADVISORY, &limit) &&
                                 above(s, METER_LOAD, limit));
    hmi_snapshot_fault_set(s, DEMO_FAULT_HEIGHT_ADVISORY,
                             threshold(s, DEMO_PARAMETER_LIFT_LIMIT, &limit) &&
                                 above(s, METER_HEIGHT, limit));
    hmi_snapshot_fault_set(s, DEMO_FAULT_CONTROLLER_HOT,
                             threshold(s, DEMO_PARAMETER_CONTROLLER_TEMP_ADVISORY, &limit) &&
                                 above(s, METER_CONTROLLER_TEMP, limit));
    hmi_snapshot_fault_set(s, DEMO_FAULT_GENERIC_WARNING, above(s, METER_WARNING, 0));
    hmi_value_t steering = hmi_snapshot_read(s, METER_STEERING);
    float degree = 0;
    hmi_snapshot_fault_set(s, DEMO_FAULT_STEERING_ADVISORY,
                             steering.state == HMI_VALUE_VALID &&
                                 threshold(s, DEMO_PARAMETER_STEERING_LIMIT, &degree) &&
                                 fabsf(steering.value) > degree);
}
