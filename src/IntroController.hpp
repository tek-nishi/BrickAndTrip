#pragma once

//
// Intro画面
//

#include "ControllerBase.hpp"
#include "UIView.hpp"
#include "ConnectionHolder.hpp"


namespace ngs {

class IntroController : public ControllerBase {
  ci::JsonTree& params_;
  Event<EventParam>& event_;

  std::unique_ptr<UIView> view_;
  
  bool active_;

  ConnectionHolder connections_;

  ci::TimelineRef event_timeline_;
  

public:
  IntroController(ci::JsonTree& params,
                  ci::TimelineRef timeline,
                  Event<EventParam>& event,
                  std::unique_ptr<UIView>&& view) :
    params_(params),
    event_(event),
    view_(std::move(view)),
    active_(true),
    event_timeline_(ci::Timeline::create())
  {
    DOUT << "IntroController()" << std::endl;

    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    timeline->apply(event_timeline_);

    event_timeline_->add([this]() {
        view_->startWidgetTween("tween-out");

        event_timeline_->add([this]() {
            event_.signal("begin-title", EventParam());

            event_timeline_->add([this]() {
                active_ = false;
              },
              event_timeline_->getCurrentTime() + params_["intro.deactive_delay"].getValue<float>());
          },
          event_timeline_->getCurrentTime() + params_["intro.event_delay"].getValue<float>());
      },
      event_timeline_->getCurrentTime() + params_["intro.tween_out_delay"].getValue<float>());
    

    view_->startWidgetTween("tween-in");
  }

  ~IntroController() {
    DOUT << "~IntroController()" << std::endl;

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
  
};

}
