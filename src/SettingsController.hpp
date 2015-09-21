#pragma once

//
// Settings画面
//

#include "ControllerBase.hpp"
#include "UIView.hpp"
#include "ConnectionHolder.hpp"


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
                     std::unique_ptr<UIView>&& view) :
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
                                  [this](const Connection& connection, EventParam& param) {
                                    bool active = records_.toggleSeOn();
                                    setSoundIcon("se-setting", active);
                                    
                                    EventParam p = {
                                      { "silent", !active },
                                    };
                                    event_.signal("se-silent", p);
                                  });

    connections_ += event.connect("bgm-change",
                                  [this](const Connection& connection, EventParam& param) {
                                    bool active = records_.toggleBgmOn();
                                    setSoundIcon("bgm-setting", active);
                                    
                                    EventParam p = {
                                      { "silent", !active },
                                    };
                                    event_.signal("bgm-silent", p);
                                  });

    connections_ += event.connect("settings-agree",
                                  [this](const Connection& connection, EventParam& param) {
                                    view_->setActive(false);
                                    records_.write(params_["game.records"].getValue<std::string>());

                                    event_timeline_->add([this]() {
                                        view_->startWidgetTween("tween-out");

                                        event_timeline_->add([this]() {
                                            event_.signal("begin-title", EventParam());
                                            
                                            event_timeline_->add([this]() {
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
      event_timeline_->add([this]() {
          view_->setActive(true);
        },
        event_timeline_->getCurrentTime() + delay);
    }
  }

  ~SettingsController() {
    DOUT << "~SettingsController()" << std::endl;

    // 再生途中のものもあるので、手動で取り除く
    event_timeline_->removeSelf();
  }


private:
  bool isActive() const override { return active_; }

  Event<EventParam>& event() override { return event_; }

  void resize() override {
  }
  
  void update(const double progressing_seconds) override {
  }
  
  void draw(FontHolder& fonts, ModelHolder& models) override {
    view_->draw(fonts, models);
  }


  void setSoundIcon(const std::string& widget, const bool is_sound) {
    view_->getWidget(widget).setText(is_sound ? "z"
                                              : "x");
  }
  
};

}
