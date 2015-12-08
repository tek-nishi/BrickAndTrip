#pragma once

//
// PAUSE UI
//

#include "ControllerBase.hpp"
#include "UIView.hpp"
#include "ConnectionHolder.hpp"


namespace ngs {

class PauseController : public ControllerBase {
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
  PauseController(ci::JsonTree& params,
                  ci::TimelineRef timeline,
                  Event<EventParam>& event,
                  std::unique_ptr<UIView>&& view) noexcept :
    params_(params),
    event_(event),
    tween_delay_(params["pause.tween_delay"].getValue<float>()),
    event_delay_(params["pause.event_delay"].getValue<float>()),
    deactive_delay_(params["pause.deactive_delay"].getValue<float>()),
    view_(std::move(view)),
    active_(true),
    event_timeline_(ci::Timeline::create())
  {
    DOUT << "PauseController()" << std::endl;
    
    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    timeline->apply(event_timeline_);

    connections_ += event.connect("pause-cancel",
                                  [this](const Connection&, EventParam& param) noexcept {
                                    view_->setActive(false);
                                    
                                    event_timeline_->add([this]() noexcept {
                                        view_->startWidgetTween("tween-out");

                                        event_timeline_->add([this]() noexcept {
                                            event_.signal("game-continue", EventParam());
                                            
                                            event_timeline_->add([this]() noexcept {
                                                active_ = false;
                                              },
                                              event_timeline_->getCurrentTime() + deactive_delay_);
                                          },
                                          event_timeline_->getCurrentTime() + event_delay_);
                                      },
                                      event_timeline_->getCurrentTime() + tween_delay_);
                                  });

    connections_ += event.connect("pause-abort",
                                  [this](const Connection&, EventParam& param) noexcept {
                                    view_->setActive(false);
                                    
                                    event_timeline_->add([this]() noexcept {
                                        view_->startWidgetTween("tween-out");

                                        event_timeline_->add([this]() noexcept {
                                            event_.signal("game-abort", EventParam());

                                            event_timeline_->add([this]() noexcept {
                                                active_ = false;
                                              },
                                              event_timeline_->getCurrentTime() + deactive_delay_);
                                          },
                                          event_timeline_->getCurrentTime() + event_delay_);
                                      },
                                      event_timeline_->getCurrentTime() + tween_delay_);
                                  });

    view_->startWidgetTween("tween-in");

    if (params.hasChild("pause.active_delay")) {
      view_->setActive(false);

      float delay = params["pause.active_delay"].getValue<float>();
      event_timeline_->add([this]() noexcept {
          view_->setActive(true);
        },
        event_timeline_->getCurrentTime() + delay);
    }
  }

  ~PauseController() noexcept {
    DOUT << "~PauseController()" << std::endl;

    // 再生途中のものもあるので、手動で取り除く
    event_timeline_->removeSelf();
  }


private:
  bool isActive() const noexcept override {
    return active_;
  }

  Event<EventParam>& event() noexcept override { return event_; }

  void resize() noexcept override { }
  
  void update(const double progressing_seconds) noexcept override { }
  
  void draw(FontHolder& fonts, ModelHolder& models) noexcept override {
    view_->draw(fonts, models);
  }

};

}
