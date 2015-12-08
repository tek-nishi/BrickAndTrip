#pragma once

//
// Intro画面
//

#include "ControllerBase.hpp"
#include "UIView.hpp"
#include "ConnectionHolder.hpp"


namespace ngs {

class IntroController : public ControllerBase {
  ci::JsonTree& params_;
  Event<EventParam>& event_;

  std::unique_ptr<UIView> view_;
  
  bool active_;

  float tween_out_delay_;
  std::string jingle_se_;
  
  ci::TimelineRef event_timeline_;
  

public:
  IntroController(ci::JsonTree& params,
                  ci::TimelineRef timeline,
                  Event<EventParam>& event,
                  const int total_play_num,
                  std::unique_ptr<UIView>&& view) noexcept :
    params_(params),
    event_(event),
    view_(std::move(view)),
    active_(true),
    tween_out_delay_(params["intro.tween_out_delay"].getValue<float>()),
    jingle_se_(params["intro.jingle-se"].getValue<std::string>()),
    event_timeline_(ci::Timeline::create())
  {
    DOUT << "IntroController()" << std::endl;

    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    timeline->apply(event_timeline_);

    // プレイ回数でtween-outの時間を短くする
    ci::Vec2f tween_out_rate = Json::getVec2<float>(params_["intro.tween_out_delay_rate"]);
    tween_out_delay_ += (tween_out_rate.x - tween_out_delay_) / tween_out_rate.y * std::min(float(total_play_num), tween_out_rate.y);
    DOUT << "tween_out_delay:" << tween_out_delay_ << std::endl;
    
    event_timeline_->add([this]() {
        view_->setDisp(true);
        view_->startWidgetTween("tween-in");
        requestSound(event_, jingle_se_);
    
        event_timeline_->add([this]() {
            view_->startWidgetTween("tween-out");

            event_timeline_->add([this]() {
                // 初回起動
                EventParam params = {
                  { "title-startup", true }
                };
                
                event_.signal("begin-title", params);

                event_timeline_->add([this]() {
                    active_ = false;
                  },
                  event_timeline_->getCurrentTime() + params_["intro.deactive_delay"].getValue<float>());
              },
              event_timeline_->getCurrentTime() + params_["intro.event_delay"].getValue<float>());
          },
          event_timeline_->getCurrentTime() + tween_out_delay_);
      },
      event_timeline_->getCurrentTime() + params_["intro.tween_in_delay"].getValue<float>());

    view_->setDisp(false);
  }

  ~IntroController() noexcept {
    DOUT << "~IntroController()" << std::endl;

    // 再生途中のものもあるので、手動で取り除く
    event_timeline_->removeSelf();
  }


private:
  bool isActive() const noexcept override { return active_; }

  Event<EventParam>& event() noexcept override { return event_; }

  void resize() noexcept override {
  }
  
  void update(const double progressing_seconds) override { }
  
  void draw(FontHolder& fonts, ModelHolder& models) noexcept override {
    view_->draw(fonts, models);
  }
  
};

}
