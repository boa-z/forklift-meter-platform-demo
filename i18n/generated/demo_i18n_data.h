/* 从 languages.json 与各语言译文生成：python tools/generate_i18n.py；请勿手改。 */
#ifndef DEMO_I18N_DATA_H
#define DEMO_I18N_DATA_H
#include "ui/common/i18n/meter_i18n_runtime.h"
/* 启用的语言数；语言编号 0..1 即持久化的语言字节。 */
#define DEMO_I18N_DATA_LANGUAGE_COUNT 2u
/* 首次开机（没有已存设置）使用的语言。 */
#define DEMO_I18N_DATA_DEFAULT_LANGUAGE 0u
extern const meter_i18n_language_t demo_i18n_data_languages[DEMO_I18N_DATA_LANGUAGE_COUNT];
/* lv_i18n 语言包，以 NULL 结尾；第一项是源语言，译文缺失时回退到它。 */
extern const lv_i18n_language_pack_t demo_i18n_data_pack[DEMO_I18N_DATA_LANGUAGE_COUNT + 1u];
/* 各语言名称的消息键（语言选择页），按语言编号。 */
extern const char *const demo_i18n_data_name_keys[DEMO_I18N_DATA_LANGUAGE_COUNT];
#endif
