#pragma once

//
// 記録画面
//

#include "ControllerBase.hpp"
#include "UIView.hpp"
#include "ConnectionHolder.hpp"


namespace ngs {

class RecordsController : public ControllerBase {
  ci::JsonTree& params_;
  Event<EventParam>& event_;

  std::unique_ptr<UIView> view_;
  
  bool active_;

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
    view_(std::move(view)),
    active_(true),
    event_timeline_(ci::Timeline::create())
  {
    DOUT << "RecordsController()" << std::endl;

    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    timeline->apply(event_timeline_);

    connections_ += event.connect("records-agree",
                                  [this](const Connection& connection, EventParam& param) {
                                    view_->startWidgetTween("tween-out");

                                    // 時間差でControllerを破棄
                                    event_timeline_->add([this]() {
                                        event_.signal("begin-title", EventParam());
                                        active_ = false;
                                      },
                                      event_timeline_->getCurrentTime() + 1.5f);
                                    
                                    connection.disconnect();
                                  });

    {
      // constなのでatを使っている
      auto total_play = boost::any_cast<int>(records.at("total_play"));
      view_->getWidget("total_play").getCubeText().setText(toFormatedString(total_play, 4));
    }

    {
      auto total_time = boost::any_cast<double>(records.at("total_time"));
      view_->getWidget("total_time").getCubeText().setText(toFormatedString(total_time, true));
    }
    
    {
      auto total_tumble = boost::any_cast<int>(records.at("total_tumble"));
      view_->getWidget("total_tumble").getCubeText().setText(toFormatedString(total_tumble, 5));
    }
    
    {
      auto total_operation = boost::any_cast<int>(records.at("total_operation"));
      view_->getWidget("total_operation").getCubeText().setText(toFormatedString(total_operation, 5));
    }
    
    {
      auto total_item = boost::any_cast<int>(records.at("total_item"));
      view_->getWidget("total_item").getCubeText().setText(toFormatedString(total_item, 5));
    }
    
    {
      auto total_clear = boost::any_cast<int>(records.at("total_clear"));
      view_->getWidget("total_clear").getCubeText().setText(toFormatedString(total_clear, 4));
    }
    
    view_->startWidgetTween("tween-in");
  }

  ~RecordsController() {
    DOUT << "~RecordsController()" << std::endl;

    // 再生途中のものもあるので、手動で取り除く
    event_timeline_->removeSelf();
  }


private:
  bool isActive() const override { return active_; }

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
