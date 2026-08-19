#include "MusicPlayer.h"
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

static WebSocketsClient webSocket;
static MusicPlayer* instance_ = nullptr;

static void webSocketEventCB(WStype_t type, uint8_t* payload, size_t length) {
    if (!instance_) return;

    switch (type) {
        case WStype_CONNECTED:
            break;
        case WStype_DISCONNECTED:
            break;
        case WStype_TEXT:
            if (payload && length > 0) {
                char buf[512];
                size_t copyLen = length < sizeof(buf) - 1 ? length : sizeof(buf) - 1;
                memcpy(buf, payload, copyLen);
                buf[copyLen] = '\0';
                instance_->handleTrackInfo(buf);
            }
            break;
        default:
            break;
    }
}

void MusicPlayer::begin(const char* serverUrl, uint16_t port) {
    strncpy(serverUrl_, serverUrl, sizeof(serverUrl_) - 1);
    serverUrl_[sizeof(serverUrl_) - 1] = '\0';
    serverPort_ = port;
    instance_ = this;

    memset(&track_, 0, sizeof(track_));
    track_.isPlaying = false;
}

void MusicPlayer::connect() {
    if (connected_) return;

    webSocket.begin(serverUrl_, serverPort_, "/ws/music");
    webSocket.onEvent(webSocketEventCB);
    webSocket.setReconnectInterval(5000);
    connected_ = true;
}

void MusicPlayer::disconnect() {
    webSocket.disconnect();
    connected_ = false;
}

void MusicPlayer::play() { sendCommand("play"); }
void MusicPlayer::pause() { sendCommand("pause"); }
void MusicPlayer::next() { sendCommand("next"); }
void MusicPlayer::prev() { sendCommand("prev"); }

void MusicPlayer::update() {
    if (connected_) {
        webSocket.loop();
    }
}

void MusicPlayer::onTrackChange(TrackChangeCallback cb) {
    callback_ = cb;
}

const TrackInfo& MusicPlayer::currentTrack() const {
    return track_;
}

void MusicPlayer::sendCommand(const char* action) {
    if (!connected_) return;

    JsonDocument doc;
    doc["type"] = "command";
    doc["action"] = action;

    char buffer[128];
    serializeJson(doc, buffer, sizeof(buffer));
    webSocket.sendTXT(buffer);
}

void MusicPlayer::handleTrackInfo(const char* json) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) return;

    TrackInfo prev = track_;

    const char* title = doc["title"] | "";
    const char* artist = doc["artist"] | "";
    strncpy(track_.title, title, sizeof(track_.title) - 1);
    track_.title[sizeof(track_.title) - 1] = '\0';
    strncpy(track_.artist, artist, sizeof(track_.artist) - 1);
    track_.artist[sizeof(track_.artist) - 1] = '\0';
    track_.isPlaying = doc["isPlaying"] | false;
    track_.duration = doc["duration"] | 0;
    track_.position = doc["position"] | 0;
    hasTrack_ = true;

    if (memcmp(&prev, &track_, sizeof(TrackInfo)) != 0 && callback_) {
        callback_(track_);
    }
}
