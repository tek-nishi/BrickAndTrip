﻿#pragma once

//
// Settings画面
//

#include "ControllerBase.hpp"
#include "UIView.hpp"
#include "ConnectionHolder.hpp"
#include "GameCenter.h"


namespace ngs {

class SettingsController : public ControllerBase {
  ci::JsonTree& params_;
  Event<EventParam>& event_;
  Records& records_;

  float tween_delay_;
  float event_delay_;
  float deactive_delay_;

  std::unique_ptr<UIView> view_;
  
  bool active_;

  ConnectionHolder connections_;

  ci::TimelineRef event_timeline_;

  
public:
  SettingsController(ci::JsonTree& params,
                    ci::TimelineRef timeline,
                    Event<EventParam>& event,
                    Records& records,
                     std::unique_ptr<UIView>&& view) noexcept :
    params_(params),
    event_(event),
    tween_delay_(params["settings.tween_delay"].getValue<float>()),
    event_delay_(params["settings.event_delay"].getValue<float>()),
    deactive_delay_(params["settings.deactive_delay"].getValue<float>()),
    records_(records),
    view_(std::move(view)),
    active_(true),
    event_timeline_(ci::Timeline::create())
  {
    DOUT << "SettingsController()" << std::endl;

    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    timeline->apply(event_timeline_);

    connections_ += event.connect("se-change",
                                  [this](const Connection&, EventParam& param) noexcept {
                                    bool active = records_.toggleSeOn();
                                    setSoundIcon("se-setting", active);
                                    
                                    EventParam p = {
                                      { "silent", !active },
                                    };
                                    event_.signal("se-silent", p);
                                    records_.write(params_["game.records"].getValue<std::string>());
                                  });

    connections_ += event.connect("bgm-change",
                                  [this](const Connection&, EventParam& param) noexcept {
                                    bool active = records_.toggleBgmOn();
                                    setSoundIcon("bgm-setting", active);
                                    
                                    EventParam p = {
                                      { "silent", !active },
                                    };
                                    event_.signal("bgm-silent", p);
                                    records_.write(params_["game.records"].getValue<std::string>());
                                  });

    connections_ += event.connect("settings-agree",
                                  [this](const Connection&, EventParam& param) noexcept {
                                    view_->setActive(false);

                                    event_timeline_->add([this]() noexcept {
                                        view_->startWidgetTween("tween-out");

                                        event_timeline_->add([this]() noexcept {
                                            EventParam params = {
                                              { "menu-to-title", true },
                                            };
                                            event_.signal("begin-title", params);
                                            
                                            event_timeline_->add([this]() noexcept {
                                                active_ = false;
                                              },
                                              event_timeline_->getCurrentTime() + deactive_delay_);
                                          },
                                          event_timeline_->getCurrentTime() + event_delay_);
                                      },
                                      event_timeline_->getCurrentTime() + tween_delay_);
                                  });
    
    setSoundIcon("se-setting", records_.isSeOn());
    setSoundIcon("bgm-setting", records_.isBgmOn());

    view_->startWidgetTween("tween-in");
    requestSound(event_, params["settings.jingle-se"].getValue<std::string>());

    if (params.hasChild("settings.active_delay")) {
      view_->setActive(false);

      float delay = params["settings.active_delay"].getValue<float>();
      event_timeline_->add([this]() noexcept {
          view_->setActive(true);
        },
        event_timeline_->getCurrentTime() + delay);
    }
    
    GameCenter::submitAchievement("BRICKTRIP.ACHIEVEMENT.VIEWED_SETTINGS");
  }

  ~SettingsController() {
    DOUT << "~SettingsController()" << std::endl;

    // 再生途中のものもあるので、手動で取り除く
    event_timeline_->removeSelf();
  }


private:
  bool isActive() const noexcept override { return active_; }

  Event<EventParam>& event() noexcept override { return event_; }

  void resize() noexcept override { }
  
  void update(const double progressing_seconds) noexcept override { }
  
  void draw(FontHolder& fonts, ModelHolder& models) noexcept override {
    view_->draw(fonts, models);
  }


  void setSoundIcon(const std::string& widget, const bool is_sound) noexcept {
    view_->getWidget(widget).setText(is_sound ? "z"
                                              : "x");
  }
  
};

}
