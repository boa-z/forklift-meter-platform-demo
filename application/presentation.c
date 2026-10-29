#include "application/presentation.h"
#include "hmi_build_identity.h"
#include <stdio.h>
#ifdef HMI_ENABLE_CAN_UPDATE
#include "hmi_update_build.h"
#endif
/* 占位符与 "HH:MM" 同长，所以时钟盒在可用与不可用之间切换时不会重排顶栏。 */
static const char clock_placeholder[] = "--:--";

bool demo_clock_text(char *out, size_t size)
{
    if (!out || size < sizeof(clock_placeholder))
        return false;
    hmi_wall_time_t utc, local;
    /* 未绑定来源、来源失败或采样越界都落在这里：显示占位符而不是 1970 或 00:00。 */
    if (!hmi_wall_clock_read(&utc) || !hmi_wall_time_shift(&utc, DEMO_UTC_OFFSET_SECONDS, &local))
    {
        for (size_t i = 0; i < sizeof(clock_placeholder); ++i)
            out[i] = clock_placeholder[i];
        return false;
    }
    /* 24 小时制：不引入 am/pm 文案，因此不需要额外的可翻译字符串或字体子集。 */
    snprintf(out, size, "%02u:%02u", (unsigned)local.hour, (unsigned)local.minute);
    return true;
}

bool demo_clock_fields(hmi_wall_time_t *local_out)
{
    if (!local_out)
        return false;
    hmi_wall_time_t utc, local;
    if (hmi_wall_clock_read(&utc) && hmi_wall_time_shift(&utc, DEMO_UTC_OFFSET_SECONDS, &local))
    {
        *local_out = local;
        return true;
    }
    /* 未对时的板子也要有一个可编辑的起点，否则对时页永远无法把时钟设起来。 */
    *local_out = (hmi_wall_time_t){(uint16_t)DEMO_CLOCK_BASE_YEAR, (uint8_t)DEMO_CLOCK_BASE_MONTH,
                                     (uint8_t)DEMO_CLOCK_BASE_DAY, 0u, 0u, 0u, true};
    return false;
}
static demo_readout_t readout(const hmi_snapshot_t *snapshot, hmi_signal_id_t id)
{
    hmi_value_t value = hmi_snapshot_read(snapshot, id);
    return (demo_readout_t){value.value, value.state};
}
void demo_presentation_build(const hmi_snapshot_t *s, demo_presentation_t *out)
{
    demo_presentation_t view = {0};
#ifdef HMI_ENABLE_CAN_UPDATE
    view.firmware_version = HMI_UPDATE_FIRMWARE_VERSION;
#endif
    view.framework_revision = HMI_BUILD_PLATFORM;
    view.revision = s->revision;
    view.profile = s->profile;
    view.language = s->language;
    view.imperial = s->imperial;
    view.brightness = s->brightness;
    view.limit_available = hmi_snapshot_parameter(s, DEMO_PARAMETER_MAX_SPEED, &view.limit);
    view.speed = readout(s, METER_SPEED);
    view.steering = readout(s, METER_STEERING);
    view.soc = readout(s, METER_SOC);
    view.height = readout(s, METER_HEIGHT);
    view.load = readout(s, METER_LOAD);
    view.hours = readout(s, METER_WORK_HOURS);
    /* 里程是公开 Demo 的合成展示值，真实 Product 应从自己的目录映射。 */
    view.mileage = (demo_readout_t){1286.4f, HMI_VALUE_VALID};
    view.speed.value *= s->imperial ? 0.621371f : 1.0f;
    view.speed_maximum = s->imperial ? 32.0f : 50.0f;
    view.speed_unit = s->imperial ? "mph" : "km/h";
    const hmi_signal_id_t flags[] = {METER_SEAT, METER_BRAKE, METER_NEUTRAL, METER_CHARGING, METER_WARNING};
    for (size_t i = 0; i < 5; ++i)
        view.status[i] = readout(s, flags[i]);
    bool stale = false;
    for (size_t i = 0; i < s->catalog->signal_count; ++i)
        if (s->signals[i].state == HMI_VALUE_STALE)
            stale = true;
    view.link = !s->connected                             ? DEMO_LINK_OFFLINE
                : stale                                   ? DEMO_LINK_STALE
                : view.speed.state == HMI_VALUE_UNKNOWN ? DEMO_LINK_WAITING
                                                          : DEMO_LINK_CONNECTED;
    for (size_t i = 0; i < DEMO_MONITOR_SLOTS; ++i)
    {
        const hmi_monitor_def_t *definition = &meter_demo_catalog.monitors[i];
        view.monitors[i].reading = readout(s, definition->signal);
        view.monitors[i].unit = definition->unit;
    }
    const hmi_signal_id_t remote[] = {DEMO_REMOTE_SPEED, DEMO_REMOTE_RAMP, DEMO_REMOTE_LIFT, DEMO_REMOTE_REGEN};
    for (size_t i = 0; i < DEMO_REMOTE_COUNT; ++i)
    {
        hmi_value_t value = hmi_snapshot_read(s, remote[i]);
        view.parameters[i] = (demo_parameter_view_t){value.value, value.state == HMI_VALUE_VALID};
    }
    hmi_value_t access = hmi_snapshot_read(s, DEMO_ACCESS_ROLE);
    view.user_authorized = access.state == HMI_VALUE_VALID && access.value >= 1;
    view.admin_authorized = access.state == HMI_VALUE_VALID && access.value == 2;
    view.auth_feedback = (demo_feedback_t)hmi_snapshot_read(s, DEMO_ACCESS_RESULT).value;
    view.parameter_feedback = (demo_feedback_t)hmi_snapshot_read(s, DEMO_REMOTE_RESULT).value;
    const hmi_signal_id_t admin[] = {DEMO_ADMIN_CAN, DEMO_ADMIN_HOURS, DEMO_ADMIN_SPEED, DEMO_ADMIN_MEMORY};
    for (size_t i = 0; i < DEMO_ADMIN_COUNT; ++i)
        view.admin_values[i] = (unsigned)hmi_snapshot_read(s, admin[i]).value;
    for (size_t i = 0; i < DEMO_FAULT_SLOTS; ++i)
    {
        view.faults[i].code = meter_demo_catalog.faults[i].id;
        view.faults[i].active = hmi_snapshot_fault_active(s, view.faults[i].code);
    }
    *out = view;
}
hmi_action_t demo_speed_limit_intent(float value)
{
    return (hmi_action_t){HMI_ACTION_PARAMETER, DEMO_PARAMETER_MAX_SPEED, value};
}

hmi_action_t demo_settings_intent(demo_settings_intent_t intent, float value)
{
    return (hmi_action_t){HMI_ACTION_PRODUCT, (uint16_t)intent, value};
}
hmi_action_t demo_remote_intent(unsigned row, float value)
{
    return demo_settings_intent((demo_settings_intent_t)(DEMO_INTENT_REMOTE_FIRST + row), value);
}
hmi_action_t demo_admin_intent(unsigned row, float value)
{
    return demo_settings_intent((demo_settings_intent_t)(DEMO_INTENT_ADMIN_FIRST + row), value);
}
hmi_action_t demo_clock_intent(demo_clock_field_t field, float value)
{
    return demo_settings_intent((demo_settings_intent_t)(DEMO_INTENT_CLOCK_FIRST + (unsigned)field), value);
}
