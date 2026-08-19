#pragma once
#include <lvgl.h>
#include "../assistant/AssistantClient.h"

class AssistantUI {
public:
    void begin(lv_obj_t* parent, AssistantClient* client);
    void update();

private:
    lv_obj_t* _container = nullptr;
    lv_obj_t* _statusLabel = nullptr;
    lv_obj_t* _userTextLabel = nullptr;
    lv_obj_t* _responseLabel = nullptr;
    AssistantClient* _client = nullptr;

    void createUI();
    void onAssistantEvent(AssistantState state, const char* data);
};
