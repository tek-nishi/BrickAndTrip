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

  float tween_delay_;
  float event_delay_;
  float deactive_delay_;

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
    tween_delay_(params["records.tween_delay"].getValue<float>()),
    event_delay_(params["records.event_delay"].getValue<float>()),
    deactive_delay_(params["records.deactive_delay"].getValue<float>()),
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
                                    view_->setActive(false);

                                    event_timeline_->add([this]() {
                                        view_->startWidgetTween("tween-out");

                                        event_timeline_->add([this]() {
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

    setupView(records);
    
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
  
  void draw(FontHolder& fonts, ModelHolder& models) override {
    view_->draw(fonts, models);
  }


  void setupView(const EventParam& records) {
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
      auto total_tumble = boost::any_cast<int>(records.at("total_tumble"));
      view_->getWidget("total_tumble").setText(toFormatedString(total_tumble, 5));
    }
    
    {
      auto total_operation = boost::any_cast<int>(records.at("total_operation"));
      view_->getWidget("total_operation").setText(toFormatedString(total_operation, 5));
    }
    
    {
      auto total_item = boost::any_cast<int>(records.at("total_item"));
      view_->getWidget("total_item").setText(toFormatedString(total_item, 5));
    }
    
    {
      auto total_clear = boost::any_cast<int>(records.at("total_clear"));
      view_->getWidget("total_clear").setText(toFormatedString(total_clear, 4));
    }

    {
      const auto& item_completed = boost::any_cast<const std::deque<bool>& >(records.at("item_completed"));

      {
        size_t stage_item = std::min(item_completed.size(), static_cast<size_t>(5));
        if (stage_item > 0) {
          std::string text;
          for (size_t i = 0; i < stage_item; ++i) {
            text = text + (item_completed[i] ? "t" : " ");
          }

          auto& widget = view_->getWidget("stage_item_1");
          widget.setText(text);
          widget.setDisp(true);
        }
      }

      {
        size_t stage_item = std::max(item_completed.size(), static_cast<size_t>(5)) - 5;
        if (stage_item > 0) {
          std::string text;
          for (size_t i = 0; i < stage_item; ++i) {
            text = text + (item_completed[i + 5] ? "t" : " ");
          }
          
          auto& widget = view_->getWidget("stage_item_2");
          widget.setText(text);
          widget.setDisp(true);
        }
      }      
    }
  }
  
};

}
