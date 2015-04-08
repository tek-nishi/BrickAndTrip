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

  std::unique_ptr<UIView> view_;
  
  bool active_;

  ConnectionHolder connections_;

  ci::TimelineRef event_timeline_;


public:
  PauseController(ci::JsonTree& params,
                  ci::TimelineRef timeline,
                  Event<EventParam>& event,
                  std::unique_ptr<UIView>&& view) :
    params_(params),
    event_(event),
    view_(std::move(view)),
    active_(true),
    event_timeline_(ci::Timeline::create())
  {
    DOUT << "PauseController()" << std::endl;
    
    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    timeline->apply(event_timeline_);

    connections_ += event.connect("pause-cancel",
                                  [this](const Connection& connection, EventParam& param) {
                                    // 時間差tween
                                    event_timeline_->add([this]() {
                                        view_->startWidgetTween("tween-out");
                                      },
                                      event_timeline_->getCurrentTime() + 0.5f);
                                    
                                    // 時間差でControllerを破棄
                                    event_timeline_->add([this]() {
                                        event_.signal("game-continue", EventParam());
                                        active_ = false;
                                      },
                                      event_timeline_->getCurrentTime() + 1.0f);
                                    
                                    connection.disconnect();
                                  });

    connections_ += event.connect("pause-abort",
                                  [this](const Connection& connection, EventParam& param) {
                                    // 時間差tween
                                    event_timeline_->add([this]() {
                                        view_->startWidgetTween("tween-out");
                                      },
                                      event_timeline_->getCurrentTime() + 0.5f);
                                    
                                    // 時間差でControllerを破棄
                                    event_timeline_->add([this]() {
                                        event_.signal("game-abort", EventParam());
                                        active_ = false;
                                      },
                                      event_timeline_->getCurrentTime() + 1.0f);
                                    
                                    connection.disconnect();
                                  });

    view_->startWidgetTween("tween-in");
  }

  ~PauseController() {
    DOUT << "~PauseController()" << std::endl;

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
  
  void draw(FontHolder& fonts, Model& cube, Model& text) override {
    view_->draw(fonts, cube, text);
  }

};

}
