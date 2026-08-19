#pragma once

#include "lvgl.h"
#include "MusicPlayer.h"

class MusicUI {
public:
    void begin(lv_obj_t* parent, MusicPlayer* player);
    void update();

    void show();
    void hide();

private:
    void createUI(lv_obj_t* parent);
    void onTrackChange(const TrackInfo& track);
    void updatePlayPauseIcon(bool isPlaying);

    static void prevBtnCb(lv_event_t* e);
    static void playPauseBtnCb(lv_event_t* e);
    static void nextBtnCb(lv_event_t* e);

    MusicPlayer* player_ = nullptr;
    lv_obj_t* container_ = nullptr;
    lv_obj_t* titleLabel_ = nullptr;
    lv_obj_t* artistLabel_ = nullptr;
    lv_obj_t* prevBtn_ = nullptr;
    lv_obj_t* playPauseBtn_ = nullptr;
    lv_obj_t* playPauseLabel_ = nullptr;
    lv_obj_t* nextBtn_ = nullptr;
};
