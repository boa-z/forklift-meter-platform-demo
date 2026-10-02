#include "ui/demo_internal.h"
#include <stdlib.h>
#include <string.h>

static void action(lv_event_t *event)
{
    demo_ui_t *u = lv_event_get_user_data(event);
    lv_obj_t *target = lv_event_get_target_obj(event);
    meter_action_t intent = {METER_ACTION_UNITS, 0, !u->view.imperial};
    if (target == u->language_button)
    {
        intent.kind = METER_ACTION_LANGUAGE;
        intent.value = u->view.language == METER_LANGUAGE_EN ? METER_LANGUAGE_ZH : METER_LANGUAGE_EN;
    }
    else if (target == u->brightness)
        intent = (meter_action_t){METER_ACTION_BRIGHTNESS, 0, (float)lv_slider_get_value(target)};
    else if (target == u->limit)
        intent = demo_speed_limit_intent((float)lv_slider_get_value(target));
    u->action_failed = !u->actions.send || !u->actions.send(u->actions.context, &intent);
}
/* 列表只负责路由，实际修改必须发生在独立详情页。 */
static const demo_text_id_t setting_titles[] = {
    DEMO_TXT_SPEED_UNITS, DEMO_TXT_LANGUAGE,   DEMO_TXT_BRIGHTNESS,    DEMO_TXT_SPEED_LIMIT,
    DEMO_TXT_CAN_RATE,    DEMO_TXT_HOUR_METER, DEMO_TXT_SPEED_DISPLAY, DEMO_TXT_MODE_MEMORY};

/* 对时轮盘按当前读数铺一次；实现放在下方，详情页打开时要调用它。 */
static void clock_sync(demo_ui_t *u);
static void settings_detail_close(lv_event_t *event)
{
    demo_ui_t *u = lv_event_get_user_data(event);
    lv_obj_set_hidden(u->settings_detail, true);
    lv_obj_set_hidden(lv_obj_get_parent(u->settings_pager.indicator), false);
}
static void settings_detail_open(lv_event_t *event)
{
    demo_ui_t *u = lv_event_get_user_data(event);
    lv_obj_t *target = lv_event_get_target_obj(event);
    unsigned selected = DEMO_SETTING_CLOCK + 1u; /* 未匹配任何已注册条目 */
    for (unsigned i = 0; i < 4; ++i)
        if (target == u->setting_entries[i])
            selected = i;
    for (unsigned i = 0; i < DEMO_ADMIN_COUNT; ++i)
        if (target == u->admin_items[i] && u->view.admin_authorized)
            selected = 4 + i;
    if (target == u->clock_entry)
        selected = DEMO_SETTING_CLOCK;
    if (selected > DEMO_SETTING_CLOCK)
        return;
    u->selected_setting = selected;
    lv_obj_t *controls[] = {u->unit_button, u->language_button, u->brightness, u->limit};
    for (unsigned i = 0; i < 4; ++i)
        lv_obj_set_hidden(controls[i], i != selected);
    /* CAN 波特率行用自己的单选组，其余管理员行仍用循环切换按钮。 */
    const bool admin_row = selected >= 4u && selected < DEMO_SETTING_CLOCK;
    const bool rate_row = admin_row && selected == 4u;
    lv_obj_set_hidden(u->admin_detail_button, !admin_row || rate_row);
    for (unsigned i = 0; i < DEMO_CAN_RATE_OPTIONS; ++i)
        lv_obj_set_hidden(u->rate_buttons[i], !rate_row);
    /* 对时轮盘只在从"时钟"条目进来时出现，并在进入时按当前读数铺一次。 */
    const bool clock_row = selected == DEMO_SETTING_CLOCK;
    lv_obj_set_hidden(u->clock_group, !clock_row);
    if (clock_row)
        clock_sync(u);
    lv_obj_set_hidden(u->settings_detail, false);
    lv_obj_set_hidden(lv_obj_get_parent(u->settings_pager.indicator), true);
    demo_settings_update(u);
}
void demo_editors_close(demo_ui_t *u)
{
    lv_obj_set_hidden(u->parameter_editor, true);
    lv_obj_set_hidden(u->password_editor, true);
    lv_textarea_set_text(u->user_password, "");
    lv_textarea_set_text(u->admin_password, "");
    if (u->settings_detail)
        lv_obj_set_hidden(u->settings_detail, true);
}
static void password_open(lv_event_t *event)
{
    demo_ui_t *u = lv_event_get_user_data(event);
    u->password_admin = lv_event_get_target_obj(event) == u->admin_password_button;
    lv_obj_set_hidden(u->user_password, u->password_admin);
    lv_obj_set_hidden(u->admin_password, !u->password_admin);
    lv_obj_t *field = u->password_admin ? u->admin_password : u->user_password;
    lv_textarea_set_text(field, "");
    lv_obj_set_hidden(u->password_editor, false);
}
static void password_submit(lv_event_t *event);
static void password_back(lv_event_t *event)
{
    demo_ui_t *u = lv_event_get_user_data(event);
    (void)event;
    u->admin_navigation_pending = false;
    demo_editors_close(u);
}
static void password_keypad(lv_event_t *event)
{
    demo_ui_t *u = lv_event_get_user_data(event);
    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED)
        return;
    lv_obj_t *field = u->password_admin ? u->admin_password : u->user_password;
    const char *key = lv_buttonmatrix_get_button_text(
        lv_event_get_target_obj(event), lv_buttonmatrix_get_selected_button(lv_event_get_target_obj(event)));
    if (!key)
        return;
    if (!strcmp(key, LV_SYMBOL_BACKSPACE))
        lv_textarea_delete_char(field);
    else if (!strcmp(key, LV_SYMBOL_OK))
        lv_obj_send_event(u->password_keyboard, LV_EVENT_READY, NULL);
    else if (strlen(key) == 1u && key[0] >= '0' && key[0] <= '9' && strlen(lv_textarea_get_text(field)) < 4u)
        lv_textarea_add_text(field, key);
}
static void password_submit(lv_event_t *event)
{
    demo_ui_t *u = lv_event_get_user_data(event);
    if (lv_event_get_code(event) == LV_EVENT_CANCEL)
    {
        demo_editors_close(u);
        return;
    }
    if (lv_event_get_code(event) != LV_EVENT_READY)
        return;
    const char *text = lv_textarea_get_text(u->password_admin ? u->admin_password : u->user_password);
    bool valid = strlen(text) == 4;
    for (size_t i = 0; valid && i < 4; ++i)
        valid = text[i] >= '0' && text[i] <= '9';
    if (valid)
    {
        meter_action_t intent =
            demo_settings_intent(u->password_admin ? DEMO_INTENT_ADMIN_LOGIN : DEMO_INTENT_USER_LOGIN,
                                 (float)strtoul(text, NULL, 10));
        u->action_failed = !u->actions.send || !u->actions.send(u->actions.context, &intent);
    }
    else
        u->action_failed = true;
    demo_editors_close(u);
}
static void logout(lv_event_t *event)
{
    demo_ui_t *u = lv_event_get_user_data(event);
    meter_action_t intent = demo_settings_intent(DEMO_INTENT_LOGOUT, 0);
    u->action_failed = !u->actions.send || !u->actions.send(u->actions.context, &intent);
    demo_editors_close(u);
}
/* 对时轮盘：六列的取值范围与显示位数。年下界高于驱动纪元哨兵，避免写入被整体拒掉。 */
static const struct
{
    unsigned first, count, digits;
} clock_specs[DEMO_CLOCK_FIELDS] = {{2021u, 79u, 4u}, {1u, 12u, 2u}, {1u, 31u, 2u},
                                    {0u, 24u, 2u},    {0u, 60u, 2u}, {0u, 60u, 2u}};
