#include "services/settings_app.h"
#include "application/presentation.h"
#include "contracts/hmi_time.h"
#include <math.h>

/* 公开演示凭据只证明授权流程，不作为真实车辆身份或安全策略。 */
enum { USER_PERMISSION = 1, ADMIN_PERMISSION = 2, GRANT_MS = 60000 };
/* 合成远端参数的量程；写入在本机立即生效，演示的是授权与量程校验，不是远端事务。 */
static const struct
{
    float minimum, maximum;
} remote_catalog[DEMO_REMOTE_COUNT] = {{5, 50}, {0, 10}, {0, 8}, {0, 100}};
/* 运行时线程单一所有者；快照发布复制下方结果，UI 不读取这些可变对象。 */
static struct
{
    unsigned permissions;
    uint32_t expires_ms;
} grant;
static uint32_t app_now;
static demo_feedback_t auth_feedback, parameter_feedback;
static unsigned admin_values[DEMO_ADMIN_COUNT];
static float remote_values[DEMO_REMOTE_COUNT] = {25, 3, 4, 30};

static bool allows(unsigned permission, uint32_t now_ms)
{
    return (grant.permissions & permission) != 0u && !hmi_time_reached(now_ms, grant.expires_ms);
}
static void revoke(void)
{
    grant.permissions = 0u;
}
void demo_settings_reset(uint32_t generation)
{
    (void)generation;
    /* 会话切换撤销授权。 */
    revoke();
    auth_feedback = DEMO_FEEDBACK_IDLE;
}
bool demo_settings_action(hmi_snapshot_t *snapshot, const hmi_action_t *action, uint32_t now_ms)
{
    if (!snapshot || !action || !isfinite(action->value))
        return false;
    app_now = now_ms;
    if (action->kind != HMI_ACTION_PRODUCT)
        return true;
    if (action->id == DEMO_INTENT_LOGOUT)
    {
        revoke();
        auth_feedback = DEMO_FEEDBACK_IDLE;
        return true;
    }
    if (action->id == DEMO_INTENT_USER_LOGIN || action->id == DEMO_INTENT_ADMIN_LOGIN)
    {
        bool admin = action->id == DEMO_INTENT_ADMIN_LOGIN;
        bool accepted = action->value == (admin ? 5312.0f : 1234.0f);
        revoke();
        if (accepted)
        {
            grant.permissions = admin ? USER_PERMISSION | ADMIN_PERMISSION : USER_PERMISSION;
            grant.expires_ms = now_ms + GRANT_MS;
        }
        auth_feedback = accepted ? DEMO_FEEDBACK_APPLIED : DEMO_FEEDBACK_DENIED;
        return accepted;
    }
    /* 对时属于用户设置，不要求管理员权限；它写的是本机时钟而不是域信号。 */
    if (action->id >= DEMO_INTENT_CLOCK_FIRST && action->id < DEMO_INTENT_CLOCK_FIRST + DEMO_CLOCK_FIELDS)
    {
        const unsigned field = action->id - DEMO_INTENT_CLOCK_FIRST;
        const float raw = action->value;
        if (!(raw >= 0.0f) || raw > 9999.0f || floorf(raw) != raw)
            return false;
        /* 以当前读数为基准只替换一个字段：有可信读数就用它，否则用固定基准日。
           字段逐个落盘，所以每一次提交本身都是一个自洽的完整时刻。 */
        hmi_wall_time_t local;
        (void)demo_clock_fields(&local);
        const unsigned value = (unsigned)raw;
        switch ((demo_clock_field_t)field)
        {
        case DEMO_CLOCK_YEAR:
            /* 下界高于驱动纪元哨兵：d13x 的 set_secs 会整体拒掉更早的读数。 */
            if (value < 2021u || value > 2099u)
                return false;
            local.year = (uint16_t)value;
            break;
        case DEMO_CLOCK_MONTH:
            if (value < 1u || value > 12u)
                return false;
            local.month = (uint8_t)value;
            break;
        case DEMO_CLOCK_DAY:
            if (value < 1u || value > 31u)
                return false;
            local.day = (uint8_t)value;
            break;
        case DEMO_CLOCK_HOUR:
            if (value > 23u)
                return false;
            local.hour = (uint8_t)value;
            break;
        case DEMO_CLOCK_MINUTE:
            if (value > 59u)
                return false;
            local.minute = (uint8_t)value;
            break;
        case DEMO_CLOCK_SECOND:
            if (value > 59u)
                return false;
            local.second = (uint8_t)value;
            break;
        default:
            return false;
        }
        /* 月长与闰年由公共层判定：非法日期（如 2 月 30 日）零偏移往返后会被归一化成别的日子。 */
        hmi_wall_time_t roundtrip;
        if (!hmi_wall_time_shift(&local, 0, &roundtrip) || roundtrip.year != local.year ||
            roundtrip.month != local.month || roundtrip.day != local.day)
            return false;
        hmi_wall_time_t utc;
        /* 界面编辑的是本地时间，写回要按同一偏移换算回 UTC。 */
        return hmi_wall_time_shift(&local, -DEMO_UTC_OFFSET_SECONDS, &utc) &&
               hmi_wall_clock_write(&utc);
    }
    if (!allows(ADMIN_PERMISSION, now_ms))
    {
        auth_feedback = DEMO_FEEDBACK_DENIED;
        parameter_feedback = DEMO_FEEDBACK_DENIED;
        return false;
    }
    if (action->id >= DEMO_INTENT_ADMIN_FIRST && action->id < DEMO_INTENT_ADMIN_FIRST + DEMO_ADMIN_COUNT)
    {
        unsigned row = action->id - DEMO_INTENT_ADMIN_FIRST;
        unsigned maximum = row == 0 ? 2 : 1;
        if (action->value < 0 || action->value > maximum || floorf(action->value) != action->value)
            return false;
        if (row == 0)
        {
            /* App 更新待重启配置；运行中不重配设备，NVM 仍由统一服务持久化。 */
            hmi_can_rate_t rate = (hmi_can_rate_t)(unsigned)action->value;
            if (snapshot->can_rate != rate)
            {
                snapshot->can_rate = rate;
                ++snapshot->revision;
            }
        }
        else
            /* 其余三项仍为易失性演示偏好，不修改小时累计或擦除介质。 */
            admin_values[row] = (unsigned)action->value;
        return true;
    }
    if (action->id >= DEMO_INTENT_REMOTE_FIRST && action->id < DEMO_INTENT_REMOTE_FIRST + DEMO_REMOTE_COUNT)
    {
        unsigned row = action->id - DEMO_INTENT_REMOTE_FIRST;
        bool valid = action->value >= remote_catalog[row].minimum && action->value <= remote_catalog[row].maximum;
        if (valid)
            remote_values[row] = action->value;
        parameter_feedback = valid ? DEMO_FEEDBACK_APPLIED : DEMO_FEEDBACK_INVALID;
        return valid;
    }
    return false;
}
void demo_settings_run(uint32_t now_ms, const hmi_command_port_t *commands)
{
    (void)commands;
    app_now = now_ms;
    if (grant.permissions && !allows(USER_PERMISSION, now_ms))
    {
        revoke();
        auth_feedback = DEMO_FEEDBACK_EXPIRED;
    }
}
static void publish(hmi_snapshot_t *snapshot, hmi_signal_id_t id, float value)
{
    size_t index = hmi_catalog_index(snapshot->catalog, id);
    if (index >= snapshot->catalog->signal_count)
        return;
    hmi_value_t *slot = &snapshot->signals[index];
    if (slot->state != HMI_VALUE_VALID || slot->value != value)
        ++snapshot->revision;
    *slot = (hmi_value_t){value, app_now, HMI_VALUE_VALID, HMI_SOURCE_NONE};
}
void demo_settings_publish(hmi_snapshot_t *snapshot)
{
    bool user = allows(USER_PERMISSION, app_now);
    bool admin = allows(ADMIN_PERMISSION, app_now);
    publish(snapshot, DEMO_ACCESS_ROLE, admin ? 2 : user ? 1 : 0);
    publish(snapshot, DEMO_ACCESS_RESULT, (float)auth_feedback);
    publish(snapshot, DEMO_REMOTE_RESULT, (float)parameter_feedback);
    const hmi_signal_id_t admin_ids[] = {DEMO_ADMIN_CAN, DEMO_ADMIN_HOURS, DEMO_ADMIN_SPEED, DEMO_ADMIN_MEMORY};
    const hmi_signal_id_t remote_ids[] = {DEMO_REMOTE_SPEED, DEMO_REMOTE_RAMP, DEMO_REMOTE_LIFT, DEMO_REMOTE_REGEN};
    for (unsigned i = 0; i < DEMO_ADMIN_COUNT; ++i)
        publish(snapshot, admin_ids[i], i == 0 ? (float)snapshot->can_rate : (float)admin_values[i]);
    for (unsigned i = 0; i < DEMO_REMOTE_COUNT; ++i)
        publish(snapshot, remote_ids[i], remote_values[i]);
}
