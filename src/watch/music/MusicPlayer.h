#pragma once

#include <Arduino.h>
#include <functional>

struct TrackInfo {
    char title[128];
    char artist[128];
    bool isPlaying;
    uint32_t duration;   // seconds
    uint32_t position;   // seconds
};

class MusicPlayer {
public:
    using TrackChangeCallback = std::function<void(const TrackInfo&)>;

    void begin(const char* serverUrl, uint16_t port = 8000);
    void connect();
    void disconnect();

    void play();
    void pause();
    void next();
    void prev();

    void update();

    void onTrackChange(TrackChangeCallback cb);
    const TrackInfo& currentTrack() const;
    void handleTrackInfo(const char* json);

private:
    void sendCommand(const char* action);

    TrackInfo track_{};
    TrackChangeCallback callback_;
    bool connected_ = false;
    bool hasTrack_ = false;

    char serverUrl_[128];
    uint16_t serverPort_ = 8000;
};
