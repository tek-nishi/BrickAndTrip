#pragma once

//
// 本編進行UI
//

#include "ControllerBase.hpp"
#include "UIView.hpp"
#include "ConnectionHolder.hpp"


namespace ngs {

class ProgressController : public ControllerBase {
  ci::JsonTree& params_;
  Event<EventParam>& event_;

  std::unique_ptr<UIView> view_;
  
  bool active_;

  ConnectionHolder connections_;

  ci::TimelineRef event_timeline_;


public:
  ProgressController(ci::JsonTree& params,
                     ci::TimelineRef timeline,
                     Event<EventParam>& event,
                     std::unique_ptr<UIView>&& view) :
    params_(params),
    event_(event),
    view_(std::move(view)),
    active_(true),
    event_timeline_(ci::Timeline::create())
  {
    DOUT << "ProgressController()" << std::endl;
    
    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    timeline->apply(event_timeline_);

    connections_ += event.connect("pause-start",
                                  [this](const Connection& connection, EventParam& param) {
                                    // 時間差tween
                                    event_timeline_->add([this]() {
                                        view_->startWidgetTween("tween-out");
                                      },
                                      event_timeline_->getCurrentTime() + 0.5f);
                                    
                                    // 時間差でPause画面表示
                                    event_timeline_->add([this]() {
                                        // Viewは非表示に
                                        view_->setDisp(false);
                                        view_->setActive(false);

                                        event_.signal("begin-pause", EventParam());
                                      },
                                      event_timeline_->getCurrentTime() + 1.0f);
                                  });

    connections_ += event.connect("game-continue",
                                  [this](const Connection& connection, EventParam& param) {
                                    view_->setDisp(true);
                                    view_->setActive(true);
                                  });

    connections_ += event.connect("game-abort",
                                  [this](const Connection& connection, EventParam& param) {
                                    active_ = false;
                                  });

    connections_ += event.connect("begin-stageclear",
                                  [this](const Connection& connection, EventParam& param) {
                                    bool all_cleard = boost::any_cast<bool>(param.at("all_cleared"));
                                    if (all_cleard) {
                                      active_ = false;
                                    }
                                    else {
                                      view_->setDisp(false);
                                      view_->setActive(false);
                                    }
                                  });

    connections_ += event.connect("stageclear-agree",
                                  [this](const Connection& connection, EventParam& param) {
                                    view_->setDisp(true);
                                    view_->setActive(true);
                                  });

    connections_ += event_.connect("first-fallen-pickable",
                                  [this](const Connection& connection, EventParam& param) {
                                     view_->setActive(false);
                                     view_->startWidgetTween("tween-out");
                                     event_timeline_->add([this]() {
                                         active_ = false;
                                       },
                                       event_timeline_->getCurrentTime() + 1.0f);
                                   });


    connections_ += event_.connect("update-record",
                                   [this](const Connection& connection, EventParam& param) {
                                     auto play_time = boost::any_cast<double>(param["play-time"]);
                                     auto& widget = view_->getWidget("play-time");
                                     widget.getCubeText().setText(toFormatedString(play_time));
                                   });

    view_->startWidgetTween("tween-in");
  }

  ~ProgressController() {
    DOUT << "~ProgressController()" << std::endl;

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
