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

  float tween_delay_;
  float event_delay_;
  float deactive_delay_;
  float deactivate_view_delay_;

  std::unique_ptr<UIView> view_;
  
  bool active_;

  ConnectionHolder connections_;

  ci::TimelineRef event_timeline_;


public:
  ProgressController(ci::JsonTree& params,
                     ci::TimelineRef timeline,
                     Event<EventParam>& event,
                     std::unique_ptr<UIView>&& view) noexcept :
    params_(params),
    event_(event),
    tween_delay_(params["progress.tween_delay"].getValue<float>()),
    event_delay_(params["progress.event_delay"].getValue<float>()),
    deactive_delay_(params["progress.deactive_delay"].getValue<float>()),
    deactivate_view_delay_(params["progress.deactivate_view_delay"].getValue<float>()),
    view_(std::move(view)),
    active_(true),
    event_timeline_(ci::Timeline::create())
  {
    DOUT << "ProgressController()" << std::endl;
    
    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    timeline->apply(event_timeline_);

    connections_ += event.connect("pause-agree",
                                  [this](const Connection&, EventParam& param) noexcept {
                                    // 自動PAUSEのための措置
                                    if (!view_->isActive()) return;

                                    view_->setActive(false);
                                    event_.signal("pause-start", EventParam());

                                    if (hasKey(param, "force") && boost::any_cast<bool>(param.at("force"))) {
                                      // 強制PAUSEはタイミングが違う
                                      event_.signal("begin-pause", EventParam());
                                      view_->startWidgetTween("tween-out");
                                      event_timeline_->add([this]() noexcept {
                                          view_->setDisp(false);
                                        },
                                        event_timeline_->getCurrentTime() + deactive_delay_);
                                    }
                                    else {
                                      // 時間差tween
                                      event_timeline_->add([this]() noexcept {
                                          view_->startWidgetTween("tween-out");

                                          // 時間差でsignal
                                          event_timeline_->add([this]() noexcept {
                                              event_.signal("begin-pause", EventParam());

                                              event_timeline_->add([this]() noexcept {
                                                  // Viewは非表示に
                                                  view_->setDisp(false);
                                                },
                                                event_timeline_->getCurrentTime() + deactive_delay_);
                                            },
                                            event_timeline_->getCurrentTime() + event_delay_);
                                        },
                                        event_timeline_->getCurrentTime() + tween_delay_);
                                    }
                                  });

    connections_ += event.connect("game-continue",
                                  [this](const Connection&, EventParam& param) noexcept {
                                    view_->setDisp(true);
                                    view_->setActive(true);
                                    
                                    view_->resetWidgetTween();
                                    view_->startWidgetTween("tween-in");
                                  });

    connections_ += event.connect("game-abort",
                                  [this](const Connection&, EventParam& param) noexcept {
                                    active_ = false;
                                  });

    connections_ += event.connect("begin-stageclear",
                                  [this](const Connection&, EventParam& param) noexcept {
                                    view_->setActive(false);
                                    bool all_cleard = boost::any_cast<bool>(param.at("all_cleared"));
                                    if (all_cleard) {
                                      deactivateView();
                                    }
                                    else {
                                      view_->startWidgetTween("tween-out");
                                      event_timeline_->add([this]() noexcept {
                                          view_->setDisp(false);
                                        },
                                        event_timeline_->getCurrentTime() + deactivate_view_delay_);
                                    }
                                  });

    connections_ += event.connect("stageclear-agree",
                                  [this](const Connection&, EventParam& param) noexcept {
                                    view_->setDisp(true);
                                    view_->setActive(true);
                                    view_->resetWidgetTween();
                                    view_->startWidgetTween("tween-in");
                                  });

    connections_ += event_.connect("first-out-pickable",
                                  [this](const Connection&, EventParam& param) noexcept {
                                     deactivateView();
                                   });

    connections_ += event_.connect("update-record",
                                   [this](const Connection&, EventParam& param) noexcept {
                                     auto play_time = boost::any_cast<double>(param["play-time"]);
                                     auto& widget = view_->getWidget("play-time");
                                     widget.setText(toFormatedString(play_time));
                                   });

    view_->startWidgetTween("tween-in");

    if (params.hasChild("progress.active_delay")) {
      view_->setActive(false);

      float delay = params["progress.active_delay"].getValue<float>();
      event_timeline_->add([this]() noexcept {
          view_->setActive(true);
        },
        event_timeline_->getCurrentTime() + delay);
    }
  }

  ~ProgressController() noexcept {
    DOUT << "~ProgressController()" << std::endl;

    // 再生途中のものもあるので、手動で取り除く
    event_timeline_->removeSelf();
  }


private:
  void deactivateView() noexcept {
    view_->setActive(false);
    view_->startWidgetTween("tween-out");
    event_timeline_->add([this]() noexcept {
        active_ = false;
      },
      event_timeline_->getCurrentTime() + deactivate_view_delay_);
  }

  
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
