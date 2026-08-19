#include "AssistantClient.h"
#include <ArduinoJson.h>

AssistantClient* AssistantClient::_instance = nullptr;

void AssistantClient::begin(const char* serverUrl, uint16_t port) {
    _instance = this;
    strncpy(_serverUrl, serverUrl, sizeof(_serverUrl) - 1);
    _serverPort = port;
}

void AssistantClient::connect() {
    _ws.begin(_serverUrl, _serverPort, "/ws/audio");
    _ws.onEvent(webSocketEvent);
    _ws.setReconnectInterval(5000);
    _state = AssistantState::CONNECTED;
    if (_callback) _callback(_state, "connected");
}

void AssistantClient::disconnect() {
    _ws.disconnect();
    _state = AssistantState::DISCONNECTED;
}

void AssistantClient::startListening() {
    if (_state != AssistantState::CONNECTED) return;

    DynamicJsonDocument doc(128);
    doc["type"] = "start";
    String msg;
    serializeJson(doc, msg);
    _ws.sendTXT(msg);

    _state = AssistantState::LISTENING;
    _isStreaming = true;
    if (_callback) _callback(_state, "listening");
}

void AssistantClient::stopListening() {
    if (!_isStreaming) return;

    DynamicJsonDocument doc(128);
    doc["type"] = "end";
    String msg;
    serializeJson(doc, msg);
    _ws.sendTXT(msg);

    _isStreaming = false;
    _state = AssistantState::PROCESSING;
    if (_callback) _callback(_state, "processing");
}

void AssistantClient::update() {
    _ws.loop();

    // TODO: Read microphone samples and send via sendAudioChunk()
    // This depends on Bruce's microphone driver
}

void AssistantClient::webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    if (_instance) {
        _instance->handleWebSocketEvent(type, payload, length);
    }
}

void AssistantClient::handleWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            _state = AssistantState::CONNECTED;
            if (_callback) _callback(_state, "connected");
            break;

        case WStype_TEXT: {
            DynamicJsonDocument doc(1024);
            DeserializationError err = deserializeJson(doc, payload, length);
            if (err) return;

            const char* msgType = doc["type"];
            if (strcmp(msgType, "text") == 0) {
                const char* text = doc["content"];
                if (_callback) _callback(AssistantState::SPEAKING, text ? text : "");
            } else if (strcmp(msgType, "audio") == 0) {
                _state = AssistantState::SPEAKING;
            }
            break;
        }

        case WStype_DISCONNECTED:
            _state = AssistantState::DISCONNECTED;
            if (_callback) _callback(_state, "disconnected");
            break;

        default:
            break;
    }
}

void AssistantClient::sendAudioChunk(const int16_t* samples, size_t count) {
    _ws.sendBIN((const uint8_t*)samples, count * sizeof(int16_t));
}
