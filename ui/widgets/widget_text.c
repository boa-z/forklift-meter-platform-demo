#include "ui/widgets/meter_widgets.h"
#include "ui/widgets/meter_format.h"
#include "ui/common/i18n/hmi_i18n_runtime.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
/* 示例控件自己渲染的词汇：只是标签名，各语言的措辞在 Demo 的语言包里（demo_i18n.c）。 */
static const char *const tags[] = {"meter.validity.live", "meter.validity.stale", "meter.validity.unknown",
                                   "meter.validity.error", "meter.state.on", "meter.state.off",
                                   "meter.value.error"};
_Static_assert(sizeof(tags) / sizeof(tags[0]) == METER_WIDGET_TXT_COUNT, "widget vocabulary count");
const char *meter_widget_tag(meter_widget_text_id_t id)
{
    return (unsigned)id < METER_WIDGET_TXT_COUNT ? tags[id] : "";
}
const char *meter_widget_text(meter_widget_text_id_t id)
{
    return hmi_i18n_tr(meter_widget_tag(id));
}
const char *meter_widget_state_text(hmi_value_state_t state)
{
    return meter_widget_text(state == HMI_VALUE_UNKNOWN ? METER_WIDGET_TXT_VALIDITY_UNKNOWN
                             : state == HMI_VALUE_STALE ? METER_WIDGET_TXT_VALIDITY_STALE
                             : state == HMI_VALUE_ERROR ? METER_WIDGET_TXT_VALIDITY_ERROR
                                                          : METER_WIDGET_TXT_VALIDITY_LIVE);
}
hmi_font_role_t meter_widget_font_role(const lv_font_t *font)
{
    return font && font->line_height >= 24 ? HMI_FONT_VALUE : HMI_FONT_LABEL;
}
void meter_widget_format_value(char *out, size_t size, float value, hmi_value_state_t state, const char *unit,
                               unsigned decimals, hmi_language_t language)
{
    if (!out || !size)
        return;
    if (state == HMI_VALUE_ERROR || (state != HMI_VALUE_UNKNOWN && !isfinite(value)))
    {
        const hmi_i18n_arg_t argument = hmi_i18n_arg_str(unit);
        (void)hmi_i18n_format(out, size, hmi_i18n_tr_in(language, tags[METER_WIDGET_TXT_VALUE_ERROR]),
                                &argument, 1u);
        return;
    }
    meter_format_value(out, size, value, state == HMI_VALUE_STALE ? HMI_VALUE_VALID : state, unit, decimals);
    if (state == HMI_VALUE_STALE)
    {
        size_t used = strlen(out);
        if (used < size)
            snprintf(out + used, size - used, " / %s",
                     hmi_i18n_tr_in(language, tags[METER_WIDGET_TXT_VALIDITY_STALE]));
    }
}
