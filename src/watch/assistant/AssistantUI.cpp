#include "AssistantUI.h"

void AssistantUI::begin(lv_obj_t* parent, AssistantClient* client) {
    _client = client;
    _container = parent;
    createUI();

    _client->onEvent([this](AssistantState state, const char* data) {
        onAssistantEvent(state, data);
    });
}

void AssistantUI::createUI() {
    lv_obj_set_flex_flow(_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(_container, 10, 0);

    _statusLabel = lv_label_create(_container);
    lv_label_set_text(_statusLabel, LV_SYMBOL_WIFI " Conectado");
    lv_obj_set_style_text_color(_statusLabel, lv_color_hex(0x00FF00), 0);

    _userTextLabel = lv_label_create(_container);
    lv_label_set_text(_userTextLabel, "");
    lv_obj_set_width(_userTextLabel, LV_PCT(100));
    lv_label_set_long_mode(_userTextLabel, LV_LABEL_LONG_WRAP);

    _responseLabel = lv_label_create(_container);
    lv_label_set_text(_responseLabel, "");
    lv_obj_set_width(_responseLabel, LV_PCT(100));
    lv_label_set_long_mode(_responseLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(_responseLabel, lv_color_hex(0x00BFFF), 0);
}

void AssistantUI::onAssistantEvent(AssistantState state, const char* data) {
    switch (state) {
        case AssistantState::LISTENING:
            lv_label_set_text(_statusLabel, LV_SYMBOL_AUDIO " Ouvindo...");
            lv_obj_set_style_text_color(_statusLabel, lv_color_hex(0xFFFF00), 0);
            break;
        case AssistantState::PROCESSING:
            lv_label_set_text(_statusLabel, LV_SYMBOL_REFRESH " Pensando...");
            lv_obj_set_style_text_color(_statusLabel, lv_color_hex(0xFF8800), 0);
            break;
        case AssistantState::SPEAKING:
            lv_label_set_text(_statusLabel, LV_SYMBOL_VOLUME_MAX " Falando...");
            lv_obj_set_style_text_color(_statusLabel, lv_color_hex(0x00BFFF), 0);
            lv_label_set_text(_responseLabel, data);
            break;
        case AssistantState::CONNECTED:
            lv_label_set_text(_statusLabel, LV_SYMBOL_WIFI " Conectado");
            lv_obj_set_style_text_color(_statusLabel, lv_color_hex(0x00FF00), 0);
            break;
        case AssistantState::DISCONNECTED:
            lv_label_set_text(_statusLabel, LV_SYMBOL_CLOSE " Desconectado");
            lv_obj_set_style_text_color(_statusLabel, lv_color_hex(0xFF0000), 0);
            break;
        case AssistantState::ERROR:
            lv_label_set_text(_statusLabel, LV_SYMBOL_WARNING " Erro");
            lv_obj_set_style_text_color(_statusLabel, lv_color_hex(0xFF0000), 0);
            break;
    }
}

void AssistantUI::update() {
    // Future: handle touch input for push-to-talk
}
