#pragma once

//
// Credits
//

#include "ControllerBase.hpp"
#include "UIView.hpp"
#include "ConnectionHolder.hpp"
#include "GameCenter.h"


namespace ngs {

class CreditsController : public ControllerBase {
  ci::JsonTree& params_;
  Event<EventParam>& event_;

  float tween_delay_;
  float event_delay_;
  float deactive_delay_;

  std::unique_ptr<UIView> view_;
  
  bool active_;

  ConnectionHolder connections_;

  ci::TimelineRef event_timeline_;


public:
  CreditsController(ci::JsonTree& params,
                    ci::TimelineRef timeline,
                    Event<EventParam>& event,
                    std::unique_ptr<UIView>&& view) :
    params_(params),
    event_(event),
    tween_delay_(params["credits.tween_delay"].getValue<float>()),
    event_delay_(params["credits.event_delay"].getValue<float>()),
    deactive_delay_(params["credits.deactive_delay"].getValue<float>()),
    view_(std::move(view)),
    active_(true),
    event_timeline_(ci::Timeline::create())
  {
    DOUT << "CreditsController()" << std::endl;
    
    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    timeline->apply(event_timeline_);

    connections_ += event.connect("credits-agree",
                                  [this](const Connection&, EventParam& param) {
                                    view_->setActive(false);
                                    
                                    event_timeline_->add([this]() {
                                        view_->startWidgetTween("tween-out");

                                        event_timeline_->add([this]() {
                                            EventParam params = {
                                              { "menu-to-title", true },
                                            };
                                            event_.signal("begin-title", params);
                                            
                                            event_timeline_->add([this]() {
                                                active_ = false;
                                              },
                                              event_timeline_->getCurrentTime() + deactive_delay_);
                                          },
                                          event_timeline_->getCurrentTime() + event_delay_);
                                      },
                                      event_timeline_->getCurrentTime() + tween_delay_);
                                  });

    view_->startWidgetTween("tween-in");
    requestSound(event_, params["credits.jingle-se"].getValue<std::string>());

    if (params.hasChild("credits.active_delay")) {
      view_->setActive(false);

      float delay = params["credits.active_delay"].getValue<float>();
      event_timeline_->add([this]() {
          view_->setActive(true);
        },
        event_timeline_->getCurrentTime() + delay);
    }
    
    GameCenter::submitAchievement("BRICKTRIP.ACHIEVEMENT.VIEWED_CREDITS");
  }

  ~CreditsController() {
    DOUT << "~CreditsController()" << std::endl;

    // 再生途中のものもあるので、手動で取り除く
    event_timeline_->removeSelf();
  }


private:
  bool isActive() const noexcept override {
    return active_;
  }

  Event<EventParam>& event() noexcept override { return event_; }

  void resize() noexcept override {
  }
  
  void update(const double progressing_seconds) noexcept override {
  }
  
  void draw(FontHolder& fonts, ModelHolder& models) noexcept override {
    view_->draw(fonts, models);
  }

};

}