/* 单字母列标不参与翻译：轮盘本身只显示数字，因此不需要额外的可翻译文案或字体子集。 */
static const char *const clock_labels[DEMO_CLOCK_FIELDS] = {"Y", "M", "D", "h", "m", "s"};
/* 选项文本在启动时生成一次：零填充让每列宽度稳定，选中项不会左右跳动。 */
static char clock_options[DEMO_CLOCK_FIELDS][400];

static void fill_clock_options(void)
{
    for (unsigned field = 0; field < DEMO_CLOCK_FIELDS; ++field)
    {
        const unsigned first = clock_specs[field].first, count = clock_specs[field].count;
        size_t used = 0u;
        for (unsigned i = 0u; i < count; ++i)
        {
            const int written = lv_snprintf(clock_options[field] + used, sizeof(clock_options[field]) - used,
                                            "%0*u%s", (int)clock_specs[field].digits, first + i,
                                            i + 1u < count ? "\n" : "");
            if (written < 0 || (size_t)written >= sizeof(clock_options[field]) - used)
                break;
            used += (size_t)written;
        }
    }
}
/* 从当前读数（不可信时是固定基准日）铺一次轮盘。
   lv_roller_set_selected 目前不发 VALUE_CHANGED，但这里仍然加抑制位：对时页写的是
   设备时钟，将来若把轮盘绑到 subject 或换用会回发的设置接口，不能让它自己喂自己。 */
