#pragma once

//
// UIView生成
//

#include <boost/noncopyable.hpp>
#include "UIView.hpp"
#include "JsonUtil.hpp"


namespace ngs {

class UIViewCreator : private boost::noncopyable {
  ci::TimelineRef timeline_;

  ci::JsonTree& params_;
  
  ci::Camera& camera_;

  Autolayout& autolayout_;
  Event<EventParam>& event_;
  Event<std::vector<Touch> >& touch_event_;


public:
  UIViewCreator(ci::JsonTree& params,
                ci::TimelineRef timeline,
                ci::Camera& camera,
                Autolayout& autolayout,
                Event<EventParam>& event,
                Event<std::vector<Touch> >& touch_event) :
    timeline_(timeline),
    params_(params),
    camera_(camera),
    autolayout_(autolayout),
    event_(event),
    touch_event_(touch_event)
  {  }


  std::unique_ptr<UIView> create(const std::string& path) noexcept {
    // ちょくちょくAutolayoutのお掃除 
    autolayout_.eraseInvalid();

    return std::unique_ptr<UIView>(new UIView(params_,
                                              Json::readFromFile(path),
                                              timeline_,
                                              camera_, autolayout_, event_, touch_event_));
  }


private:

  
};

}
