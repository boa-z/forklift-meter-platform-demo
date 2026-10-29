#include "core/hmi_core.h"
#include "core/hmi_settings.h"
#include "generated/demo_catalog.h"
#include "protocol/demo_protocol.h"
#include "protocols/common/hmi_frame_router.h"
#include "runtime/hmi_runtime.h"
#include "sim/synthetic.h"
#include "ui/widgets/meter_format.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x)                                                                                             \
    do                                                                                                       \
    {                                                                                                        \
        if (!(x))                                                                                            \
        {                                                                                                    \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);                                          \
            return 1;                                                                                        \
        }                                                                                                    \
    } while (0)
#define SLOTS(array) (sizeof(array) / sizeof((array)[0]))
/* 平台从不持有域内存：每个调用方绑定的数组按自己产品目录的大小准备。 */
static hmi_value_t signal_slots[DEMO_SIGNAL_SLOTS];
static float parameter_slots[DEMO_PARAMETER_SLOTS];
static hmi_fault_state_t fault_slots[DEMO_FAULT_SLOTS];
static const hmi_signal_def_t duplicate_signals[] = {{.id = METER_SPEED, .key = "speed"}, {.id = METER_SPEED, .key = "flow"}};
static const hmi_signal_def_t anonymous_signals[] = {{.id = 0, .key = "gap"}};
static const hmi_monitor_def_t dangling_monitor[] = {{"Gap", "gap", METER_WARNING + 100}};
static const hmi_fault_def_t duplicate_faults[] = {{DEMO_FAULT_LOW_CHARGE, "one", "first"},
                                                     {DEMO_FAULT_LOW_CHARGE, "two", "second"}};
static bool keep_first_source(void *context, hmi_signal_id_t signal, const hmi_value_t *incoming,
                              const hmi_value_t *current)
{
    (void)context;
    (void)signal;
    return current->source == HMI_SOURCE_NONE || incoming->source == current->source;
}
static hmi_core_storage_t storage(void)
{
    return (hmi_core_storage_t){signal_slots,           SLOTS(signal_slots), parameter_slots,
                                  SLOTS(parameter_slots), fault_slots,         SLOTS(fault_slots)};
}
/* 设置块是公开文档化的格式，所以测试可以直接伪造一个合法文件，
 * 而不是只破坏字节直到校验和报错。 */
static void store_float(uint8_t *p, float value)
{
    uint32_t bits;
    memcpy(&bits, &value, 4);
    for (unsigned i = 0; i < 4; ++i)
        p[i] = (uint8_t)(bits >> (8 * i));
}
static void seal(uint8_t *bytes, size_t size)
{
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i + 4 < size; ++i)
        hash = (hash ^ bytes[i]) * 16777619u;
    for (unsigned i = 0; i < 4; ++i)
        bytes[size - 4 + i] = (uint8_t)(hash >> (8 * i));
}
static const hmi_frame_route_t routes[] = {{HMI_BUS_CAN0, 0x100, false, 1},
                                             {HMI_BUS_CAN1, 0x100, false, 2},
                                             {HMI_BUS_CAN0, 0x100, true, 2},
                                             {HMI_BUS_CAN1, 0x1abc123, true, 2}};
static const hmi_route_profile_t route_profile = {routes, 4};
static bool noop(const hmi_can_frame_t *f, hmi_update_sink_t sink, void *context)
{
    (void)f;
    (void)sink;
    (void)context;
    return true;
}
static const hmi_protocol_binding_t bindings[] = {{1, meter_demo_decode}, {2, noop}};
static const hmi_protocol_profile_t protocol_profile = {bindings, 2};
static const hmi_product_t product = {
    .id = "test", .protocols = &protocol_profile, .routes = &route_profile, .catalog = &meter_demo_catalog};
