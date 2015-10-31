#pragma once

//
// Title画面
//

#include "ControllerBase.hpp"
#include "UIView.hpp"
#include "ConnectionHolder.hpp"
#include "GameCenter.h"
#include "AppSupport.hpp"


namespace ngs {

class TitleController : public ControllerBase {
  ci::JsonTree& params_;
  Event<EventParam>& event_;
  Records& records_;

  float tween_delay_;
  float event_delay_;
  float deactive_delay_;
  
  std::unique_ptr<UIView> view_;
  
  bool active_;

  ConnectionHolder connections_;

  ci::TimelineRef event_timeline_;
  

public:
  TitleController(ci::JsonTree& params,
                  ci::TimelineRef timeline,
                  Event<EventParam>& event,
                  const EventParam& exec_params,
                  Records& records,
                  std::unique_ptr<UIView>&& view) :
    params_(params),
    event_(event),
    records_(records),
    tween_delay_(params["title.tween_delay"].getValue<float>()),
    event_delay_(params["title.event_delay"].getValue<float>()),
    deactive_delay_(params["title.deactive_delay"].getValue<float>()),
    view_(std::move(view)),
    active_(true),
    event_timeline_(ci::Timeline::create())
  {
    DOUT << "TitleController()" << std::endl;

    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    timeline->apply(event_timeline_);
    
    connections_ += event.connect("pickable-moved",
                                  [this](const Connection& connection, EventParam& param) {
                                    view_->setActive(false);
                                    view_->startWidgetTween("tween-out");

                                    // 時間差でControllerを破棄
                                    // メニュー遷移の時間調整方法とあわせるために
                                    // event_delay_ + deactive_delay_ としている
                                    event_timeline_->add([this]() {
                                        active_ = false;
                                      },
                                      event_timeline_->getCurrentTime() + event_delay_ + deactive_delay_);
                                    
                                    connection.disconnect();
                                  });

    connections_ += event.connect("records-start",
                                  [this](const Connection&, EventParam& param) {
                                    view_->setActive(false);
                                    event_.signal("field-input-stop", EventParam());

                                    event_timeline_->add([this]() {
                                        view_->startWidgetTween("tween-out");

                                        // 時間差でsignal
                                        event_timeline_->add([this]() {
                                            event_.signal("begin-records", EventParam());

                                            // 時間差で消滅
                                            event_timeline_->add([this]() {
                                                active_ = false;
                                              },
                                              event_timeline_->getCurrentTime() + deactive_delay_);
                                          },
                                          event_timeline_->getCurrentTime() + event_delay_);
                                      },
                                      event_timeline_->getCurrentTime() + tween_delay_);
                                  });
    
    connections_ += event.connect("settings-start",
                                  [this](const Connection&, EventParam& param) {
                                    view_->setActive(false);
                                    event_.signal("field-input-stop", EventParam());

                                    event_timeline_->add([this]() {
                                        view_->startWidgetTween("tween-out");

                                        event_timeline_->add([this]() {
                                            event_.signal("begin-settings", EventParam());

                                            event_timeline_->add([this]() {
                                                active_ = false;
                                              },
                                              event_timeline_->getCurrentTime() + deactive_delay_);
                                          },
                                          event_timeline_->getCurrentTime() + event_delay_);
                                      },
                                      event_timeline_->getCurrentTime() + tween_delay_);
                                  });
    
    connections_ += event.connect("credits-start",
                                  [this](const Connection&, EventParam& param) {
                                    view_->setActive(false);
                                    event_.signal("field-input-stop", EventParam());
                                    
                                    event_timeline_->add([this]() {
                                        view_->startWidgetTween("tween-out");

                                        event_timeline_->add([this]() {
                                            event_.signal("begin-credits", EventParam());

                                            event_timeline_->add([this]() {
                                                active_ = false;
                                              },
                                              event_timeline_->getCurrentTime() + deactive_delay_);
                                          },
                                          event_timeline_->getCurrentTime() + event_delay_);
                                      },
                                      event_timeline_->getCurrentTime() + tween_delay_);
                                  });
    
    connections_ += event.connect("leaderboard-start",
                                  [this](const Connection&, EventParam& param) {
                                    view_->setActive(false);
                                    event_.signal("field-input-stop", EventParam());
                                    
                                    event_timeline_->add([this]() {
                                        GameCenter::showBoard([this]() {
                                            AppSupport::pauseDraw(true);
                                          },
                                          [this]() {
                                            AppSupport::pauseDraw(false);
                                            
                                            event_.signal("field-input-start", EventParam());
                                            view_->setActive(true);
                                          });
                                      },
                                      event_timeline_->getCurrentTime() + tween_delay_);
                                  });
    
    setupView();

    // アプリ起動直後のタイトル画面か??
    bool startup = false;
    if (hasKey(exec_params, "title-startup")) {
      startup = boost::any_cast<bool>(exec_params.at("title-startup"));
    }
    // 各種menuから戻ってきた??
    bool from_menu = false;
    if (hasKey(exec_params, "menu-to-title")) {
      from_menu = boost::any_cast<bool>(exec_params.at("menu-to-title"));
    }

    {
      std::string jingle_name = "title.jingle-short";
      if (startup)        jingle_name = "title.jingle-full";
      else if (from_menu) jingle_name = "title.jingle-mini";
      
      requestSound(event_, params[jingle_name].getValue<std::string>());
    }
    
    event_.signal("field-input-start", EventParam());
    event_.signal("sound-title-start", EventParam());

    if (params.hasChild("title.active_delay")) {
      view_->setActive(false);

      float delay = params["title.active_delay"].getValue<float>();
      event_timeline_->add([this]() {
          view_->setActive(true);
        },
        event_timeline_->getCurrentTime() + delay);
    }
  }

  ~TitleController() {
    DOUT << "~TitleController()" << std::endl;

    // 再生途中のものもあるので、手動で取り除く
    event_timeline_->removeSelf();
  }


private:
  void dispLeaderBoard() {
    auto& widget = view_->getWidget("leaderboard");
    if (widget.isDisp()) return;
    
    widget.setDisp(true);
    widget.setActive(true);
  }
  
  void setupView() {
    // プレイ回数が0の場合はMenuをOFF
    if (records_.getTotalPlayNum() == 0) {
      std::string widget_names[] = {
        "records",
        "settings",
        "credits",
      };
      
      for (const auto& name : widget_names) {
        auto& widget = view_->getWidget(name);
        widget.setDisp(false);
        widget.setActive(false);
      }
    }
    else {
      if (GameCenter::isAuthenticated()) {
        dispLeaderBoard();
      }

      // 時間差で認証される事もありうる
      connections_ += event_.connect("gamecenter-authenticated",
                                     [this](const Connection&, EventParam& param) {
                                       if (GameCenter::isAuthenticated()) {
                                         dispLeaderBoard();
                                       }
                                     });
    }
    
    view_->startWidgetTween("tween-in");    
  }

  
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
