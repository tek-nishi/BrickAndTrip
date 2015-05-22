#pragma once

//
// all-stage clear
//

#include "ControllerBase.hpp"
#include "UIView.hpp"
#include "ConnectionHolder.hpp"


namespace ngs {

class AllStageClearController : public ControllerBase {
  Event<EventParam>& event_;

  std::string message_;
  
  float event_delay_;
  float tween_in_delay_;
  float tween_out_delay_;
  float collapse_delay_;
  float deactive_delay_;
  
  std::unique_ptr<UIView> view_;
  
  bool active_;

  ConnectionHolder connections_;

  ci::TimelineRef event_timeline_;


public:
  AllStageClearController(ci::JsonTree& params,
                          ci::TimelineRef timeline,
                          Event<EventParam>& event,
                          std::unique_ptr<UIView>&& view) :
    event_(event),
    message_(params["message"].getValue<std::string>()),
    event_delay_(params["event_delay"].getValue<float>()),
    tween_in_delay_(params["tween_in_delay"].getValue<float>()),
    tween_out_delay_(params["tween_out_delay"].getValue<float>()),
    collapse_delay_(params["collapse_delay"].getValue<float>()),
    deactive_delay_(params["deactive_delay"].getValue<float>()),
    view_(std::move(view)),
    active_(true),
    event_timeline_(ci::Timeline::create())
  {
    DOUT << "AllStageClearController()" << std::endl;
    
    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    timeline->apply(event_timeline_);

    event_timeline_->add([this]() {
        event_.signal(message_, EventParam());

        event_timeline_->add([this]() {
            view_->setActive(true);
            view_->startWidgetTween("tween-in");

            event_timeline_->add([this]() {
                view_->startWidgetTween("tween-out");

                event_timeline_->add([this]() {
                    event_.signal("back-to-title", EventParam());

                    event_timeline_->add([this]() {
                        active_ = false;
                      },
                      event_timeline_->getCurrentTime() + deactive_delay_);
                  },
                  event_timeline_->getCurrentTime() + collapse_delay_);
              },
              event_timeline_->getCurrentTime() + tween_out_delay_);
          },
          event_timeline_->getCurrentTime() + tween_in_delay_);
      },
      event_timeline_->getCurrentTime() + event_delay_);
    
    view_->setActive(false);
  }

  ~AllStageClearController() {
    DOUT << "~AllStageClearController()" << std::endl;

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
