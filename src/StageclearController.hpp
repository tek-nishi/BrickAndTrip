#pragma once

//
// Stage clear
//

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

    {
      // constなのでatを使っている
      auto clear_time = boost::any_cast<double>(result.at("clear_time"));
      view_->getWidget("time-result").getCubeText().setText(toFormatedString(clear_time));
    }

    {
      auto tumble_num = boost::any_cast<int>(result.at("tumble_num"));

      std::ostringstream str;
      str << std::setw(5) << std::setfill('0') << tumble_num;

      view_->getWidget("tumble-result").getCubeText().setText(str.str());
    }

    {
      auto operation_num = boost::any_cast<int>(result.at("operation_num"));

      std::ostringstream str;
      str << std::setw(5) << std::setfill('0') << operation_num;

      view_->getWidget("operation-result").getCubeText().setText(str.str());
    }
    
    {
      auto item_num = boost::any_cast<int>(result.at("item_num"));
      auto& widget = view_->getWidget("item-result");
      if (!item_num) {
        widget.setDisp(false);
      }
      else {
        std::string text(item_num, ' ');
        widget.setText(text);
      }
    }

    // 全ステージクリア用設定
    bool all_cleard = boost::any_cast<bool>(result.at("all_cleared"));
    if (all_cleard) {
      view_->getWidget("try").setDisp(false);
      view_->getWidget("next").setDisp(false);
    }
    
    view_->startWidgetTween("tween-in");

    event_timeline_->add([this, all_cleard]() {
        event_.signal(all_cleard ? "all-stageclear-agree"
                                 : "stageclear-agree", EventParam());
        active_ = false;
      },
      event_timeline_->getCurrentTime() + 3.0f);
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
  
  void draw(FontHolder& fonts, Model& cube, Model& text) override {
    view_->draw(fonts, cube, text);
  }

};

}
