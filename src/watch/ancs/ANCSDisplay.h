#pragma once
#include <lvgl.h>

struct Notification {
    char app[32];
    char title[64];
    char message[128];
    uint32_t timestamp;
};

class ANCSDisplay {
public:
    void begin(lv_obj_t* parent);
    void addNotification(const Notification& notif);
    void update();
    int unreadCount() const { return _unreadCount; }

private:
    static const int MAX_NOTIFICATIONS = 10;
    lv_obj_t* _container = nullptr;
    lv_obj_t* _list = nullptr;
    lv_obj_t* _countLabel = nullptr;
    Notification _notifications[MAX_NOTIFICATIONS] = {};
    int _notifCount = 0;
    int _unreadCount = 0;

    void createUI();
    void refreshList();
};
