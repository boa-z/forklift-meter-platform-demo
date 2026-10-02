#ifndef DEMO_PRESENTATION_H
#define DEMO_PRESENTATION_H
#include "contracts/meter_domain.h"
#include "contracts/meter_wall_clock.h"
#include "generated/demo_catalog.h"
#include <stddef.h>
enum { DEMO_REMOTE_COUNT = 4, DEMO_ADMIN_COUNT = 4, DEMO_CLOCK_FIELDS = 6 };
/**
 * @brief 参考 Demo 的示例时区：东八区。
 *
 * 契约固定输出 UTC，区域偏移是 Product 的展示决策，所以它就落在这里，
 * 而不是藏进公共格式化函数。真实 Product 换成自己市场的偏移——这是唯一
 * 需要改的一处，日期与闰年换算都由公共层的纯函数负责。
 */
enum { DEMO_UTC_OFFSET_SECONDS = 8 * 3600 };
/**
 * @brief 对时页没有可用日期来源时使用的基准日。
 *
 * 对时轮盘必须有一个可编辑的起点，而 d13x 的 RTC 是 32 位秒计数器，掉电且无备份时
 * 读数落在驱动纪元哨兵上，公共层据此判定为"未设置"。若此时拒绝写入，未对时的板子
 * 就永远无法对时，因此这里给一个固定基准日：写入成功后计数器即有可信值。
 */
enum { DEMO_CLOCK_BASE_YEAR = 2026, DEMO_CLOCK_BASE_MONTH = 1, DEMO_CLOCK_BASE_DAY = 1 };
/** @brief 对时轮盘的列顺序；UI 与 App 共用同一套字段编号，避免两侧各自约定顺序。 */
typedef enum
{
    DEMO_CLOCK_YEAR = 0, DEMO_CLOCK_MONTH, DEMO_CLOCK_DAY,
    DEMO_CLOCK_HOUR, DEMO_CLOCK_MINUTE, DEMO_CLOCK_SECOND
} demo_clock_field_t;
typedef enum
{
    DEMO_INTENT_USER_LOGIN = 1, DEMO_INTENT_ADMIN_LOGIN, DEMO_INTENT_LOGOUT,
    DEMO_INTENT_ADMIN_FIRST = 10, DEMO_INTENT_REMOTE_FIRST = 20,
    /** @brief 对时按字段提交：+0..5 依次是年、月、日、时、分、秒，value 是该字段的值。 */
    DEMO_INTENT_CLOCK_FIRST = 30
} demo_settings_intent_t;
typedef enum
{
    DEMO_FEEDBACK_IDLE, DEMO_FEEDBACK_QUEUED, DEMO_FEEDBACK_APPLIED,
    DEMO_FEEDBACK_DENIED, DEMO_FEEDBACK_INVALID, DEMO_FEEDBACK_EXPIRED
} demo_feedback_t;
/* Product 所有的展示值，不借用可变快照数组，不包含 LVGL 或运行时状态。 */
typedef struct
{
    float value;
    meter_value_state_t state;
} demo_readout_t;
typedef enum
{
    DEMO_LINK_CONNECTED,
    DEMO_LINK_OFFLINE,
    DEMO_LINK_STALE,
    DEMO_LINK_WAITING
} demo_link_t;
typedef struct
{
    demo_readout_t reading;
    const char *unit;
} demo_monitor_view_t;
typedef struct
{
    float value;
    bool available;
} demo_parameter_view_t;
typedef struct
{
    uint16_t code;
    bool active;
} demo_fault_view_t;
typedef struct
{
    const char *firmware_version, *framework_revision;
    uint32_t revision;
    meter_profile_t profile;
    meter_language_t language;
    bool imperial;
    uint8_t brightness;
    bool limit_available;
    float limit;
    demo_link_t link;
    demo_readout_t speed, steering, soc, height, load, mileage, hours;
    float speed_maximum;
    const char *speed_unit;
    demo_readout_t status[5];
    demo_monitor_view_t monitors[DEMO_MONITOR_SLOTS];
    demo_parameter_view_t parameters[DEMO_REMOTE_COUNT];
    bool user_authorized, admin_authorized;
    demo_feedback_t auth_feedback, parameter_feedback;
    unsigned admin_values[DEMO_ADMIN_COUNT];
    demo_fault_view_t faults[DEMO_FAULT_SLOTS];
} demo_presentation_t;
/* 对已有一致快照做纯投影；Product 目录不可变。
 * 在接收所有者的快照副本上调用，不建立第二套发布机制。 */
void demo_presentation_build(const meter_snapshot_t *snapshot, demo_presentation_t *out);
/**
 * @brief 顶栏时钟文本：成功写入 "HH:MM"，时钟不可信时写入 "--:--" 并返回 false。
 *
 * 读数来自公共墙上时钟契约，本地化只做这一处偏移换算；out 必须能容纳 6 字节。
 */
bool demo_clock_text(char *out, size_t size);
/**
 * @brief 对时轮盘的初始字段（本地时间）：有可信读数就用它，否则用固定基准日。
 *
 * 返回读数是否可信，调用方据此决定是否提示"尚未对时"。无论返回值如何，
 * local_out 的字段都已填好且自洽，可以直接作为轮盘的起点。
 */
bool demo_clock_fields(meter_wall_time_t *local_out);
meter_action_t demo_speed_limit_intent(float value);
meter_action_t demo_settings_intent(demo_settings_intent_t intent, float value);
meter_action_t demo_remote_intent(unsigned row, float value);
meter_action_t demo_admin_intent(unsigned row, float value);
/** @brief 对时意图：一次携带一个字段的值，App 侧据此只替换该字段。 */
meter_action_t demo_clock_intent(demo_clock_field_t field, float value);
#endif
