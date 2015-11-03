#pragma once

//
// Game Over
//

#include "ControllerBase.hpp"
#include "UIView.hpp"
#include "ConnectionHolder.hpp"
#include "Share.h"
#include "Capture.h"
#include "Localize.h"
#include "GameCenter.h"


namespace ngs {

class GameoverController : public ControllerBase {
  ci::JsonTree& params_;
  Event<EventParam>& event_;

  float tween_delay_;
  float event_delay_;
  float deactive_delay_;
  float sns_delay_;

  std::unique_ptr<UIView> view_;
  
  bool active_;

  std::string sns_text_;

  ConnectionHolder connections_;

  ci::TimelineRef event_timeline_;


public:
  GameoverController(ci::JsonTree& params,
                     ci::TimelineRef timeline,
                     Event<EventParam>& event,
                     const EventParam& event_params,
                     std::unique_ptr<UIView>&& view) :
    params_(params),
    event_(event),
    tween_delay_(params["gameover.tween_delay"].getValue<float>()),
    event_delay_(params["gameover.event_delay"].getValue<float>()),
    deactive_delay_(params["gameover.deactive_delay"].getValue<float>()),
    sns_delay_(params["gameover.sns_delay"].getValue<float>()),
    view_(std::move(view)),
    active_(true),
    event_timeline_(ci::Timeline::create())
  {
    DOUT << "GameoverController()" << std::endl;
    
    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    timeline->apply(event_timeline_);

    connections_ += event.connect("gameover-agree",
                                  [this](const Connection&, EventParam& param) {
                                    view_->setActive(false);
                                    
                                    event_timeline_->add([this]() {
                                        view_->startWidgetTween("tween-out");

                                        event_timeline_->add([this]() {
                                            event_.signal("check-after-gameover", EventParam());
                                            event_.signal("begin-title", EventParam());
                                            
                                            event_timeline_->add([this]() {
                                                active_ = false;
                                              },
                                              event_timeline_->getCurrentTime() + deactive_delay_);
                                          },
                                          event_timeline_->getCurrentTime() + event_delay_);
                                      },
                                      event_timeline_->getCurrentTime() + tween_delay_);
                                  });

    connections_ += event.connect("gameover-continue",
                                  [this](const Connection&, EventParam& param) {
                                    view_->setActive(false);
                                    
                                    event_timeline_->add([this]() {
                                        view_->startWidgetTween("tween-out");

                                        event_timeline_->add([this]() {
                                            event_.signal("continue-game", EventParam());
                                            
                                            event_timeline_->add([this]() {
                                                active_ = false;
                                              },
                                              event_timeline_->getCurrentTime() + deactive_delay_);
                                          },
                                          event_timeline_->getCurrentTime() + event_delay_);
                                      },
                                      event_timeline_->getCurrentTime() + tween_delay_);
                                  });

    connections_ += event.connect("selected-share",
                                  [this](const Connection&, EventParam& param) {
                                    view_->setActive(false);
                                    
                                    event_timeline_->add([this]() {
                                        DOUT << "Share" << std::endl;

                                        AppSupport::pauseDraw(true);
                                        
                                        Share::post(sns_text_,
                                                    Capture::execute(),
                                                    [this]() {
                                                      AppSupport::pauseDraw(false);
                                                      view_->setActive(true);
                                                    });
                                      },
                                      event_timeline_->getCurrentTime() + sns_delay_);
                                  });
    
    // 再開できるかどうかの判断
    if (boost::any_cast<bool>(event_params.at("can_continue"))) {
      view_->getWidget("continue").setDisp(true);
      view_->getWidget("continue").setActive(true);
      view_->getWidget("done").setDisp(true);
      view_->getWidget("done").setActive(true);

      view_->getWidget("agree").setDisp(false);
      view_->getWidget("agree").setActive(false);
    }

    
    auto game_score = boost::any_cast<int>(event_params.at("score"));
    view_->getWidget("score-result").setText(toFormatedString(game_score, 5));
    auto total_items = boost::any_cast<int>(event_params.at("total_items"));
    GameCenter::submitScore(game_score, total_items);

    if (game_score == 0) {
      GameCenter::submitAchievement("BRICKTRIP.ACHIEVEMENT.NO_SCORE");
    }
    
    if (boost::any_cast<bool>(event_params.at("hi_score"))) {
      view_->startWidgetTween("tween-hi-score");
    }
    
    view_->startWidgetTween("tween-in");
    requestSound(event_, params["gameover.jingle-se"].getValue<std::string>());

    if (params.hasChild("gameover.active_delay")) {
      view_->setActive(false);

      float delay = params["gameover.active_delay"].getValue<float>();
      event_timeline_->add([this]() {
          view_->setActive(true);
        },
        event_timeline_->getCurrentTime() + delay);
    }

    if (Capture::canExec() && Share::canPost()) {
      auto& widget = view_->getWidget("share");
        
      widget.setDisp(true);
      widget.setActive(true);
    }

    {
      // SNSへの投稿テキストはローカライズされたものを使う
      auto text = params["gameover.sns_text"].getValue<std::string>();
      
      sns_text_ = Localize::get(text);
      replaceString(sns_text_, "%1", std::to_string(game_score));
    }
  }

  ~GameoverController() {
    DOUT << "~GameoverController()" << std::endl;

    // 再生途中のものもあるので、手動で取り除く
    event_timeline_->removeSelf();
  }


private:
  bool isActive() const noexcept override {
    return active_;
  }

  Event<EventParam>& event() noexcept override { return event_; }

  void resize() noexcept override {
  }
  
  void update(const double progressing_seconds) noexcept override {
  }
  
  void draw(FontHolder& fonts, ModelHolder& models) noexcept override {
    view_->draw(fonts, models);
  }

};

}
