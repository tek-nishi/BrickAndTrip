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
  

public:
  TitleController(ci::JsonTree& params,
                  Event<EventParam>& event,
                  std::unique_ptr<UIView>&& view) :
    params_(params),
    event_(event),
    view_(std::move(view)),
    active_(true)
  {
    view_->startWidgetTween("tween-in");

    connections_ += event.connect("pickable-moved",
                                  [this](const Connection& connection, EventParam& param) {
                                    view_->startWidgetTween("tween-out");
                                    connection.disconnect();
                                  });
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
