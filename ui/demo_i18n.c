#include "ui/demo_i18n.h"
#include "i18n/generated/demo_i18n_data.h"
#include "ui/widgets/meter_widgets.h"
#include <lvgl.h>
#include <stddef.h>
LV_FONT_DECLARE(meter_demo_cjk_14);
LV_FONT_DECLARE(meter_demo_cjk_20);
static const char *const tags[] = {"FIELD",
                                   "REFERENCE",
                                   "WAITING",
                                   "CONNECTED",
                                   "OFFLINE",
                                   "STALE",
                                   "DASHBOARD",
                                   "MONITOR",
                                   "FAULTS",
                                   "SETTINGS",
                                   "TRACTION",
                                   "ENERGY",
                                   "STEERING",
                                   "LIFT_HEIGHT",
                                   "LOAD",
                                   "MILEAGE",
                                   "LIVE_TELEMETRY",
                                   "TELEMETRY_SUBTITLE",
                                   "ADVISORIES",
                                   "FAULT_SUBTITLE",
                                   "LOCAL_PREFERENCES",
                                   "SETTINGS_SUBTITLE",
                                   "PARAMETER_MONITOR",
                                   "CONTROLLER_SETTINGS",
                                   "PASSWORD",
                                   "USER_SETTINGS",
                                   "ADMIN_SETTINGS",
                                   "INSTRUMENT_VERSION",
                                   "USER_PASSWORD",
                                   "ADMIN_PASSWORD",
                                   "CAN_RATE",
                                   "HOUR_METER",
                                   "SPEED_DISPLAY",
                                   "MODE_MEMORY",
                                   "CLOCK_SET",
                                   "PARAMETER_0",
                                   "PARAMETER_1",
                                   "PARAMETER_2",
                                   "PARAMETER_3",
                                   "PARAMETER_4",
                                   "PARAMETER_5",
                                   "PARAMETER_6",
                                   "PARAMETER_7",
                                   "PARAMETER_8",
                                   "PARAMETER_9",
    "ACCESS_IDLE",
    "ACCESS_QUEUED",
    "ACCESS_APPLIED",
    "ACCESS_DENIED",
    "ACCESS_INVALID",
    "ACCESS_EXPIRED",
    "ACCESS_USER",
    "ACCESS_ADMIN",
    "SIGN_OUT",
    "ADMIN_LOCKED",
    "DEMO_CREDENTIALS",
    "DEMO_SESSION",
    "REMOTE_NOTE",
    "ADMIN_NOTE",
    "EDIT_VALUE",
    "ENTER_PIN",
    "WORK_MODE",
    "POWER_MODE",
    "REMEMBER",
    "RESET_MODE",
    "FIRMWARE",
    "PRODUCT",
    "FRAMEWORK",
    "HOST_BUILD",
                                   "SPEED_UNITS",
                                   "BRIGHTNESS",
                                   "SPEED_LIMIT",
                                   "LIMITS_NOTE",
                                   "SETTINGS_OK",
                                   "SETTINGS_ERROR",
                                   "METRIC",
                                   "IMPERIAL",
                                   "LANGUAGE",
                                   "ENGLISH",
                                   "CHINESE",
                                   "ACTIVE",
                                   "CLEAR",
                                   "SEAT",
                                   "BRAKE",
                                   "NEUTRAL",
                                   "CHARGE",
                                   "WARNING",
                                   "OTA_TITLE",
                                   "OTA_NOTE",
                                   "OTA_IDLE",
                                   "OTA_DOWNLOADING",
                                   "OTA_TRANSFERRED",
                                   "OTA_VERIFYING",
                                   "OTA_CANDIDATE",
                                   "OTA_DURABLE",
                                   "OTA_ACTIVATING",
                                   "OTA_ACTIVATED",
                                   "OTA_CONFIRMED",
                                   "OTA_ABORTED",
                                   "OTA_FAILED",
                                   "OTA_CURRENT",
                                   "OTA_TARGET",
                                   "OTA_ERROR",
                                   "OTA_STOPPED",
                                   "OTA_PREVIEW",
                                   "MONITOR_0",
                                   "MONITOR_1",
                                   "MONITOR_2",
                                   "MONITOR_3",
                                   "MONITOR_4",
                                   "MONITOR_5",
                                   "MONITOR_6",
                                   "MONITOR_7",
                                   "MONITOR_8",
                                   "MONITOR_9",
                                   "MONITOR_10",
                                   "MONITOR_11",
                                   "MONITOR_12",
                                   "MONITOR_13",
                                   "FAULT_0",
                                   "FAULT_1",
                                   "FAULT_2",
                                   "FAULT_3",
                                   "FAULT_4",
                                   "FAULT_5",
                                   "FAULT_6",
                                   "FAULT_7",
                                   "FAULT_8",
                                   "FAULT_9",
                                   NULL};
