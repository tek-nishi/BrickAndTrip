#pragma once

//
// Title画面
//

#include "ControllerBase.hpp"
#include "UIView.hpp"
#include "ConnectionHolder.hpp"


namespace ngs {

class TitleController : public ControllerBase {
  ci::JsonTree& params_;
  Event<EventParam>& event_;

  std::unique_ptr<UIView> view_;
  
  bool active_;

  ConnectionHolder connections_;

  ci::TimelineRef event_timeline_;
  

public:
  TitleController(ci::JsonTree& params,
                  ci::TimelineRef timeline,
                  Event<EventParam>& event,
                  std::unique_ptr<UIView>&& view) :
    params_(params),
    event_(event),
    view_(std::move(view)),
    active_(true),
    event_timeline_(ci::Timeline::create())
  {
    DOUT << "TitleController()" << std::endl;

    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    timeline->apply(event_timeline_);
    
    view_->startWidgetTween("tween-in");

    connections_ += event.connect("pickable-moved",
                                  [this](const Connection& connection, EventParam& param) {
                                    view_->startWidgetTween("tween-out");

                                    // 時間差でControllerを破棄
                                    event_timeline_->add([this]() {
                                        active_ = false;
                                      },
                                      event_timeline_->getCurrentTime() + 1.5f);
                                    
                                    connection.disconnect();
                                  });

    event_.signal("sound-title-start", EventParam());
  }

  ~TitleController() {
    DOUT << "~TitleController()" << std::endl;

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
  
  void draw(FontHolder& fonts) override {
    view_->draw(fonts);
  }

};

}