static hmi_can_frame_t frame(void)
{
    hmi_can_frame_t f = {HMI_BUS_CAN0, 0x100, 1, false, false, 8, {0xc4, 0x09, 0, 0, 0, 0, 0, 0}};
    return f;
}
static int contracts(void)
{
    CHECK(hmi_routes_valid(&route_profile));
    hmi_can_frame_t f = frame();
    CHECK(hmi_route_lookup(&route_profile, &f)->owner == 1);
    f.bus = HMI_BUS_CAN1;
    CHECK(hmi_route_lookup(&route_profile, &f)->owner == 2);
    f.bus = HMI_BUS_CAN0;
    f.extended = true;
    CHECK(hmi_route_lookup(&route_profile, &f)->owner == 2);
    f.bus = HMI_BUS_CAN1;
    f.id = 0x1abc123;
    CHECK(hmi_route_lookup(&route_profile, &f)->owner == 2);
    f.extended = false;
    CHECK(!hmi_frame_valid(&f));
    f = frame();
    f.size = 9;
    CHECK(!hmi_frame_valid(&f));
    f = frame();
    f.remote = true;
    CHECK(!hmi_frame_valid(&f));
    f = frame();
    f.bus = (hmi_bus_role_t)99;
    CHECK(!hmi_frame_valid(&f));
    hmi_frame_route_t duplicate[2] = {routes[0], routes[0]};
    hmi_route_profile_t bad = {duplicate, 2};
    CHECK(!hmi_routes_valid(&bad));
    return 0;
}
static int core(void)
{
    hmi_core_t c;
    hmi_core_storage_t bound = storage();
    CHECK(hmi_core_init(&c, &meter_demo_catalog, &bound));
    CHECK(hmi_snapshot_read(&c.snapshot, METER_SPEED).state == HMI_VALUE_UNKNOWN);
    CHECK(c.snapshot.revision == 0);
    hmi_update_t u = {METER_SPEED, {12.5f, 100, HMI_VALUE_VALID}};
    CHECK(hmi_core_apply(&c, &u));
    CHECK(hmi_snapshot_read(&c.snapshot, METER_SPEED).source == HMI_SOURCE_NONE);
    CHECK(c.snapshot.revision == 1);
    CHECK(hmi_core_apply(&c, &u));
    CHECK(c.snapshot.revision == 1);
    u.signal = METER_SOC;
    u.value.timestamp_ms = 100;
    CHECK(hmi_core_apply(&c, &u));
    u.signal = METER_SPEED;
    hmi_core_tick(&c, 849);
    CHECK(hmi_snapshot_read(&c.snapshot, METER_SPEED).state == HMI_VALUE_VALID);
    hmi_core_tick(&c, 850);
    CHECK(hmi_snapshot_read(&c.snapshot, METER_SPEED).state == HMI_VALUE_STALE);
    CHECK(c.snapshot.revision == 3);
    CHECK(hmi_snapshot_read(&c.snapshot, METER_SPEED).value == 12.5f);
    CHECK(hmi_snapshot_read(&c.snapshot, METER_SOC).state == HMI_VALUE_VALID);
    hmi_catalog_t policy_catalog = meter_demo_catalog;
    policy_catalog.source_policy = keep_first_source;
    CHECK(hmi_core_init(&c, &policy_catalog, &bound));
    u = (hmi_update_t){METER_SPEED, {1, 0, HMI_VALUE_VALID, 7}};
    CHECK(hmi_core_apply(&c, &u));
    u.value.source = 8;
    CHECK(!hmi_core_apply(&c, &u));
    CHECK(hmi_snapshot_read(&c.snapshot, METER_SPEED).source == 7);
    CHECK(hmi_core_init(&c, &meter_demo_catalog, &bound));
    u.value.timestamp_ms = UINT32_MAX - 49;
    CHECK(hmi_core_apply(&c, &u));
    hmi_core_tick(&c, 750);
    CHECK(hmi_snapshot_read(&c.snapshot, METER_SPEED).state == HMI_VALUE_STALE);
    u.value.value = NAN;
    CHECK(hmi_core_apply(&c, &u));
    CHECK(hmi_snapshot_read(&c.snapshot, METER_SPEED).state == HMI_VALUE_ERROR);
    /* 目录从未声明的身份代表未知数据，不是槽位号：在旧模型下 999
     * 恰好是平台定长数组的合法下标。 */
    u.signal = (hmi_signal_id_t)999;
    CHECK(!hmi_core_apply(&c, &u));
    u.signal = HMI_ID_PRIVATE_FIRST;
    CHECK(!hmi_core_apply(&c, &u));
    CHECK(hmi_snapshot_read(&c.snapshot, HMI_ID_PRIVATE_FIRST).state == HMI_VALUE_UNKNOWN);
    CHECK(hmi_core_parameter(&c, DEMO_PARAMETER_MAX_SPEED, 50));
    uint32_t revision = c.snapshot.revision;
    CHECK(hmi_core_parameter(&c, DEMO_PARAMETER_MAX_SPEED, 50));
    CHECK(c.snapshot.revision == revision);
    CHECK(!hmi_core_parameter(&c, DEMO_PARAMETER_MAX_SPEED, 51));
    CHECK(!hmi_core_parameter(&c, DEMO_PARAMETER_MAX_SPEED, NAN));
    CHECK(!hmi_core_parameter(&c, 999, 10));
    /* 故障状态用同样的方式寻址，产品无法触发自己不存在的条目。 */
    CHECK(hmi_snapshot_fault_set(&c.snapshot, DEMO_FAULT_LOW_CHARGE, true));
    revision = c.snapshot.revision;
    CHECK(hmi_snapshot_fault_set(&c.snapshot, DEMO_FAULT_LOW_CHARGE, true));
    CHECK(c.snapshot.revision == revision);
    CHECK(hmi_snapshot_fault_active(&c.snapshot, DEMO_FAULT_LOW_CHARGE));
    CHECK(!hmi_snapshot_fault_set(&c.snapshot, 999, true));
    /* 存储小于目录规模属于集成错误，必须在写入任何槽位之前被拦下。 */
    hmi_core_storage_t tight = storage();
    tight.signal_capacity = meter_demo_catalog.signal_count - 1;
    CHECK(!hmi_core_init(&c, &meter_demo_catalog, &tight));
    tight = storage();
    tight.fault_capacity = meter_demo_catalog.fault_count - 1;
    CHECK(!hmi_core_init(&c, &meter_demo_catalog, &tight));
    tight = storage();
    tight.parameters = NULL;
    CHECK(!hmi_core_init(&c, &meter_demo_catalog, &tight));
    /* 监控项只是展示信息，因此没有监控项的产品依然拥有合法的域。 */
    hmi_catalog_t monitorless = meter_demo_catalog;
    monitorless.monitors = NULL;
    monitorless.monitor_count = 0;
    CHECK(hmi_core_init(&c, &monitorless, &bound));
    hmi_catalog_t ambiguous = meter_demo_catalog;
    ambiguous.signals = duplicate_signals;
    ambiguous.signal_count = SLOTS(duplicate_signals);
    ambiguous.monitors = NULL;
    ambiguous.monitor_count = 0;
    CHECK(!hmi_core_init(&c, &ambiguous, &bound));
    ambiguous.signals = anonymous_signals;
    ambiguous.signal_count = SLOTS(anonymous_signals);
    CHECK(!hmi_core_init(&c, &ambiguous, &bound));
    hmi_catalog_t unresolved = meter_demo_catalog;
    unresolved.monitors = dangling_monitor;
    unresolved.monitor_count = SLOTS(dangling_monitor);
    CHECK(!hmi_core_init(&c, &unresolved, &bound));
    hmi_catalog_t shared = meter_demo_catalog;
    shared.faults = duplicate_faults;
    shared.fault_count = SLOTS(duplicate_faults);
    CHECK(!hmi_core_init(&c, &shared, &bound));
    return 0;
}
static int runtime(void)
{
    hmi_core_t c;
    hmi_runtime_t r;
    hmi_core_storage_t bound = storage();
    CHECK(hmi_core_init(&c, &meter_demo_catalog, &bound));
    CHECK(hmi_runtime_init(&r, &product, hmi_core_apply, &c));
    hmi_can_frame_t f = frame();
    CHECK(!hmi_runtime_receive(&r, &f));
    hmi_runtime_connection(&r, true);
    CHECK(hmi_runtime_receive(&r, &f));
    CHECK(hmi_snapshot_read(&c.snapshot, METER_SPEED).value == 25);
    uint32_t generation = r.generation;
    hmi_runtime_connection(&r, false);
    CHECK(r.generation == generation + 1);
    hmi_runtime_connection(&r, true);
    f.id = 0x777;
    CHECK(!hmi_runtime_receive(&r, &f));
    CHECK(r.diagnostics.unrouted == 1);
    f = frame();
    f.size = 7;
    CHECK(!hmi_runtime_receive(&r, &f));
    CHECK(r.diagnostics.decode_failed == 1);
    f = frame();
    f.extended = true;
    f.id = 0x20000000;
    CHECK(!hmi_runtime_receive(&r, &f));
    CHECK(r.diagnostics.malformed == 1);
    CHECK(!hmi_runtime_receive(&r, NULL));
    CHECK(r.diagnostics.malformed == 2);
    hmi_product_t bad = product;
    hmi_protocol_binding_t dup[2] = {bindings[0], bindings[0]};
    hmi_protocol_profile_t pp = {dup, 2};
    bad.protocols = &pp;
    CHECK(!hmi_runtime_init(&r, &bad, hmi_core_apply, &c));
    return 0;
}
static int protocol(void)
{
    hmi_core_t c;
    hmi_core_storage_t bound = storage();
    CHECK(hmi_core_init(&c, &meter_demo_catalog, &bound));
    hmi_can_frame_t f = frame();
    CHECK(meter_demo_decode(&f, hmi_core_apply, &c));
    CHECK(hmi_snapshot_read(&c.snapshot, METER_SPEED).value == 25);
    f.data[2] = 0x6c;
    f.data[3] = 0xee;
    CHECK(meter_demo_decode(&f, hmi_core_apply, &c));
    CHECK(hmi_snapshot_read(&c.snapshot, METER_STEERING).value == -45);
    f.id = 0x101;
    f.data[0] = 101;
    CHECK(meter_demo_decode(&f, hmi_core_apply, &c));
    CHECK(hmi_snapshot_read(&c.snapshot, METER_SOC).state == HMI_VALUE_ERROR);
    f.data[0] = 100;
    CHECK(meter_demo_decode(&f, hmi_core_apply, &c));
    CHECK(hmi_snapshot_read(&c.snapshot, METER_SOC).state == HMI_VALUE_VALID);
    f.id = 0x102;
    f.data[0] = 0xff;
    f.data[1] = 0xff;
    CHECK(meter_demo_decode(&f, hmi_core_apply, &c));
    CHECK(hmi_snapshot_read(&c.snapshot, METER_HEIGHT).state == HMI_VALUE_ERROR);
    f = frame();
    f.size = 2;
    CHECK(!meter_demo_decode(&f, hmi_core_apply, &c));
    f = frame();
    f.bus = HMI_BUS_CAN1;
    CHECK(!meter_demo_decode(&f, hmi_core_apply, &c));
    f = frame();
    f.extended = true;
    CHECK(!meter_demo_decode(&f, hmi_core_apply, &c));
    return 0;
}
static int widgets(void)
{
    float a = 0;
    CHECK(meter_gauge_map(0, 0, 50, 135, 405, &a) && a == 135);
    CHECK(meter_gauge_map(25, 0, 50, 135, 405, &a) && a == 270);
    CHECK(meter_gauge_map(50, 0, 50, 135, 405, &a) && a == 405);
    CHECK(meter_gauge_map(500, 0, 50, 135, 405, &a) && a == 405);
    CHECK(meter_gauge_map(-5, 0, 50, 135, 405, &a) && a == 135);
    CHECK(!meter_gauge_map(NAN, 0, 50, 0, 180, &a));
    CHECK(!meter_gauge_map(1, 1, 1, 0, 180, &a));
    CHECK(meter_gauge_map(-45, -45, 45, 225, 315, &a) && a == 225);
    CHECK(meter_gauge_map(0, -45, 45, 225, 315, &a) && a == 270);
    CHECK(meter_gauge_map(45, -45, 45, 225, 315, &a) && a == 315);
    CHECK(meter_threshold_band(19, 20, 40) == 0);
    CHECK(meter_threshold_band(20, 20, 40) == 1);
    CHECK(meter_threshold_band(40, 20, 40) == 2);
    char text[64];
    meter_format_value(text, sizeof(text), 12.5f, HMI_VALUE_UNKNOWN, "km/h", 1);
    CHECK(!strcmp(text, "-- km/h"));
    meter_format_value(text, sizeof(text), 12.5f, HMI_VALUE_STALE, "km/h", 1);
    CHECK(strstr(text, "12.5") && strstr(text, "STALE"));
    meter_format_value(text, sizeof(text), NAN, HMI_VALUE_VALID, "m", 1);
    CHECK(!strcmp(text, "ERR m"));
    return 0;
}
static int settings(void)
{
    hmi_core_t a, b;
    hmi_core_storage_t sa = storage(), sb = storage();
    CHECK(hmi_core_init(&a, &meter_demo_catalog, &sa));
    CHECK(hmi_core_init(&b, &meter_demo_catalog, &sb));
    hmi_action_t u = {HMI_ACTION_UNITS, 0, 1};
    CHECK(hmi_core_action(&a, &u));
    u.kind = HMI_ACTION_LANGUAGE;
    u.value = HMI_LANGUAGE_ZH;
    CHECK(hmi_core_action(&a, &u));
    CHECK(hmi_core_parameter(&a, DEMO_PARAMETER_MAX_SPEED, 33));
    uint8_t bytes[128];
    /* 块长度随产品目录增长，而不是随平台容量上限增长。 */
    size_t size = hmi_settings_size(&a);
    CHECK(a.snapshot.can_rate == HMI_CAN_RATE_500K);
    a.snapshot.can_rate = HMI_CAN_RATE_250K;
    CHECK(size == HMI_SETTINGS_OVERHEAD + meter_demo_catalog.parameter_count * HMI_SETTINGS_ENTRY_SIZE);
    CHECK(!hmi_settings_encode(&a, bytes, size - 1));
    CHECK(hmi_settings_encode(&a, bytes, sizeof(bytes)));
    CHECK(hmi_settings_decode(&b, bytes, size));
    CHECK(!memcmp(bytes, "MSP3", 4) && b.snapshot.can_rate == HMI_CAN_RATE_250K);
    uint32_t revision = b.snapshot.revision;
    CHECK(hmi_settings_decode(&b, bytes, size) && b.snapshot.revision == revision);
    uint8_t legacy[128];
    memcpy(legacy, bytes, size);
    memcpy(legacy, "MSP2", 4);
    legacy[7] = 0;
    seal(legacy, size);
    CHECK(!hmi_settings_decode(&b, legacy, size));
    CHECK(b.snapshot.can_rate == HMI_CAN_RATE_250K && b.snapshot.revision == revision);
    legacy[7] = 1;
    seal(legacy, size);
    CHECK(!hmi_settings_decode(&b, legacy, size));
    memcpy(legacy, bytes, size);
    legacy[7] = 3;
    seal(legacy, size);
    CHECK(!hmi_settings_decode(&b, legacy, size));
    CHECK(b.snapshot.can_rate == HMI_CAN_RATE_250K && b.snapshot.revision == revision);
    a.snapshot.can_rate = (hmi_can_rate_t)3;
    CHECK(!hmi_settings_encode(&a, legacy, sizeof(legacy)));
    a.snapshot.can_rate = HMI_CAN_RATE_250K;
    float stored = 0;
    CHECK(b.snapshot.imperial && b.snapshot.language == HMI_LANGUAGE_ZH && b.snapshot.brightness == 80);
    CHECK(hmi_snapshot_parameter(&b.snapshot, DEMO_PARAMETER_MAX_SPEED, &stored) && stored == 33);
    bytes[8] ^= 1;
    CHECK(!hmi_settings_decode(&b, bytes, size));
    /* 所有取值先整体校验再写入，被拒绝的文件不会让 core 停在半应用状态。
     * 结构体副本已检测不到这种问题：存储由调用方绑定后，两个副本共享同一组产品数组。 */
    CHECK(hmi_snapshot_parameter(&b.snapshot, DEMO_PARAMETER_MAX_SPEED, &stored) && stored == 33);
    CHECK(b.snapshot.imperial && b.snapshot.language == HMI_LANGUAGE_ZH && b.snapshot.brightness == 80);
    bytes[8] ^= 1;
    CHECK(!hmi_settings_decode(&b, bytes, 4));
    CHECK(!hmi_settings_decode(&b, bytes, size - 4));
    uint8_t foreign[128];
    memcpy(foreign, bytes, sizeof(foreign));
    foreign[6] = (uint8_t)(meter_demo_catalog.parameter_count - 1);
    CHECK(!hmi_settings_decode(&b, foreign, HMI_SETTINGS_OVERHEAD + foreign[6] * 4u));
    /* 格式完整但含一个越界参数的文件必须整体拒绝。第一项故意留在合法区间内，
     * 任何边校验边写入的解码器都会把它落进 core。 */
    uint8_t poisoned[128];
    memcpy(poisoned, bytes, sizeof(poisoned));
    store_float(poisoned + 16, 44.0f);
    store_float(poisoned + 24, 1000.0f);
    seal(poisoned, size);
    CHECK(!hmi_settings_decode(&b, poisoned, size));
    CHECK(hmi_snapshot_parameter(&b.snapshot, DEMO_PARAMETER_MAX_SPEED, &stored) && stored == 33);
    CHECK(hmi_snapshot_parameter(&b.snapshot, meter_demo_catalog.parameters[1].id, &stored) &&
          stored == meter_demo_catalog.parameters[1].initial);
    u.value = 2;
    CHECK(!hmi_core_action(&a, &u));
    return 0;
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    int result = 2;
    if (!strcmp(argv[1], "contracts"))
        result = contracts();
    if (!strcmp(argv[1], "core"))
        result = core();
    if (!strcmp(argv[1], "runtime"))
        result = runtime();
    if (!strcmp(argv[1], "protocol"))
        result = protocol();
    if (!strcmp(argv[1], "widgets"))
        result = widgets();
    if (!strcmp(argv[1], "settings"))
        result = settings();
    printf("%s: %s\n", argv[1], result ? "FAIL" : "PASS");
    return result;
}