_Static_assert(sizeof(tags) / sizeof(tags[0]) == DEMO_TXT_COUNT + 1, "demo tag count");
/* 文字在 i18n/（languages.json 与每语言一份 JSON）：tools/generate_i18n.py 生成 demo_i18n_data_pack，
   含本文件的界面文字，也含示例控件自己渲染的词汇（ui/widgets/widget_text.c 的 meter.* 标签）。 */
#define DEMO_MONITOR_COUNT (DEMO_TXT_FAULT_0 - DEMO_TXT_MONITOR_0)
#define DEMO_FAULT_COUNT (DEMO_TXT_COUNT - DEMO_TXT_FAULT_0)
/* 语言表：编号即持久化语言字节。英文保持控件设计时的字体，中文使用仓库内固定的子集字体。 */
static const meter_i18n_language_t demo_languages[] = {
    {METER_LANGUAGE_EN, METER_LANGUAGE_CODE_EN, NULL, NULL},
    {METER_LANGUAGE_ZH, METER_LANGUAGE_CODE_ZH, &meter_demo_cjk_20, &meter_demo_cjk_20}};
bool demo_i18n_init(void)
{
    return meter_i18n_register_languages(demo_languages, sizeof(demo_languages) / sizeof(demo_languages[0])) &&
           meter_i18n_register_pack(demo_i18n_data_pack);
}
const char *demo_i18n_tag(demo_text_id_t id)
{
    return (unsigned)id < DEMO_TXT_COUNT ? tags[id] : "";
}
const char *demo_i18n_text(demo_text_id_t id)
{
    return meter_i18n_tr(demo_i18n_tag(id));
}
void demo_i18n_bind_label(lv_obj_t *label, demo_text_id_t id)
{
    meter_i18n_bind_text(label, demo_i18n_tag(id), meter_widget_font_role(lv_obj_get_style_text_font(label, 0)));
}
const char *demo_i18n_monitor_label(size_t i)
{
    return i < DEMO_MONITOR_COUNT ? demo_i18n_text((demo_text_id_t)(DEMO_TXT_MONITOR_0 + i)) : "";
}
const char *demo_i18n_fault_description(size_t i)
{
    return i < DEMO_FAULT_COUNT ? demo_i18n_text((demo_text_id_t)(DEMO_TXT_FAULT_0 + i)) : "";
}

demo_text_id_t demo_i18n_feedback(unsigned feedback)
{
    const demo_text_id_t ids[] = {DEMO_TXT_ACCESS_IDLE, DEMO_TXT_ACCESS_QUEUED, DEMO_TXT_ACCESS_APPLIED,
                                  DEMO_TXT_ACCESS_DENIED, DEMO_TXT_ACCESS_INVALID, DEMO_TXT_ACCESS_EXPIRED};
    return feedback < sizeof(ids) / sizeof(ids[0]) ? ids[feedback] : DEMO_TXT_ACCESS_INVALID;
}
