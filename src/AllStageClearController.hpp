#pragma once

//
// all-stage clear
//

#include "ControllerBase.hpp"
#include "UIView.hpp"
#include "ConnectionHolder.hpp"


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
  
  std::unique_ptr<UIView> view_;
  
  bool active_;

  ConnectionHolder connections_;

  ci::TimelineRef event_timeline_;


public:
  AllStageClearController(ci::JsonTree& params,
                          ci::TimelineRef timeline,
                          Event<EventParam>& event,
                          const EventParam& result,
                          std::unique_ptr<UIView>&& view) :
    event_(event),
    message_(params["message"].getValue<std::string>()),
    event_delay_(params["event_delay"].getValue<float>()),
    collapse_delay_(params["collapse_delay"].getValue<float>()),
    tween_in_delay_(params["tween_in_delay"].getValue<float>()),
    tween_out_delay_(params["tween_out_delay"].getValue<float>()),
    titleback_delay_(params["titleback_delay"].getValue<float>()),
    deactive_delay_(params["deactive_delay"].getValue<float>()),
    view_(std::move(view)),
    active_(true),
    event_timeline_(ci::Timeline::create())
  {
    DOUT << "AllStageClearController()" << std::endl;
    
    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    timeline->apply(event_timeline_);

    event_timeline_->add([this]() {
        // 演出開始
        event_.signal(message_, EventParam());

        // stage崩壊
        event_timeline_->add([this]() {
            event_.signal("collapse-stage", EventParam());

            // text表示
            event_timeline_->add([this]() {
                view_->setDisp(true);
                view_->startWidgetTween("tween-in");

                // text消去
                event_timeline_->add([this]() {
                    view_->startWidgetTween("tween-out");

                    // title処理へ
                    event_timeline_->add([this]() {
                        event_.signal("back-to-title", EventParam());

                        // 終了
                        event_timeline_->add([this]() {
                            active_ = false;
                          },
                          event_timeline_->getCurrentTime() + deactive_delay_);
                      },
                      event_timeline_->getCurrentTime() + titleback_delay_);
                  },
                  event_timeline_->getCurrentTime() + tween_out_delay_);
              },
              event_timeline_->getCurrentTime() + tween_in_delay_);
          },
          event_timeline_->getCurrentTime() + collapse_delay_);
      },
      event_timeline_->getCurrentTime() + event_delay_);

    {
      auto score = boost::any_cast<int>(result.at("total_score"));
      view_->getWidget("score-result").setText(toFormatedString(score, 4));

      if (boost::any_cast<bool>(result.at("highest_score"))) {
        view_->startWidgetTween("tween-highest-score");
      }
    }
    
    view_->setDisp(false);
  }

  ~AllStageClearController() {
    DOUT << "~AllStageClearController()" << std::endl;

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
