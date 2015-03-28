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

  std::unique_ptr<UIView> view_;
  
  bool active_;

  ConnectionHolder connections_;

  ci::TimelineRef event_timeline_;


public:
  GameoverController(ci::JsonTree& params,
                     Event<EventParam>& event,
                     std::unique_ptr<UIView>&& view) :
    params_(params),
    event_(event),
    view_(std::move(view)),
    active_(true),
    event_timeline_(ci::Timeline::create())
  {
    DOUT << "GameoverController()" << std::endl;
    
    view_->startWidgetTween("tween-in");
    
    auto current_time = ci::app::timeline().getCurrentTime();
    event_timeline_->setStartTime(current_time);
    ci::app::timeline().apply(event_timeline_);
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
  
  void draw(FontHolder& fonts) override {
    view_->draw(fonts);
  }

};

}
