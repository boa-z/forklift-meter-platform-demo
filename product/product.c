#include "product/product.h"
#include "generated/demo_catalog.h"
#include "protocol/can/demo_pdo.h"
#include "services/settings_app.h"
#include "update/meter_update.h"
#ifdef METER_HEADLESS
#define PRODUCT_UI NULL
#else
#define PRODUCT_UI &meter_demo_ui
#endif
/** @brief Demo 无车辆控制命令；router 仅证明新签名（只选 owner，不触碰 TX）。 */
static bool demo_command_route(void *context, const meter_command_t *command,
                               meter_frame_route_owner_t *owner_out)
{
    (void)context;
    (void)command;
    (void)owner_out;
    return false;
}
/* 公开合成 Demo 参数为本机权威；真实远端参数由其 Product 排除持久化。 */
static const meter_storage_profile_t storage_profile = {true, 1u, 0x444Du, 3u, 500u, 3000u};
/* 公开合成台架仅由本机维护开关准入；真实 Product 必须增加驻车/车速策略。 */
static bool update_admission(const meter_snapshot_t *snapshot, bool maintenance)
{
    (void)snapshot;
    return maintenance;
}
static const meter_update_policy_t update_policy = {.product = "reference-demo",
                                                    .hardware = "reference-board",
                                                    .max_size = 4u * 1024u * 1024u + 2048u,
                                                    .timeout_ms = 15000u};
/**
 * @brief 公开合成协议的两条状态帧：0x3c0 是亮度（总是新鲜），0x2f0 是车速（VALID 且在量程内才新鲜）。
 * 布局：数值、fresh、滚动计数、快照 revision 低 16 位、反码及异或校验。
 */
static bool tx_encode(const meter_snapshot_t *snapshot, uint32_t now, meter_can_frame_t *frame)
{
    (void)now;
    static uint8_t counters[2];
    if (!snapshot || !frame)
        return false;
    const size_t index = frame->id == 0x3c0u ? 0u : 1u;
    uint16_t value = snapshot->brightness;
    bool fresh = true;
    if (index == 1u)
    {
        const meter_value_t speed = meter_snapshot_read(snapshot, METER_SPEED);
        fresh = speed.state == METER_VALUE_VALID && speed.value >= 0.0f && speed.value <= 655.0f;
        value = fresh ? (uint16_t)(speed.value * 100.0f) : 0u;
    }
    frame->size = 8u;
    frame->data[0] = (uint8_t)value;
    frame->data[1] = (uint8_t)(value >> 8);
    frame->data[2] = fresh ? 1u : 0u;
    frame->data[3] = counters[index]++;
    frame->data[4] = (uint8_t)snapshot->revision;
    frame->data[5] = (uint8_t)(snapshot->revision >> 8);
    frame->data[6] = (uint8_t)~frame->data[0];
    frame->data[7] = 0u;
    for (unsigned i = 0u; i < 7u; ++i)
        frame->data[7] ^= frame->data[i];
    return true;
}
/* 0x3c0 为关键帧：升级维护期间仍发送。 */
static const meter_periodic_frame_t periodic[] = {
    {.frame = {.bus = METER_BUS_CAN0, .id = 0x3c0u}, .period_ms = 50u, .critical = true, .encode = tx_encode},
    {.frame = {.bus = METER_BUS_CAN0, .id = 0x2f0u}, .period_ms = 100u, .encode = tx_encode},
    {.frame = {.bus = METER_BUS_CAN0, .id = DEMO_PDO_MOTION_ID}, .period_ms = 100u, .encode = demo_pdo_encode},
    {.frame = {.bus = METER_BUS_CAN0, .id = DEMO_PDO_STATUS_ID}, .period_ms = 100u, .encode = demo_pdo_encode}};
static const meter_product_t product = {.id = "reference-demo",
                                        .protocols = &meter_demo_protocols,
                                        .routes = &meter_demo_routes,
                                        .ui = PRODUCT_UI,
                                        .catalog = &meter_demo_catalog,
                                        .app_tick = meter_demo_evaluate,
                                        .command_route = demo_command_route,
                                        .storage = &storage_profile,
                                        .update = &update_policy,
                                        .update_admission = update_admission,
                                        .update_exclusive = true,
                                        .local_action = demo_settings_action,
                                        .app_reset = demo_settings_reset,
                                        .app_run = demo_settings_run,
                                        .periodic = periodic,
                                        .periodic_count = sizeof(periodic) / sizeof(periodic[0])};
const meter_product_t *meter_product_get(void)
{
    return &product;
}
