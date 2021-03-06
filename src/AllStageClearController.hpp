﻿#pragma once

//
// all-stage clear
//

#include "ControllerBase.hpp"
#include "UIView.hpp"
#include "ConnectionHolder.hpp"
#include "Share.h"
#include "Capture.h"
#include "Localize.h"
#include "GameCenter.h"


namespace ngs {

class AllStageClearController : public ControllerBase {
  Event<EventParam>& event_;

  std::string message_;
  
  float event_delay_;
  float collapse_delay_;
  float tween_in_delay_;
  float tween_out_delay_;
  float titleback_delay_;
  float deactive_delay_;
  float sns_delay_;

  std::string jingle_se_;
  
  std::string sns_text_;
  std::string sns_url_;
  
  std::unique_ptr<UIView> view_;
  
  bool active_;

  ConnectionHolder connections_;

  ci::TimelineRef event_timeline_;


public:
  AllStageClearController(ci::JsonTree& params,
                          ci::TimelineRef timeline,
                          Event<EventParam>& event,
                          const EventParam& result,
                          std::unique_ptr<UIView>&& view) noexcept :
    event_(event),
    message_(params["message"].getValue<std::string>()),
    event_delay_(params["event_delay"].getValue<float>()),
    collapse_delay_(params["collapse_delay"].getValue<float>()),
    tween_in_delay_(params["tween_in_delay"].getValue<float>()),
    tween_out_delay_(params["tween_out_delay"].getValue<float>()),
    titleback_delay_(params["titleback_delay"].getValue<float>()),
    deactive_delay_(params["deactive_delay"].getValue<float>()),
    sns_delay_(params["sns_delay"].getValue<float>()),
    jingle_se_(params["jingle-se"].getValue<std::string>()),
    sns_url_(Localize::get(params["sns_url"].getValue<std::string>())),
    view_(std::move(view)),
    active_(true),
    event_timeline_(ci::Timeline::create())
  {
    DOUT << "AllStageClearController()" << std::endl;
    
    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    timeline->apply(event_timeline_);

    event_timeline_->add([this]() noexcept {
        // 演出開始
        event_.signal(message_, EventParam());

        // stage崩壊
        event_timeline_->add([this]() noexcept {
            event_.signal("collapse-stage", EventParam());
            requestSound(event_, "all-stage-collapse");
            

            // text表示
            event_timeline_->add([this]() noexcept {
                view_->setDisp(true);
                view_->setActive(true);
                view_->startWidgetTween("tween-in");
                requestSound(event_, jingle_se_);
              },
              event_timeline_->getCurrentTime() + tween_in_delay_);
          },
          event_timeline_->getCurrentTime() + collapse_delay_);
      },
      event_timeline_->getCurrentTime() + event_delay_);

    connections_ += event.connect("selected-agree",
                                  [this](const Connection&, EventParam& param) noexcept {
                                    view_->setActive(false);
                                    
                                    event_timeline_->add([this]() noexcept {
                                        view_->startWidgetTween("tween-out");

                                        event_timeline_->add([this]() noexcept {
                                            event_.signal("check-after-gameover", EventParam());
                                            event_.signal("back-to-title", EventParam());

                                            event_timeline_->add([this]() noexcept {
                                                active_ = false;
                                              },
                                              event_timeline_->getCurrentTime() + deactive_delay_);
                                          },
                                          event_timeline_->getCurrentTime() + titleback_delay_);
                                      },
                                      event_timeline_->getCurrentTime() + tween_out_delay_);
                                  });
    
    connections_ += event.connect("selected-share",
                                  [this](const Connection&, EventParam& param) noexcept {
                                    view_->setActive(false);
                                    
                                    event_timeline_->add([this]() noexcept {
                                        DOUT << "Share" << std::endl;

                                        AppSupport::pauseDraw(true);
                                        
                                        Share::post(sns_text_,
                                                    Capture::execute(),
                                                    [this]() noexcept {
                                                      AppSupport::pauseDraw(false);
                                                      view_->setActive(true);
                                                    });
                                      },
                                      event_timeline_->getCurrentTime() + sns_delay_);
                                  });
    
    setup(params, result);
  }

  ~AllStageClearController() {
    DOUT << "~AllStageClearController()" << std::endl;

    // 再生途中のものもあるので、手動で取り除く
    event_timeline_->removeSelf();
  }


private:
  void setup(const ci::JsonTree& params, const EventParam& result) noexcept {

    // SNS投稿で使うのでここで定義
    int item_rate = 0;
    {
      auto item_num = boost::any_cast<int>(result.at("play_item_num"));
      auto item_total_num = boost::any_cast<int>(result.at("play_item_total_num"));

      if (item_total_num > 0) {
        item_rate = item_num * 100 / item_total_num;
        view_->getWidget("item-result").setText(toFormatedString(item_rate, 3) + "%");
      }
      if (boost::any_cast<bool>(result.at("highest_item_num"))) {
        view_->startWidgetTween("tween-complete-item");
      }
    }
    
    auto game_score = boost::any_cast<int>(result.at("total_score"));
    view_->getWidget("score-result").setText(toFormatedString(game_score, 5));
    auto total_items = boost::any_cast<int>(result.at("total_items"));
    GameCenter::submitScore(game_score, total_items);

    if (boost::any_cast<bool>(result.at("highest_total_score"))) {
      view_->startWidgetTween("tween-hi-score");
    }
    
    view_->setDisp(false);
    view_->setActive(false);
    
    if (Capture::canExec() && Share::canPost()) {
      auto& widget = view_->getWidget("share");
        
      widget.setDisp(true);
      widget.setActive(true);
    }
    
    {
      // SNSへの投稿テキストはローカライズされたものを使う
      auto text = params["sns_text"].getValue<std::string>();
      sns_text_ = Localize::get(text);
      
      replaceString(sns_text_, "%1", std::to_string(game_score));
      replaceString(sns_text_, "%3", std::to_string(item_rate));

      auto stage_num = boost::any_cast<int>(result.at("current_stage"));
      replaceString(sns_text_, "%4", std::to_string(stage_num));
      replaceString(sns_text_, "%5", sns_url_);
    }
  }

  
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
