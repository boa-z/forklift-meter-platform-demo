#include "protocol/can/demo_pdo.h"
#include "generated/demo_catalog.h"
#include <math.h>
#include <string.h>

/* 量程内才取值；返回 false 表示该信号让整帧不新鲜。 */
static bool measurement(const meter_snapshot_t *snapshot, meter_signal_id_t id, float maximum, float scale,
                        uint16_t *out)
{
    const meter_value_t value = meter_snapshot_read(snapshot, id);
    if (value.state != METER_VALUE_VALID || !isfinite(value.value) || value.value < 0.0f || value.value > maximum)
        return false;
    *out = (uint16_t)(value.value * scale);
    return true;
}
static bool flag(const meter_snapshot_t *snapshot, meter_signal_id_t id, bool *out)
{
    const meter_value_t value = meter_snapshot_read(snapshot, id);
    if (value.state != METER_VALUE_VALID || (value.value != 0.0f && value.value != 1.0f))
        return false;
    *out = value.value != 0.0f;
    return true;
}
bool demo_pdo_encode(const meter_snapshot_t *snapshot, uint32_t now_ms, meter_can_frame_t *frame)
{
    (void)now_ms;
    static uint8_t motion_counter, status_counter;
    if (!snapshot || !frame || frame->bus != METER_BUS_CAN0 || frame->extended || frame->remote ||
        (frame->id != DEMO_PDO_MOTION_ID && frame->id != DEMO_PDO_STATUS_ID))
        return false;
    memset(frame->data, 0, sizeof(frame->data));
    frame->size = 8u;
    if (frame->id == DEMO_PDO_MOTION_ID)
    {
        uint16_t speed = 0u, height = 0u, load = 0u, soc = 0u;
        const bool fresh = measurement(snapshot, METER_SPEED, 655.0f, 100.0f, &speed) &&
                           measurement(snapshot, METER_HEIGHT, 6.0f, 1000.0f, &height) &&
                           measurement(snapshot, METER_LOAD, 1500.0f, 1.0f, &load) &&
                           measurement(snapshot, METER_SOC, 100.0f, 1.0f, &soc);
        if (fresh)
        {
            frame->data[0] = (uint8_t)speed;
            frame->data[1] = (uint8_t)(speed >> 8);
            frame->data[2] = (uint8_t)height;
            frame->data[3] = (uint8_t)(height >> 8);
            frame->data[4] = (uint8_t)load;
            frame->data[5] = (uint8_t)(load >> 8);
            frame->data[6] = (uint8_t)soc;
        }
        frame->data[7] = (uint8_t)(((motion_counter & 0x7fu) << 1) | (fresh ? 1u : 0u));
        motion_counter = (uint8_t)((motion_counter + 1u) & 0x7fu);
    }
    else
    {
        /* 位顺序与独立发送 DBC 一致；不把状态布尔值推断为整车安全结论。 */
        static const meter_signal_id_t flags[] = {METER_SEAT, METER_BRAKE, METER_NEUTRAL, METER_CHARGING,
                                                  METER_WARNING};
        bool fresh = true;
        uint8_t bits = 0u;
        for (unsigned i = 0u; i < 5u; ++i)
        {
            bool on = false;
            fresh = fresh && flag(snapshot, flags[i], &on);
            if (on)
                bits |= (uint8_t)(1u << i);
        }
        frame->data[0] = fresh ? bits : 0u;
        frame->data[1] = fresh ? 1u : 0u;
        frame->data[2] = status_counter++;
        frame->data[3] = 1u; /* 合成 PDO 布局版本。 */
        frame->data[4] = (uint8_t)snapshot->generation;
        frame->data[5] = (uint8_t)(snapshot->generation >> 8);
        frame->data[6] = (uint8_t)(snapshot->generation >> 16);
        frame->data[7] = (uint8_t)(snapshot->generation >> 24);
    }
    return true;
}
