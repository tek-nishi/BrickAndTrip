#pragma once

//
// 記録画面
//

#include "ControllerBase.hpp"
#include "UIView.hpp"
#include "ConnectionHolder.hpp"
#include "Share.h"
#include "Capture.h"
#include "Localize.h"
#include "GameCenter.h"


namespace ngs {

class RecordsController : public ControllerBase {
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
  RecordsController(ci::JsonTree& params,
                    ci::TimelineRef timeline,
                    Event<EventParam>& event,
                    const EventParam& records,
                    std::unique_ptr<UIView>&& view) :
    params_(params),
    event_(event),
    tween_delay_(params["records.tween_delay"].getValue<float>()),
    event_delay_(params["records.event_delay"].getValue<float>()),
    deactive_delay_(params["records.deactive_delay"].getValue<float>()),
    sns_delay_(params["records.sns_delay"].getValue<float>()),
    view_(std::move(view)),
    active_(true),
    event_timeline_(ci::Timeline::create())
  {
    DOUT << "RecordsController()" << std::endl;

    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    timeline->apply(event_timeline_);

    connections_ += event.connect("records-agree",
                                  [this](const Connection&, EventParam& param) {
                                    view_->setActive(false);

                                    event_timeline_->add([this]() {
                                        view_->startWidgetTween("tween-out");

                                        event_timeline_->add([this]() {
                                            EventParam params = {
                                              { "menu-to-title", true },
                                            };
                                            event_.signal("begin-title", params);

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

    setupView(params, records);
    
    view_->startWidgetTween("tween-in");
    requestSound(event_, params["records.jingle-se"].getValue<std::string>());

    if (params.hasChild("records.active_delay")) {
      view_->setActive(false);

      float delay = params["records.active_delay"].getValue<float>();
      event_timeline_->add([this]() {
          view_->setActive(true);
        },
        event_timeline_->getCurrentTime() + delay);
    }

    GameCenter::submitAchievement("BRICKTRIP.ACHIEVEMENT.VIEWED_RECORDS");
  }

  ~RecordsController() {
    DOUT << "~RecordsController()" << std::endl;

    // 再生途中のものもあるので、手動で取り除く
    event_timeline_->removeSelf();
  }


private:
  bool isActive() const noexcept override { return active_; }

  Event<EventParam>& event() noexcept override { return event_; }

  void resize() noexcept override { }
  
  void update(const double progressing_seconds) noexcept override { }
  
  void draw(FontHolder& fonts, ModelHolder& models) noexcept override {
    view_->draw(fonts, models);
  }


  void setupView(const ci::JsonTree& params, const EventParam& records) noexcept {
    {
      // constなのでatを使っている
      auto total_play = boost::any_cast<int>(records.at("total_play"));
      view_->getWidget("total_play").setText(toFormatedString(total_play, 4));
    }

    {
      auto total_time = boost::any_cast<double>(records.at("total_time"));
      view_->getWidget("total_time").setText(toFormatedString(total_time, true));
    }

    {
      auto total_score = boost::any_cast<int>(records.at("high_score"));
      view_->getWidget("high_score").setText(toFormatedString(total_score, 5));
    }
    
    {
      auto total_item = boost::any_cast<int>(records.at("total_item"));
      view_->getWidget("total_item").setText(toFormatedString(total_item, 5));
    }

    {
      const auto& stage_ranks = boost::any_cast<const std::vector<int>& >(records.at("stage_ranks"));

      auto& rank_text = params_["stageclear.rank"];
      
      {
        size_t stage_item = std::min(stage_ranks.size(), static_cast<size_t>(5));
        if (stage_item > 0) {
          std::string text;
          for (size_t i = 0; i < stage_item; ++i) {
            if (stage_ranks[i] == Records::RANK_DUMMY) {
              text += " ";
            }
            else {
              text += rank_text[stage_ranks[i]].getValue<std::string>();
            }
          }

          auto& widget = view_->getWidget("stage_item_1");
          widget.setText(text);
          widget.setDisp(true);
        }
      }

      {
        size_t stage_item = std::max(stage_ranks.size(), static_cast<size_t>(5)) - 5;
        if (stage_item > 0) {
          std::string text;
          for (size_t i = 0; i < stage_item; ++i) {
            if (stage_ranks[i + 5] == Records::RANK_DUMMY) {
              text += " ";
            }
            else {
              text += rank_text[stage_ranks[i + 5]].getValue<std::string>();
            }
          }
          
          auto& widget = view_->getWidget("stage_item_2");
          widget.setText(text);
          widget.setDisp(true);
        }
      }      
    }
    
    if (Capture::canExec() && Share::canPost()) {
      auto& widget = view_->getWidget("share");
        
      widget.setDisp(true);
      widget.setActive(true);
    }

    {
      auto text = params["records.sns_text"].getValue<std::string>();
      sns_text_ = Localize::get(text);
    }
  }
  
};

}
