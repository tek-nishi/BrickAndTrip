#pragma once

//
// Credits
//

#include "ControllerBase.hpp"
#include "UIView.hpp"
#include "ConnectionHolder.hpp"


namespace ngs {

class CreditsController : public ControllerBase {
  ci::JsonTree& params_;
  Event<EventParam>& event_;

  float event_delay_;

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
    event_delay_(params["credits.event_delay"].getValue<float>()),
    view_(std::move(view)),
    active_(true),
    event_timeline_(ci::Timeline::create())
  {
    DOUT << "CreditsController()" << std::endl;
    
    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    timeline->apply(event_timeline_);

    connections_ += event.connect("credits-agree",
                                  [this](const Connection& connection, EventParam& param) {
                                    view_->setActive(false);
                                    
                                    // 時間差tween
                                    event_timeline_->add([this]() {
                                        view_->startWidgetTween("tween-out");

                                        // 時間差でControllerを破棄
                                        event_timeline_->add([this]() {
                                            event_.signal("begin-title", EventParam());
                                            active_ = false;
                                          },
                                          event_timeline_->getCurrentTime() + 1.5f);
                                      },
                                      event_timeline_->getCurrentTime() + event_delay_);
                                  });

    view_->startWidgetTween("tween-in");
  }

  ~CreditsController() {
    DOUT << "~CreditsController()" << std::endl;

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
