#pragma once

//
// Game Over
//

#include "ControllerBase.hpp"
#include "UIView.hpp"
#include "ConnectionHolder.hpp"


namespace ngs {

class GameoverController : public ControllerBase {
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
  GameoverController(ci::JsonTree& params,
                     ci::TimelineRef timeline,
                     Event<EventParam>& event,
                     std::unique_ptr<UIView>&& view) :
    params_(params),
    event_(event),
    tween_delay_(params["gameover.tween_delay"].getValue<float>()),
    event_delay_(params["gameover.event_delay"].getValue<float>()),
    deactive_delay_(params["gameover.deactive_delay"].getValue<float>()),
    view_(std::move(view)),
    active_(true),
    event_timeline_(ci::Timeline::create())
  {
    DOUT << "GameoverController()" << std::endl;
    
    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    timeline->apply(event_timeline_);

    connections_ += event.connect("gameover-agree",
                                  [this](const Connection& connection, EventParam& param) {
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

    view_->startWidgetTween("tween-in");

    if (params.hasChild("gameover.active_delay")) {
      view_->setActive(false);

      float delay = params["gameover.active_delay"].getValue<float>();
      event_timeline_->add([this]() {
          view_->setActive(true);
        },
        event_timeline_->getCurrentTime() + delay);
    }
  }

  ~GameoverController() {
    DOUT << "~GameoverController()" << std::endl;

    // 再生途中のものもあるので、手動で取り除く
    event_timeline_->removeSelf();
  }


private:
  bool isActive() const override {
    return active_;
  }

  Event<EventParam>& event() override { return event_; }

  void resize() override {
  }
  
  void update(const double progressing_seconds) override {
  }
  
  void draw(FontHolder& fonts, ModelHolder& models) override {
    view_->draw(fonts, models);
  }

};

}
