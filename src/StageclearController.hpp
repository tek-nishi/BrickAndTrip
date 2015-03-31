#pragma once

//
// Stage clear
//

#include <sstream>
#include "ControllerBase.hpp"
#include "UIView.hpp"
#include "ConnectionHolder.hpp"


namespace ngs {

class StageclearController : public ControllerBase {
  ci::JsonTree& params_;
  Event<EventParam>& event_;

  std::unique_ptr<UIView> view_;
  
  bool active_;

  ConnectionHolder connections_;

  ci::TimelineRef event_timeline_;


public:
  StageclearController(ci::JsonTree& params,
                       ci::TimelineRef timeline,
                       Event<EventParam>& event,
                       const EventParam& result,
                       std::unique_ptr<UIView>&& view) :
    params_(params),
    event_(event),
    view_(std::move(view)),
    active_(true),
    event_timeline_(ci::Timeline::create())
  {
    DOUT << "StageclearController()" << std::endl;
    
    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    timeline->apply(event_timeline_);

    {
      // constなのでatを使っている
      auto clear_time = boost::any_cast<double>(result.at("clear_time"));
      // 表示の最大時間は59:59.9
      clear_time = std::min(clear_time, 59 * 60 + 59 + 0.9);
      int minutes = int(clear_time) / 60;
      int seconds = int(clear_time) % 60;
      int milli_seconds = int(clear_time * 10.0) % 10;
      
      std::ostringstream str;
      str << std::setw(2) << std::setfill('0') << minutes << ":" << seconds
          << "." << std::setw(1) << milli_seconds;

      auto& widget = view_->getWidget("time-result");
      widget.getCubeText().setText(str.str());
    }

    {
      auto tumble_num = boost::any_cast<int>(result.at("tumble_num"));

      std::ostringstream str;
      str << std::setw(5) << std::setfill('0') << tumble_num;

      auto& widget = view_->getWidget("tumble-result");
      widget.getCubeText().setText(str.str());
    }
    
    view_->startWidgetTween("tween-in");

    event_timeline_->add([this]() {
        event_.signal("stageclear-agree", EventParam());
        active_ = false;
      },
      event_timeline_->getCurrentTime() + 3.0f);

    
#if 0
    connections_ += event.connect("stageclear-agree",
                                  [this](const Connection& connection, EventParam& param) {
                                    // 時間差tween
                                    event_timeline_->add([this]() {
                                        view_->startWidgetTween("tween-out");
                                      },
                                      event_timeline_->getCurrentTime() + 0.5f);
                                    
                                    // 時間差でControllerを破棄
                                    event_timeline_->add([this]() {
                                        
                                        active_ = false;
                                      },
                                      event_timeline_->getCurrentTime() + 1.0f);
                                    
                                    connection.disconnect();
                                  });
#endif
  }

  ~StageclearController() {
    DOUT << "~StageclearController()" << std::endl;

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
