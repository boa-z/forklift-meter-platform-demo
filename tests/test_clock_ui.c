#include "application/presentation.h"
#include "core/meter_core.h"
#include "platform/host/host_platform.h"
#include "product/demo_storage.h"
#include "services/settings_app.h"
#include "ui/demo_internal.h"
#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                                                     \
    do                                                                                                       \
    {                                                                                                        \
        if (!(condition))                                                                                    \
        {                                                                                                    \
            fprintf(stderr, "line %d: %s\n", __LINE__, #condition);                                          \
            return 1;                                                                                        \
        }                                                                                                    \
    } while (0)

/* 假 RTC：读固定值、可控失败；写通道像真实 RTC 一样把新值立刻变成后续读数的来源。 */
static meter_wall_time_t rtc_utc;
static unsigned writes;
static meter_wall_time_t written;

static bool read_rtc(meter_wall_time_t *out, void *context)
{
    (void)context;
    *out = rtc_utc;
    return true;
}
static bool read_failed(meter_wall_time_t *out, void *context)
{
    (void)out;
    (void)context;
    return false;
}
static bool write_rtc(const meter_wall_time_t *utc, void *context)
{
    (void)context;
    ++writes;
    written = *utc;
    rtc_utc = *utc;
    return true;
}

static unsigned actions_sent;
static meter_action_t last_action;
static bool send(void *context, const meter_action_t *action)
{
    ++actions_sent;
    last_action = *action;
    return demo_settings_action(&((meter_core_t *)context)->snapshot, action, lv_tick_get());
}

static void present(meter_core_t *core, demo_ui_t *ui)
{
    demo_settings_publish(&core->snapshot);
    demo_ui_present(ui, &core->snapshot, 16);
}

/* 轮盘停到某一项并发 VALUE_CHANGED，等同于用户拨动轮盘。 */
static void roll(demo_ui_t *ui, demo_clock_field_t field, unsigned index)
{
    lv_roller_set_selected(ui->clock_rollers[field], index, LV_ANIM_OFF);
    lv_obj_send_event(ui->clock_rollers[field], LV_EVENT_VALUE_CHANGED, NULL);
}

static bool field_set(meter_core_t *core, demo_clock_field_t field, float value)
{
    meter_action_t intent = demo_clock_intent(field, value);
    return demo_settings_action(&core->snapshot, &intent, lv_tick_get());
}

static bool field_rejected(meter_core_t *core, demo_clock_field_t field, float value)
{
    const unsigned before = writes;
    meter_action_t intent = demo_clock_intent(field, value);
    CHECK(!demo_settings_action(&core->snapshot, &intent, lv_tick_get()));
    CHECK(writes == before);
    return true;
}

int main(void)
{
    char text[8];

    /* 未绑定来源：只能给占位符，不能给 00:00 或 1970。 */
    meter_wall_clock_bind(NULL, NULL);
    meter_wall_clock_bind_set(NULL, NULL);
    CHECK(!demo_clock_text(text, sizeof(text)) && !strcmp(text, "--:--"));
    /* 缓冲区容不下 "HH:MM" 时也必须拒绝，而不是截断出一个假时间。 */
    char too_small[5];
    CHECK(!demo_clock_text(too_small, sizeof(too_small)));
    CHECK(!demo_clock_text(NULL, sizeof(text)));

    /* 固定 UTC 2026-10-01T00:30Z，按 Product 的东八区偏移即顶栏 08:30。 */
    rtc_utc = (meter_wall_time_t){2026u, 10u, 1u, 0u, 30u, 0u, true};
    meter_wall_clock_bind(read_rtc, NULL);
    meter_wall_clock_bind_set(write_rtc, NULL);
    CHECK(demo_clock_text(text, sizeof(text)) && !strcmp(text, "08:30"));
    /* 占位符与真实读数同宽，顶栏不会因为时钟可用与否而重排。 */
    CHECK(strlen(text) == strlen("--:--"));

    /* 跨午夜：UTC 16:05 + 8h 落到次日 00:05，日期由公共层负责。 */
    rtc_utc = (meter_wall_time_t){2026u, 10u, 1u, 16u, 5u, 0u, true};
    CHECK(demo_clock_text(text, sizeof(text)) && !strcmp(text, "00:05"));

    /* 源失败与越界读数都按不可信处理。 */
    meter_wall_clock_bind(read_failed, NULL);
    CHECK(!demo_clock_text(text, sizeof(text)) && !strcmp(text, "--:--"));
    rtc_utc = (meter_wall_time_t){2026u, 13u, 1u, 0u, 0u, 0u, true};
    meter_wall_clock_bind(read_rtc, NULL);
    CHECK(!demo_clock_text(text, sizeof(text)) && !strcmp(text, "--:--"));

    /* 对时页的初始字段：可信读数直接用它，不可信时回落到固定基准日但仍然可编辑。 */
    meter_wall_time_t local;
    rtc_utc = (meter_wall_time_t){2026u, 10u, 1u, 0u, 30u, 0u, true};
    CHECK(demo_clock_fields(&local) && local.year == 2026u && local.month == 10u && local.day == 1u &&
          local.hour == 8u && local.minute == 30u && local.second == 0u);
    meter_wall_clock_bind(NULL, NULL);
    CHECK(!demo_clock_fields(&local) && local.valid && local.year == DEMO_CLOCK_BASE_YEAR &&
          local.month == DEMO_CLOCK_BASE_MONTH && local.day == DEMO_CLOCK_BASE_DAY);
    CHECK(!demo_clock_fields(NULL));

    meter_core_t core;
    demo_domain_store_t store;
    meter_core_storage_t storage = demo_domain_bind(&store);
    CHECK(meter_core_init(&core, &meter_demo_catalog, &storage));
    lv_init();
    CHECK(meter_i18n_init() && demo_i18n_init() && meter_host_open(true));
    meter_ui_actions_t actions = {send, &core};
    demo_ui_t *ui = demo_ui_create(lv_screen_active(), &actions);
    CHECK(ui);

    /* 顶栏跟随来源：有源显示时间，无源显示占位符。 */
    rtc_utc = (meter_wall_time_t){2026u, 10u, 1u, 0u, 30u, 0u, true};
    meter_wall_clock_bind(read_rtc, NULL);
    meter_wall_clock_bind_set(write_rtc, NULL);
    demo_ui_present(ui, &core.snapshot, 16);
    CHECK(!strcmp(lv_label_get_text(ui->clock), "08:30"));
    meter_wall_clock_bind(NULL, NULL);
    demo_ui_present(ui, &core.snapshot, 16);
    CHECK(!strcmp(lv_label_get_text(ui->clock), "--:--"));
    meter_wall_clock_bind(read_rtc, NULL);

    /* 对时在用户设置的第二页，而且是一个条目：无需任何权限，翻页即可到达。 */
    present(&core, ui);
    CHECK(!ui->view.admin_authorized && !ui->view.user_authorized);
    CHECK(ui->settings_subpage == 0u && ui->settings_pager.count == (unsigned)DEMO_USER_SETTINGS_PAGES);
    CHECK(lv_obj_is_hidden(ui->settings_cards[DEMO_SETTINGS_CARD_CLOCK]));
    CHECK(!lv_obj_is_hidden(ui->settings_cards[DEMO_SETTINGS_CARD_USER]));
    lv_obj_send_event(ui->settings_pager.next, LV_EVENT_CLICKED, NULL);
    present(&core, ui);
    CHECK(ui->settings_subpage == 1u);
    CHECK(!strcmp(lv_label_get_text(ui->settings_pager.indicator), "2/2"));
    CHECK(!lv_obj_is_hidden(ui->settings_cards[DEMO_SETTINGS_CARD_CLOCK]));
    CHECK(lv_obj_is_hidden(ui->settings_cards[DEMO_SETTINGS_CARD_USER]));
    CHECK(!strcmp(lv_label_get_text(ui->settings_title), demo_i18n_text(DEMO_TXT_CLOCK_SET)));
    /* 第二页只有"时钟"这一条目，值就是当前本地时间；轮盘界面此时还没出现。 */
    CHECK(!lv_obj_is_hidden(ui->clock_entry));
    CHECK(!strcmp(lv_label_get_text(ui->clock_value), "08:30"));
    CHECK(lv_obj_is_hidden(ui->clock_group));
    CHECK(lv_obj_is_hidden(ui->settings_detail));

    /* 点进条目才是真正的对时界面：六个轮盘，量程与顺序都要与 App 侧一致。 */
    lv_obj_send_event(ui->clock_entry, LV_EVENT_CLICKED, NULL);
    CHECK(!lv_obj_is_hidden(ui->settings_detail) && !lv_obj_is_hidden(ui->clock_group));
    CHECK(ui->selected_setting == DEMO_SETTING_CLOCK);
    CHECK(!strcmp(lv_label_get_text(ui->settings_detail_title), demo_i18n_text(DEMO_TXT_CLOCK_SET)));
    CHECK(!strcmp(lv_label_get_text(ui->settings_detail_value), "08:30"));
    /* 轮盘界面里没有其它条目的控件。 */
    CHECK(lv_obj_is_hidden(ui->unit_button) && lv_obj_is_hidden(ui->language_button));
    CHECK(lv_obj_is_hidden(ui->brightness) && lv_obj_is_hidden(ui->limit));
    CHECK(lv_obj_is_hidden(ui->admin_detail_button));
    const unsigned expected_count[DEMO_CLOCK_FIELDS] = {79u, 12u, 31u, 24u, 60u, 60u};
    const unsigned expected_index[DEMO_CLOCK_FIELDS] = {2026u - 2021u, 9u, 0u, 8u, 30u, 0u};
    for (unsigned field = 0u; field < DEMO_CLOCK_FIELDS; ++field)
    {
        CHECK(lv_roller_get_option_count(ui->clock_rollers[field]) == expected_count[field]);
        CHECK(lv_roller_get_selected(ui->clock_rollers[field]) == expected_index[field]);
    }

    /* 拨动一个轮盘就提交一个字段：意图只带该字段的值，且不需要管理员权限。 */
    const unsigned before = actions_sent;
    roll(ui, DEMO_CLOCK_YEAR, 2029u - 2021u);
    CHECK(actions_sent == before + 1u);
    CHECK(last_action.kind == METER_ACTION_PRODUCT);
    CHECK(last_action.id == (uint16_t)(DEMO_INTENT_CLOCK_FIRST + DEMO_CLOCK_YEAR));
    CHECK(last_action.value == 2029.0f);
    /* 本地 2029-10-01 08:30 加东八区偏移写回，UTC 是同日 00:30。 */
    CHECK(written.year == 2029u && written.month == 10u && written.day == 1u && written.hour == 0u &&
          written.minute == 30u);
    /* 写成功后假 RTC 也变了，所以下一次读数是新的本地时间。 */
    CHECK(demo_clock_fields(&local) && local.year == 2029u && local.hour == 8u);

    roll(ui, DEMO_CLOCK_MINUTE, 45u);
    CHECK(last_action.id == (uint16_t)(DEMO_INTENT_CLOCK_FIRST + DEMO_CLOCK_MINUTE));
    CHECK(written.year == 2029u && written.hour == 0u && written.minute == 45u);
    roll(ui, DEMO_CLOCK_HOUR, 23u);
    CHECK(written.hour == 15u && written.minute == 45u);

    /* App 侧逐字段校验：范围、整数性与真实日历（月长、闰年）都要挡住。 */
    const unsigned settled = writes;
    meter_action_t accepted = demo_clock_intent(DEMO_CLOCK_SECOND, 59.0f);
    CHECK(demo_settings_action(&core.snapshot, &accepted, lv_tick_get()));
    CHECK(writes == settled + 1u);
    CHECK(field_rejected(&core, DEMO_CLOCK_YEAR, 2020.0f));
    CHECK(field_rejected(&core, DEMO_CLOCK_YEAR, 2100.0f));
    CHECK(field_rejected(&core, DEMO_CLOCK_YEAR, 2026.5f));
    CHECK(field_rejected(&core, DEMO_CLOCK_MONTH, 0.0f));
    CHECK(field_rejected(&core, DEMO_CLOCK_MONTH, 13.0f));
    CHECK(field_rejected(&core, DEMO_CLOCK_DAY, 0.0f));
    CHECK(field_rejected(&core, DEMO_CLOCK_DAY, 32.0f));
    CHECK(field_rejected(&core, DEMO_CLOCK_HOUR, 24.0f));
    CHECK(field_rejected(&core, DEMO_CLOCK_MINUTE, 60.0f));
    CHECK(field_rejected(&core, DEMO_CLOCK_SECOND, 60.0f));
    /* 4 月没有 31 日：零偏移往返会把它归一化，因此必须拒绝而不是静默进位。 */
    CHECK(field_set(&core, DEMO_CLOCK_MONTH, 4.0f));
    CHECK(field_rejected(&core, DEMO_CLOCK_DAY, 31.0f));
    /* 2029 不是闰年，2 月 29 日不存在；把年改成 2028 后同一天就合法。 */
    CHECK(field_set(&core, DEMO_CLOCK_MONTH, 2.0f));
    CHECK(field_rejected(&core, DEMO_CLOCK_DAY, 29.0f));
    CHECK(field_set(&core, DEMO_CLOCK_YEAR, 2028.0f));
    const unsigned leap_base = writes;
    meter_action_t leap = demo_clock_intent(DEMO_CLOCK_DAY, 29.0f);
    CHECK(demo_settings_action(&core.snapshot, &leap, lv_tick_get()));
    CHECK(writes == leap_base + 1u);

    /* CAN 波特率：管理员页上的三个单选按钮，只有当前值处于 CHECKED。 */
    demo_settings_show_page(ui, 1);
    present(&core, ui);
    CHECK(!ui->view.admin_authorized && lv_obj_is_hidden(ui->admin_items[0]));
    for (unsigned i = 0u; i < DEMO_CAN_RATE_OPTIONS; ++i)
        CHECK(lv_obj_is_hidden(ui->rate_buttons[i]));
    demo_settings_reset(1);
    meter_action_t login = demo_settings_intent(DEMO_INTENT_ADMIN_LOGIN, 5312);
    CHECK(demo_settings_action(&core.snapshot, &login, lv_tick_get()));
    present(&core, ui);
    CHECK(ui->view.admin_authorized && !lv_obj_is_hidden(ui->admin_items[0]));
    /* 打开 CAN 波特率这一行：单选组出现，循环切换按钮让位。 */
    lv_obj_send_event(ui->admin_items[0], LV_EVENT_CLICKED, NULL);
    CHECK(ui->selected_setting == 4u);
    CHECK(lv_obj_is_hidden(ui->admin_detail_button));
    for (unsigned i = 0u; i < DEMO_CAN_RATE_OPTIONS; ++i)
    {
        CHECK(!lv_obj_is_hidden(ui->rate_buttons[i]));
        CHECK(lv_obj_has_state(ui->rate_buttons[i], LV_STATE_CHECKED) == (ui->view.admin_values[0] == i));
    }
    /* 选 500 kbit/s：提交第 2 项，发布后选中态随之移动。 */
    const uint32_t rate_now = lv_tick_get();
    lv_obj_send_event(ui->rate_buttons[2], LV_EVENT_CLICKED, NULL);
    CHECK(last_action.id == (uint16_t)(DEMO_INTENT_ADMIN_FIRST + 0u) && last_action.value == 2.0f);
    demo_settings_run(rate_now, NULL);
    present(&core, ui);
    CHECK(ui->view.admin_values[0] == 2u);
    CHECK(lv_obj_has_state(ui->rate_buttons[2], LV_STATE_CHECKED));
    CHECK(!lv_obj_has_state(ui->rate_buttons[0], LV_STATE_CHECKED));
    /* 其余管理员行仍然用循环切换按钮，不能被单选组顶掉。 */
    lv_obj_send_event(ui->admin_items[1], LV_EVENT_CLICKED, NULL);
    CHECK(ui->selected_setting == 5u && !lv_obj_is_hidden(ui->admin_detail_button));
    for (unsigned i = 0u; i < DEMO_CAN_RATE_OPTIONS; ++i)
        CHECK(lv_obj_is_hidden(ui->rate_buttons[i]));

    meter_wall_clock_bind(NULL, NULL);
    meter_wall_clock_bind_set(NULL, NULL);
    demo_ui_destroy(ui);
    meter_host_close();
    lv_deinit();
    puts("Wall-clock header, page-two roller fields, per-field App validation and CAN radio PASS");
    return 0;
}
