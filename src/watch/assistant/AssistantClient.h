#pragma once
#include <Arduino.h>
#include <functional>
#include <WiFi.h>
#include <WebSocketsClient.h>

enum class AssistantState {
    DISCONNECTED,
    CONNECTED,
    LISTENING,
    PROCESSING,
    SPEAKING,
    ERROR
};

using AssistantCallback = std::function<void(AssistantState state, const char* data)>;

class AssistantClient {
public:
    void begin(const char* serverUrl, uint16_t port);
    void connect();
    void disconnect();
    void startListening();
    void stopListening();
    void update();  // Call in loop
    AssistantState state() const { return _state; }
    void onEvent(AssistantCallback cb) { _callback = cb; }

private:
    WebSocketsClient _ws;
    AssistantState _state = AssistantState::DISCONNECTED;
    AssistantCallback _callback = nullptr;
    char _serverUrl[64] = {0};
    uint16_t _serverPort = 8000;
    bool _isStreaming = false;

    static void webSocketEvent(WStype_t type, uint8_t* payload, size_t length);
    static AssistantClient* _instance;  // For static callback
    void handleWebSocketEvent(WStype_t type, uint8_t* payload, size_t length);
    void sendAudioChunk(const int16_t* samples, size_t count);
};