static void clock_sync(demo_ui_t *u)
{
    meter_wall_time_t local;
    (void)demo_clock_fields(&local);
    const unsigned values[DEMO_CLOCK_FIELDS] = {local.year, local.month, local.day,
                                                local.hour, local.minute, local.second};
    u->clock_syncing = true;
    for (unsigned field = 0u; field < DEMO_CLOCK_FIELDS; ++field)
    {
        const unsigned first = clock_specs[field].first;
        const unsigned index = values[field] >= first && values[field] < first + clock_specs[field].count
                                   ? values[field] - first
                                   : 0u;
        lv_roller_set_selected(u->clock_rollers[field], index, LV_ANIM_OFF);
    }
    u->clock_syncing = false;
}
/* 轮盘停在哪一格就提交哪个字段：一次手势一个意图，符合本框架"UI 提交已复制意图"的模型。 */
static void clock_roll(lv_event_t *event)
{
    demo_ui_t *u = lv_event_get_user_data(event);
    if (u->clock_syncing || lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED)
        return;
    lv_obj_t *target = lv_event_get_target_obj(event);
    for (unsigned field = 0u; field < DEMO_CLOCK_FIELDS; ++field)
    {
        if (target != u->clock_rollers[field])
            continue;
        const float value = (float)(clock_specs[field].first + lv_roller_get_selected(target));
        meter_action_t intent = demo_clock_intent((demo_clock_field_t)field, value);
        u->action_failed = !u->actions.send || !u->actions.send(u->actions.context, &intent);
    }
}
/* CAN 波特率单选：三项常驻可见，只有当前值处于 CHECKED。 */
static void rate_select(lv_event_t *event)
{
    demo_ui_t *u = lv_event_get_user_data(event);
    if (!u->view.admin_authorized)
        return;
    for (unsigned i = 0u; i < DEMO_CAN_RATE_OPTIONS; ++i)
        if (lv_event_get_target_obj(event) == u->rate_buttons[i])
        {
            meter_action_t intent = demo_admin_intent(0, (float)i);
            u->action_failed = !u->actions.send || !u->actions.send(u->actions.context, &intent);
        }
}
static void admin_change(lv_event_t *event)
{
    demo_ui_t *u = lv_event_get_user_data(event);
    if (!u->view.admin_authorized)
        return;
    if (u->selected_setting < 4 || u->selected_setting >= 4 + DEMO_ADMIN_COUNT)
        return;
    unsigned i = u->selected_setting - 4;
    unsigned value = (u->view.admin_values[i] + 1u) % (i == 0 ? 3u : 2u);
    meter_action_t intent = demo_admin_intent(i, (float)value);
    u->action_failed = !u->actions.send || !u->actions.send(u->actions.context, &intent);
}
/* 当前标签页该显示哪张卡片；用户设置的第二页只有"时钟"这一条目。 */
static void settings_show_card(demo_ui_t *u)
{
    unsigned card = DEMO_SETTINGS_CARD_USER;
    if (u->settings_tab == 1u)
        card = DEMO_SETTINGS_CARD_ADMIN;
    else if (u->settings_subpage == 1u)
        card = DEMO_SETTINGS_CARD_CLOCK;
    for (unsigned i = 0u; i < DEMO_SETTINGS_CARDS; ++i)
        lv_obj_set_hidden(u->settings_cards[i], i != card);
}
void demo_settings_show_page(demo_ui_t *u, unsigned page)
{
    if (page >= 2)
        return;
    demo_editors_close(u);
    u->settings_tab = page;
    u->settings_subpage = 0u;
    u->admin_navigation_pending = false;
    lv_obj_set_hidden(lv_obj_get_parent(u->settings_pager.indicator), false);
    u->version_open = false;
    lv_obj_set_hidden(u->version_back, true);
    /* 用户设置有两页，管理员页只有一页；翻页额度随标签页切换。 */
    u->settings_pager.count = page == 0u ? (unsigned)DEMO_USER_SETTINGS_PAGES : 1u;
    (void)demo_pager_select(&u->settings_pager, 0);
    settings_show_card(u);
    for (unsigned i = 0; i < 2; ++i)
        demo_theme_menu_button(u->settings_menu[i], i == page);
}
static void settings_menu_select(lv_event_t *event)
{
    demo_ui_t *u = lv_event_get_user_data(event);
    for (unsigned i = 0; i < 2; ++i)
        if (lv_event_get_target_obj(event) == u->settings_menu[i])
        {
            if (i == 1 && !u->view.admin_authorized)
            {
                demo_editors_close(u);
                u->admin_navigation_pending = true;
                u->password_admin = true;
                lv_textarea_set_text(u->admin_password, "");
                lv_obj_set_hidden(u->user_password, true);
                lv_obj_set_hidden(u->admin_password, false);
                lv_obj_set_hidden(u->password_editor, false);
            }
            else
                demo_settings_show_page(u, i);
        }
}
static void turn_page(lv_event_t *event)
{
    demo_ui_t *u = lv_event_get_user_data(event);
    if (!demo_pager_select(&u->settings_pager, demo_pager_target(&u->settings_pager, event)))
        return;
    /* 只有用户设置分页；管理员页 count 为 1，翻页不会生效。 */
    if (u->settings_tab == 0u)
        u->settings_subpage = u->settings_pager.current;
    settings_show_card(u);
}
static void version_open(lv_event_t *event)
{
    demo_ui_t *u = lv_event_get_user_data(event);
    u->version_open = true;
    lv_obj_set_hidden(lv_obj_get_parent(u->settings_pager.indicator), true);
    for (unsigned i = 0; i < DEMO_SETTINGS_CARDS; ++i)
        lv_obj_set_hidden(u->settings_cards[i], i != DEMO_SETTINGS_CARD_VERSION);
    lv_label_set_text(u->settings_title, demo_i18n_text(DEMO_TXT_INSTRUMENT_VERSION));
    lv_obj_set_hidden(u->version_back, false);
}
static void version_close(lv_event_t *event)
{
    demo_ui_t *u = lv_event_get_user_data(event);
    (void)event;
    lv_obj_set_hidden(u->version_back, true);
    demo_settings_show_page(u, 0);
}
static lv_obj_t *settings_button(demo_ui_t *u, lv_obj_t *parent, int x, int y, demo_text_id_t text,
                                 lv_event_cb_t callback)
{
    lv_obj_t *button = lv_button_create(parent);
    demo_theme_list_row(button);
    lv_obj_set_style_text_color(button, lv_color_hex(0xedf5f8), 0);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_size(button, 250, 48);
    lv_obj_t *label = demo_text(u, button, 0, 0, text, &lv_font_montserrat_16, 0xedf5f8);
    lv_obj_center(label);
    if (callback)
        lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, u);
    return button;
}
void demo_settings_create(demo_ui_t *u)
{
    lv_obj_t *p = u->pages[DEMO_SETTINGS];
    u->settings_title = meter_text(p, 430, 8, "", &lv_font_montserrat_24, 0xedf5f8);
    u->settings_note = meter_text(p, 24, 34, "", &lv_font_montserrat_20, 0x8ba9bb);
    lv_obj_set_hidden(u->settings_note, true);
    /* 左侧背景铺满内容区，分类按钮保持固定高度。 */
    u->settings_rail = lv_obj_create(p);
    lv_obj_remove_style_all(u->settings_rail);
    lv_obj_set_pos(u->settings_rail, 0, 0);
    lv_obj_set_size(u->settings_rail, 200, 372);
    lv_obj_set_style_bg_color(u->settings_rail, lv_color_hex(0x1a242b), 0);
    lv_obj_set_style_bg_opa(u->settings_rail, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(u->settings_rail, 0, 0);
    lv_obj_set_style_outline_width(u->settings_rail, 0, 0);
    lv_obj_set_scrollable(u->settings_rail, false);
    const demo_text_id_t menu_ids[] = {DEMO_TXT_USER_SETTINGS, DEMO_TXT_ADMIN_SETTINGS};
    for (unsigned i = 0; i < 2; ++i)
    {
        u->settings_menu[i] = lv_button_create(u->settings_rail);
        demo_theme_menu_button(u->settings_menu[i], i == 0);
        lv_obj_set_pos(u->settings_menu[i], 0, (int)i * 64);
        lv_obj_set_size(u->settings_menu[i], 200, 64);
        lv_obj_add_event_cb(u->settings_menu[i], settings_menu_select, LV_EVENT_CLICKED, u);
        lv_obj_t *label =
            demo_text(u, u->settings_menu[i], 12, 0, menu_ids[i], &lv_font_montserrat_16, 0xedf5f8);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(label, 176);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(label);
    }
    for (unsigned i = 0; i < DEMO_SETTINGS_CARDS; ++i)
    {
        u->settings_cards[i] = lv_obj_create(p);
        lv_obj_remove_style_all(u->settings_cards[i]);
        lv_obj_set_pos(u->settings_cards[i], 200, DEMO_LIST_TOP);
        lv_obj_set_size(u->settings_cards[i], 584, DEMO_LIST_CAPACITY * DEMO_LIST_PITCH);
        lv_obj_set_scrollable(u->settings_cards[i], false);
    }
    lv_obj_t *card = u->settings_cards[0];
    for (unsigned i = 0; i < 4; ++i)
    {
        u->setting_entries[i] =
            settings_button(u, card, 0, (int)i * DEMO_LIST_PITCH, setting_titles[i], settings_detail_open);
        lv_obj_set_size(u->setting_entries[i], 584, DEMO_LIST_HEIGHT);
        lv_obj_t *label = lv_obj_get_child(u->setting_entries[i], 0);
        lv_obj_set_width(label, 280);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 18, 0);
        u->setting_values[i] = meter_text(u->setting_entries[i], 0, 0, "", &lv_font_montserrat_20, 0xedf5f8);
        lv_obj_set_width(u->setting_values[i], 236);
        lv_obj_set_style_text_align(u->setting_values[i], LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_align(u->setting_values[i], LV_ALIGN_RIGHT_MID, -30, 0);
    }
    u->version_entry =
        settings_button(u, card, 0, 4 * DEMO_LIST_PITCH, DEMO_TXT_INSTRUMENT_VERSION, version_open);
    lv_obj_set_size(u->version_entry, 584, DEMO_LIST_HEIGHT);
    lv_obj_align(lv_obj_get_child(u->version_entry, 0), LV_ALIGN_LEFT_MID, 18, 0);
    /* 详情控件固定归属于详情页，避免搬移对象破坏列表和触摸事件。 */
    u->settings_detail = lv_obj_create(p);
    lv_obj_remove_style_all(u->settings_detail);
    lv_obj_set_pos(u->settings_detail, 200, 0);
    lv_obj_set_size(u->settings_detail, 600, 372);
    lv_obj_set_style_bg_color(u->settings_detail, lv_color_hex(0x07090b), 0);
    lv_obj_set_style_bg_opa(u->settings_detail, LV_OPA_COVER, 0);
    lv_obj_set_scrollable(u->settings_detail, false);
    lv_obj_set_parent(u->settings_note, u->settings_detail);
    lv_obj_set_pos(u->settings_note, 18, 300);
    lv_obj_set_width(u->settings_note, 560);
    demo_i18n_bind_label(u->settings_note, DEMO_TXT_ADMIN_NOTE);
    u->settings_detail_title = meter_text(u->settings_detail, 18, 20, "", &lv_font_montserrat_20, 0xedf5f8);
    lv_obj_set_width(u->settings_detail_title, 470);
    lv_label_set_long_mode(u->settings_detail_title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    u->settings_detail_back = lv_button_create(u->settings_detail);
    demo_theme_menu_button(u->settings_detail_back, false);
    lv_obj_set_pos(u->settings_detail_back, 514, 8);
    lv_obj_set_size(u->settings_detail_back, 70, 44);
    lv_obj_t *detail_back_label =
        meter_text(u->settings_detail_back, 0, 0, LV_SYMBOL_LEFT, &lv_font_montserrat_24, 0xedf5f8);
    lv_obj_center(detail_back_label);
    lv_obj_add_event_cb(u->settings_detail_back, settings_detail_close, LV_EVENT_CLICKED, u);
    u->settings_detail_value = meter_text(u->settings_detail, 42, 86, "", &lv_font_montserrat_20, 0xedf5f8);
    lv_obj_set_width(u->settings_detail_value, 500);
    lv_obj_set_style_text_align(u->settings_detail_value, LV_TEXT_ALIGN_CENTER, 0);
    u->unit_button = settings_button(u, u->settings_detail, 166, 160, DEMO_TXT_METRIC, action);
    u->language_button = settings_button(u, u->settings_detail, 166, 160, DEMO_TXT_CHINESE, action);
    u->admin_detail_button =
        settings_button(u, u->settings_detail, 166, 160, DEMO_TXT_EDIT_VALUE, admin_change);
    /* CAN 波特率单选：三项并排常驻，文案就是速率本身，不参与翻译。 */
    static const char *const rate_labels[DEMO_CAN_RATE_OPTIONS] = {"125 kbit/s", "250 kbit/s", "500 kbit/s"};
    for (unsigned i = 0; i < DEMO_CAN_RATE_OPTIONS; ++i)
    {
        lv_obj_t *button = lv_button_create(u->settings_detail);
        demo_theme_button(button);
        lv_obj_set_pos(button, 20 + (int)i * 186, 160);
        lv_obj_set_size(button, 170, 48);
        lv_obj_t *label = meter_text(button, 0, 0, rate_labels[i], &lv_font_montserrat_16, 0xedf5f8);
        lv_obj_center(label);
        lv_obj_add_event_cb(button, rate_select, LV_EVENT_CLICKED, u);
        lv_obj_set_hidden(button, true);
        u->rate_buttons[i] = button;
    }
    u->brightness = lv_slider_create(u->settings_detail);
    u->limit = lv_slider_create(u->settings_detail);
    lv_obj_t *sliders[] = {u->brightness, u->limit};
    for (unsigned i = 0; i < 2; ++i)
    {
        demo_theme_slider(sliders[i]);
        lv_obj_set_pos(sliders[i], 70, 180);
        lv_obj_set_size(sliders[i], 444, 16);
        lv_obj_add_event_cb(sliders[i], action, LV_EVENT_RELEASED, u);
    }
    lv_slider_set_range(u->brightness, 10, 100);
    lv_slider_set_range(u->limit, 5, 50);
    lv_obj_set_hidden(u->settings_detail, true);
    card = u->settings_cards[1];
    u->user_password_button = settings_button(u, card, 22, 24, DEMO_TXT_USER_PASSWORD, password_open);
    u->admin_password_button = settings_button(u, card, 290, 24, DEMO_TXT_ADMIN_PASSWORD, password_open);
    u->password_status = meter_text(card, 22, 92, "", &lv_font_montserrat_20, 0x5de5ca);
    u->logout_button = settings_button(u, card, 290, 92, DEMO_TXT_SIGN_OUT, logout);
    /* 参考参考产品密码页：左侧设置导航，右侧标题、输入框和 3x4 数字键盘。 */
    u->password_editor = lv_obj_create(u->root);
    lv_obj_remove_style_all(u->password_editor);
    lv_obj_set_pos(u->password_editor, 0, 53);
    lv_obj_set_size(u->password_editor, 800, 372);
    lv_obj_set_style_bg_color(u->password_editor, lv_color_hex(0x11161b), 0);
    lv_obj_set_style_bg_opa(u->password_editor, LV_OPA_COVER, 0);
    lv_obj_set_scrollable(u->password_editor, false);
    lv_obj_t *side = lv_obj_create(u->password_editor);
    lv_obj_remove_style_all(side);
    lv_obj_set_pos(side, 0, 0);
    lv_obj_set_size(side, 200, 372);
    lv_obj_set_style_bg_color(side, lv_color_hex(0x1a242b), 0);
    lv_obj_set_style_bg_opa(side, LV_OPA_COVER, 0);

    lv_obj_t *side_system = lv_button_create(side);
    demo_theme_menu_button(side_system, false);
    lv_obj_set_pos(side_system, 0, 0);
    lv_obj_set_size(side_system, 200, 64);
    lv_obj_t *system_label =
        demo_text(u, side_system, 18, 0, DEMO_TXT_USER_SETTINGS, &lv_font_montserrat_16, 0xedf5f8);
    lv_obj_set_width(system_label, 176);
    lv_label_set_long_mode(system_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(system_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(system_label);
    lv_obj_t *side_advanced = lv_button_create(side);
    demo_theme_menu_button(side_advanced, true);
    lv_obj_set_pos(side_advanced, 0, 64);
    lv_obj_set_size(side_advanced, 200, 64);
    lv_obj_t *side_label =
        demo_text(u, side_advanced, 4, 0, DEMO_TXT_ADMIN_SETTINGS, &lv_font_montserrat_14, 0xffffff);
    lv_obj_set_width(side_label, 176);
    lv_label_set_long_mode(side_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(side_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(side_label);
    u->password_editor_title =
        demo_text(u, u->password_editor, 320, 16, DEMO_TXT_ENTER_PIN, &lv_font_montserrat_24, 0xedf5f8);
    lv_obj_t *back = lv_button_create(u->password_editor);
    demo_theme_button(back);
    lv_obj_set_pos(back, 710, 10);
    lv_obj_set_size(back, 70, 42);
    lv_obj_set_style_bg_opa(back, LV_OPA_TRANSP, 0);
    lv_obj_t *back_label = meter_text(back, 0, 0, LV_SYMBOL_LEFT, &lv_font_montserrat_24, 0xedf5f8);
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back, password_back, LV_EVENT_CLICKED, u);
    u->user_password = lv_textarea_create(u->password_editor);
    u->admin_password = lv_textarea_create(u->password_editor);
    lv_obj_t *fields[] = {u->user_password, u->admin_password};
    for (unsigned i = 0; i < 2; ++i)
    {
        lv_textarea_set_one_line(fields[i], true);
        lv_textarea_set_password_mode(fields[i], true);
        lv_textarea_set_password_show_time(fields[i], 0);
        lv_textarea_set_accepted_chars(fields[i], "0123456789");
        lv_textarea_set_max_length(fields[i], 4);
        lv_obj_set_pos(fields[i], 320, 56);
        lv_obj_set_size(fields[i], 400, 58);
        lv_obj_set_style_text_align(fields[i], LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_style_text_font(fields[i], &lv_font_montserrat_24, 0);
        lv_obj_set_style_bg_color(fields[i], lv_color_hex(0x1a242b), 0);
        lv_obj_set_style_text_color(fields[i], lv_color_hex(0xedf5f8), 0);
        lv_obj_set_style_border_color(fields[i], lv_color_hex(0x53616b), 0);
        lv_obj_set_style_border_width(fields[i], 1, 0);
        lv_obj_set_style_radius(fields[i], 6, 0);
    }
    u->password_keyboard = lv_buttonmatrix_create(u->password_editor);
    demo_theme_keyboard(u->password_keyboard);
    static const char *const password_map[] = {
        "1", "2",          "3", "\n", "4", "5", "6", "\n", "7", "8", "9", "\n", LV_SYMBOL_BACKSPACE,
        "0", LV_SYMBOL_OK, ""};
    lv_buttonmatrix_set_map(u->password_keyboard, password_map);
    lv_obj_set_pos(u->password_keyboard, 320, 126);
    lv_obj_set_size(u->password_keyboard, 400, 230);
    lv_obj_add_event_cb(u->password_keyboard, password_keypad, LV_EVENT_VALUE_CHANGED, u);
    lv_obj_add_event_cb(u->password_keyboard, password_submit, LV_EVENT_READY, u);
    lv_obj_add_event_cb(u->password_keyboard, password_submit, LV_EVENT_CANCEL, u);
    card = u->settings_cards[2];
    u->admin_locked = meter_text(card, 0, 92, LV_SYMBOL_WARNING, &lv_font_montserrat_24, 0xf3ba65);
    lv_obj_set_width(u->admin_locked, 544);
    lv_obj_set_style_text_align(u->admin_locked, LV_TEXT_ALIGN_CENTER, 0);
    const demo_text_id_t admin_ids[] = {DEMO_TXT_CAN_RATE, DEMO_TXT_HOUR_METER, DEMO_TXT_SPEED_DISPLAY,
                                        DEMO_TXT_MODE_MEMORY};
    for (unsigned i = 0; i < DEMO_ADMIN_COUNT; ++i)
    {
        u->admin_items[i] =
            settings_button(u, card, 0, (int)i * DEMO_LIST_PITCH, admin_ids[i], settings_detail_open);
        lv_obj_set_size(u->admin_items[i], 584, DEMO_LIST_HEIGHT);
        lv_obj_t *label = lv_obj_get_child(u->admin_items[i], 0);
        lv_obj_set_width(label, 280);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 18, 0);
        u->admin_value_labels[i] = meter_text(u->admin_items[i], 0, 0, "", &lv_font_montserrat_20, 0xedf5f8);
        lv_obj_set_width(u->admin_value_labels[i], 236);
        lv_obj_set_style_text_align(u->admin_value_labels[i], LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_align(u->admin_value_labels[i], LV_ALIGN_RIGHT_MID, -30, 0);
        lv_obj_set_hidden(u->admin_items[i], true);
    }
    card = u->settings_cards[3];
    const demo_text_id_t version_ids[] = {DEMO_TXT_FIRMWARE, DEMO_TXT_PRODUCT, DEMO_TXT_FRAMEWORK,
                                          DEMO_TXT_INSTRUMENT_VERSION};
    for (unsigned i = 0; i < 4; ++i)
    {
        u->version_labels[i] = i == 3 ? meter_text(card, 22, 18 + (int)i * DEMO_LIST_PITCH, "LVGL",
                                                   &lv_font_montserrat_20, 0x9cb5c4)
                                      : demo_text(u, card, 22, 18 + (int)i * DEMO_LIST_PITCH, version_ids[i],
                                                  &lv_font_montserrat_20, 0x9cb5c4);
        u->version_values[i] =
            meter_text(card, 290, 18 + (int)i * DEMO_LIST_PITCH, "", &lv_font_montserrat_20, 0xedf5f8);
        lv_obj_set_width(u->version_values[i], 276);
        lv_label_set_long_mode(u->version_values[i], LV_LABEL_LONG_SCROLL_CIRCULAR);
    }
    lv_label_set_text(u->version_labels[3], "LVGL");
    u->version_back = lv_button_create(p);
    demo_theme_menu_button(u->version_back, false);
    lv_obj_set_pos(u->version_back, 710, 8);
    lv_obj_set_size(u->version_back, 70, 42);
    lv_obj_t *version_back_label =
        meter_text(u->version_back, 0, 0, LV_SYMBOL_LEFT, &lv_font_montserrat_24, 0xedf5f8);
    lv_obj_center(version_back_label);
    lv_obj_add_event_cb(u->version_back, version_close, LV_EVENT_CLICKED, u);
    lv_obj_set_hidden(u->version_back, true);
    /* 用户设置第二页：只有"时钟"这一条目；点进去才是六个轮盘的对时界面。 */
    fill_clock_options();
    lv_obj_t *clock_card = u->settings_cards[DEMO_SETTINGS_CARD_CLOCK];
    u->clock_entry = settings_button(u, clock_card, 0, 0, DEMO_TXT_CLOCK_SET, settings_detail_open);
    lv_obj_set_size(u->clock_entry, 584, DEMO_LIST_HEIGHT);
    lv_obj_t *clock_name = lv_obj_get_child(u->clock_entry, 0);
    lv_obj_set_width(clock_name, 280);
    lv_label_set_long_mode(clock_name, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_align(clock_name, LV_ALIGN_LEFT_MID, 18, 0);
    u->clock_value = meter_text(u->clock_entry, 0, 0, "", &lv_font_montserrat_20, 0xedf5f8);
    lv_obj_set_width(u->clock_value, 236);
    lv_obj_set_style_text_align(u->clock_value, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(u->clock_value, LV_ALIGN_RIGHT_MID, -30, 0);
    /* 对时轮盘归详情页所有：从"时钟"条目进入才显示，六列依次是年月日时分秒。 */
    u->clock_group = lv_obj_create(u->settings_detail);
    lv_obj_remove_style_all(u->clock_group);
    lv_obj_set_pos(u->clock_group, 0, 0);
    lv_obj_set_size(u->clock_group, 600, 372);
    lv_obj_set_scrollable(u->clock_group, false);
    for (unsigned field = 0; field < DEMO_CLOCK_FIELDS; ++field)
    {
        /* 列标只用单字母：轮盘本身显示数字，因此不需要额外的可翻译文案或字体子集。 */
        lv_obj_t *caption = meter_text(u->clock_group, 20 + (int)field * 92, 118, clock_labels[field],
                                       &lv_font_montserrat_16, 0x8ba9bb);
        lv_obj_set_width(caption, 84);
        lv_obj_set_style_text_align(caption, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_t *roller = lv_roller_create(u->clock_group);
        demo_theme_roller(roller);
        lv_roller_set_options(roller, clock_options[field], LV_ROLLER_MODE_NORMAL);
        /* 三行刚好放下上一项、当前项、下一项；高度由行数决定，避免半行字形被裁掉。 */
        lv_roller_set_visible_row_count(roller, 3);
        lv_obj_set_pos(roller, 20 + (int)field * 92, 142);
        lv_obj_set_width(roller, 84);
        lv_obj_add_event_cb(roller, clock_roll, LV_EVENT_VALUE_CHANGED, u);
        u->clock_rollers[field] = roller;
    }
    lv_obj_set_hidden(u->clock_group, true);
    u->setting_status = meter_text(u->root, 218, 393, "", &lv_font_montserrat_16, 0xff856d);

    demo_pager_create(p, &u->settings_pager, (unsigned)DEMO_USER_SETTINGS_PAGES, turn_page, u);
    lv_obj_move_foreground(u->settings_detail);
    demo_settings_show_page(u, 0);
}
void demo_settings_update(demo_ui_t *u)
{
    lv_obj_set_hidden(u->settings_note, u->selected_setting != 4);
    const lv_font_t *font = u->view.language == METER_LANGUAGE_EN
                                ? &lv_font_montserrat_20
                                : meter_font_get(u->view.language, METER_FONT_LABEL);
    if (u->admin_navigation_pending && u->view.admin_authorized)
    {
        u->admin_navigation_pending = false;
        demo_settings_show_page(u, 1);
    }
    if (!u->view.admin_authorized && u->settings_tab == 1)
        demo_settings_show_page(u, 0);
    /* 标题只在版本页与用户设置第二页（对时）有意义；其余页面留空，避免与列表标题重复。 */
    const bool clock_page = !u->version_open && u->settings_tab == 0u && u->settings_subpage == 1u;
    lv_label_set_text(u->settings_title,
                      u->version_open  ? demo_i18n_text(DEMO_TXT_INSTRUMENT_VERSION)
                      : clock_page     ? demo_i18n_text(DEMO_TXT_CLOCK_SET)
                                       : "");
    /* 第二页的"时钟"条目显示当前本地时间；读数不可信时是占位符而不是 00:00。 */
    char clock_buffer[8];
    (void)demo_clock_text(clock_buffer, sizeof(clock_buffer));
    lv_label_set_text(u->clock_value, clock_buffer);
    lv_label_set_text(lv_obj_get_child(u->unit_button, 0),
                      demo_i18n_text(u->view.imperial ? DEMO_TXT_METRIC : DEMO_TXT_IMPERIAL));
    lv_label_set_text(
        lv_obj_get_child(u->language_button, 0),
        demo_i18n_text(u->view.language == METER_LANGUAGE_EN ? DEMO_TXT_CHINESE : DEMO_TXT_ENGLISH));
    if (!lv_obj_has_state(u->brightness, LV_STATE_PRESSED))
        lv_slider_set_value(u->brightness, u->view.brightness, LV_ANIM_OFF);
    lv_obj_set_state(u->limit, LV_STATE_DISABLED, !u->view.limit_available);
    if (u->view.limit_available && !lv_obj_has_state(u->limit, LV_STATE_PRESSED))
        lv_slider_set_value(u->limit, (int)u->view.limit, LV_ANIM_OFF);
    lv_obj_set_hidden(u->setting_status, u->page != DEMO_SETTINGS);
    lv_label_set_text(u->setting_status, u->action_failed ? demo_i18n_text(DEMO_TXT_ACCESS_DENIED) : "");
    demo_text_id_t access = u->view.admin_authorized  ? DEMO_TXT_ACCESS_ADMIN
                            : u->view.user_authorized ? DEMO_TXT_ACCESS_USER
                                                      : demo_i18n_feedback(u->view.auth_feedback);
    lv_label_set_text(u->password_status, demo_i18n_text(access));
    lv_obj_set_hidden(u->admin_locked, u->view.admin_authorized);
    const char *rates[] = {"125 kbit/s", "250 kbit/s", "500 kbit/s"};
    for (unsigned i = 0; i < DEMO_ADMIN_COUNT; ++i)
    {
        lv_obj_set_hidden(u->admin_items[i], !u->view.admin_authorized);
        unsigned value = u->view.admin_values[i];
        const char *text = i == 0   ? rates[value < 3 ? value : 0]
                           : i == 1 ? demo_i18n_text(value ? DEMO_TXT_POWER_MODE : DEMO_TXT_WORK_MODE)
                           : i == 2 ? (value ? "1 km/h" : "0.1 km/h")
                                    : demo_i18n_text(value ? DEMO_TXT_REMEMBER : DEMO_TXT_RESET_MODE);
        lv_label_set_text(u->admin_value_labels[i], text);
    }
    /* CAN 波特率单选：文案就是速率本身，只有当前值处于 CHECKED。 */
    for (unsigned i = 0; i < DEMO_CAN_RATE_OPTIONS; ++i)
        lv_obj_set_state(u->rate_buttons[i], LV_STATE_CHECKED, u->view.admin_values[0] == i);
    lv_label_set_text(u->version_values[0], u->view.firmware_version ? u->view.firmware_version
                                                                     : demo_i18n_text(DEMO_TXT_HOST_BUILD));
    lv_label_set_text(u->version_values[1], "reference-demo");
    lv_label_set_text_fmt(u->version_values[2], "%.8s%s", u->view.framework_revision,
                          strstr(u->view.framework_revision, "dirty") ? " *" : "");
    lv_label_set_text_fmt(u->version_values[3], "%d.%d.%d", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR,
                          LVGL_VERSION_PATCH);
    lv_label_set_text(u->setting_values[0],
                      demo_i18n_text(u->view.imperial ? DEMO_TXT_IMPERIAL : DEMO_TXT_METRIC));
    lv_label_set_text(
        u->setting_values[1],
        demo_i18n_text(u->view.language == METER_LANGUAGE_EN ? DEMO_TXT_ENGLISH : DEMO_TXT_CHINESE));
    lv_label_set_text_fmt(u->setting_values[2], "%u %%", (unsigned)u->view.brightness);
    if (u->view.limit_available)
        lv_label_set_text_fmt(u->setting_values[3], "%.0f km/h", (double)u->view.limit);
    else
        lv_label_set_text(u->setting_values[3], "--");
    for (unsigned i = 0; i < 4; ++i)
        lv_obj_set_style_text_font(u->setting_values[i], font, 0);
    lv_obj_set_style_text_font(u->clock_value, font, 0);
    if (u->selected_setting == DEMO_SETTING_CLOCK)
    {
        /* 对时详情页：标题与当前值都用时钟这一项自己的文本。 */
        lv_label_set_text(u->settings_detail_title, demo_i18n_text(DEMO_TXT_CLOCK_SET));
        lv_label_set_text(u->settings_detail_value, lv_label_get_text(u->clock_value));
    }
    else if (u->selected_setting < 4 + DEMO_ADMIN_COUNT)
    {
        lv_label_set_text(u->settings_detail_title, demo_i18n_text(setting_titles[u->selected_setting]));
        lv_obj_t *value = u->selected_setting < 4 ? u->setting_values[u->selected_setting]
                                                  : u->admin_value_labels[u->selected_setting - 4];
        lv_label_set_text(u->settings_detail_value, lv_label_get_text(value));
    }
    lv_obj_t *dynamic[] = {u->settings_title,
                           u->settings_note,
                           u->password_status,
                           u->setting_status,
                           u->settings_detail_title,
                           u->settings_detail_value,
                           lv_obj_get_child(u->unit_button, 0),
                           lv_obj_get_child(u->language_button, 0)};
    for (unsigned i = 0; i < sizeof(dynamic) / sizeof(dynamic[0]); ++i)
        lv_obj_set_style_text_font(dynamic[i], font, 0);
    for (unsigned i = 0; i < 2; ++i)
        meter_i18n_apply_font(lv_obj_get_child(u->settings_menu[i], 0), u->view.language, METER_FONT_LABEL);
    for (unsigned i = 0; i < 4; ++i)
    {
        lv_obj_set_style_text_font(u->admin_value_labels[i], font, 0);
        lv_obj_set_style_text_font(u->version_values[i], font, 0);
    }
}
