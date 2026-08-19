#include "MusicUI.h"
#include <cstdio>

void MusicUI::begin(lv_obj_t* parent, MusicPlayer* player) {
    player_ = player;
    createUI(parent);

    player_->onTrackChange([this](const TrackInfo& track) {
        onTrackChange(track);
    });
}

void MusicUI::update() {
    // Nothing to poll — updates come via onTrackChange callback
}

void MusicUI::show() {
    if (container_) lv_obj_clear_flag(container_, LV_OBJ_FLAG_HIDDEN);
}

void MusicUI::hide() {
    if (container_) lv_obj_add_flag(container_, LV_OBJ_FLAG_HIDDEN);
}

void MusicUI::createUI(lv_obj_t* parent) {
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, 240, 240);
    lv_obj_set_style_bg_color(container_, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 0, 0);
    lv_obj_center(container_);
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Title
    titleLabel_ = lv_label_create(container_);
    lv_label_set_text(titleLabel_, "No track");
    lv_obj_set_style_text_color(titleLabel_, lv_color_white(), 0);
    lv_obj_set_style_text_font(titleLabel_, &lv_font_montserrat_14, 0);
    lv_label_set_long_mode(titleLabel_, LV_LABEL_LONG_DOT);
    lv_obj_set_width(titleLabel_, 200);
    lv_obj_set_style_text_align(titleLabel_, LV_TEXT_ALIGN_CENTER, 0);

    // Artist
    artistLabel_ = lv_label_create(container_);
    lv_label_set_text(artistLabel_, "");
    lv_obj_set_style_text_color(artistLabel_, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(artistLabel_, &lv_font_montserrat_12, 0);
    lv_label_set_long_mode(artistLabel_, LV_LABEL_LONG_DOT);
    lv_obj_set_width(artistLabel_, 200);
    lv_obj_set_style_text_align(artistLabel_, LV_TEXT_ALIGN_CENTER, 0);

    // Controls row
    lv_obj_t* controlsRow = lv_obj_create(container_);
    lv_obj_set_size(controlsRow, 220, 50);
    lv_obj_set_style_bg_opa(controlsRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(controlsRow, 0, 0);
    lv_obj_set_style_pad_all(controlsRow, 0, 0);
    lv_obj_set_flex_flow(controlsRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(controlsRow, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Prev button
    prevBtn_ = lv_btn_create(controlsRow);
    lv_obj_set_size(prevBtn_, 48, 48);
    lv_obj_set_style_bg_color(prevBtn_, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(prevBtn_, 24, 0);
    lv_obj_add_event_cb(prevBtn_, prevBtnCb, LV_EVENT_CLICKED, this);
    lv_obj_t* prevLabel = lv_label_create(prevBtn_);
    lv_label_set_text(prevLabel, LV_SYMBOL_PREV);
    lv_obj_set_style_text_color(prevLabel, lv_color_white(), 0);
    lv_obj_center(prevLabel);

    // Play/Pause button
    playPauseBtn_ = lv_btn_create(controlsRow);
    lv_obj_set_size(playPauseBtn_, 56, 56);
    lv_obj_set_style_bg_color(playPauseBtn_, lv_color_hex(0x1DB954), 0);
    lv_obj_set_style_radius(playPauseBtn_, 28, 0);
    lv_obj_add_event_cb(playPauseBtn_, playPauseBtnCb, LV_EVENT_CLICKED, this);
    playPauseLabel_ = lv_label_create(playPauseBtn_);
    lv_label_set_text(playPauseLabel_, LV_SYMBOL_PLAY);
    lv_obj_set_style_text_color(playPauseLabel_, lv_color_white(), 0);
    lv_obj_center(playPauseLabel_);

    // Next button
    nextBtn_ = lv_btn_create(controlsRow);
    lv_obj_set_size(nextBtn_, 48, 48);
    lv_obj_set_style_bg_color(nextBtn_, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(nextBtn_, 24, 0);
    lv_obj_add_event_cb(nextBtn_, nextBtnCb, LV_EVENT_CLICKED, this);
    lv_obj_t* nextLabel = lv_label_create(nextBtn_);
    lv_label_set_text(nextLabel, LV_SYMBOL_NEXT);
    lv_obj_set_style_text_color(nextLabel, lv_color_white(), 0);
    lv_obj_center(nextLabel);
}

void MusicUI::onTrackChange(const TrackInfo& track) {
    lv_label_set_text(titleLabel_, track.title[0] ? track.title : "No track");
    lv_label_set_text(artistLabel_, track.artist);
    updatePlayPauseIcon(track.isPlaying);
}

void MusicUI::updatePlayPauseIcon(bool isPlaying) {
    lv_label_set_text(playPauseLabel_, isPlaying ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    lv_obj_center(playPauseLabel_);
}

void MusicUI::prevBtnCb(lv_event_t* e) {
    auto* ui = static_cast<MusicUI*>(lv_event_get_user_data(e));
    if (ui && ui->player_) ui->player_->prev();
}

void MusicUI::playPauseBtnCb(lv_event_t* e) {
    auto* ui = static_cast<MusicUI*>(lv_event_get_user_data(e));
    if (!ui || !ui->player_) return;

    if (ui->player_->currentTrack().isPlaying) {
        ui->player_->pause();
    } else {
        ui->player_->play();
    }
}

void MusicUI::nextBtnCb(lv_event_t* e) {
    auto* ui = static_cast<MusicUI*>(lv_event_get_user_data(e));
    if (ui && ui->player_) ui->player_->next();
}
