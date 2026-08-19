#include "ANCSDisplay.h"
#include <cstdio>

void ANCSDisplay::begin(lv_obj_t* parent) {
    _container = parent;
    createUI();
}

void ANCSDisplay::createUI() {
    lv_obj_set_flex_flow(_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(_container, 10, 0);

    lv_obj_t* header = lv_obj_create(_container);
    lv_obj_set_size(header, LV_PCT(100), 30);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* titleLabel = lv_label_create(header);
    lv_label_set_text(titleLabel, LV_SYMBOL_BELL " Notificações");
    lv_obj_set_style_text_color(titleLabel, lv_color_white(), 0);
    lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_14, 0);

    _countLabel = lv_label_create(header);
    lv_label_set_text(_countLabel, "0");
    lv_obj_set_style_text_color(_countLabel, lv_color_hex(0x00BFFF), 0);
    lv_obj_set_style_text_font(_countLabel, &lv_font_montserrat_14, 0);

    _list = lv_list_create(_container);
    lv_obj_set_size(_list, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(_list, 1);
}

void ANCSDisplay::addNotification(const Notification& notif) {
    if (_notifCount < MAX_NOTIFICATIONS) {
        _notifications[_notifCount++] = notif;
    } else {
        for (int i = 0; i < MAX_NOTIFICATIONS - 1; i++) {
            _notifications[i] = _notifications[i + 1];
        }
        _notifications[MAX_NOTIFICATIONS - 1] = notif;
    }

    _unreadCount++;
    refreshList();
}

void ANCSDisplay::refreshList() {
    lv_obj_clean(_list);

    char buf[20];
    snprintf(buf, sizeof(buf), "%d", _unreadCount);
    lv_label_set_text(_countLabel, buf);

    for (int i = _notifCount - 1; i >= 0; i--) {
        lv_obj_t* btn = lv_list_add_btn(_list, LV_SYMBOL_BELL, _notifications[i].title);

        lv_obj_t* subLabel = lv_label_create(btn);
        char subBuf[160];
        snprintf(subBuf, sizeof(subBuf), "%s — %s", _notifications[i].app, _notifications[i].message);
        lv_label_set_text(subLabel, subBuf);
        lv_label_set_long_mode(subLabel, LV_LABEL_LONG_DOT);
        lv_obj_set_width(subLabel, 180);
        lv_obj_set_style_text_color(subLabel, lv_color_hex(0xAAAAAA), 0);
        lv_obj_set_style_text_font(subLabel, &lv_font_montserrat_10, 0);
    }
}

void ANCSDisplay::update() {
    // Future: tap-to-expand, swipe-to-dismiss
}
