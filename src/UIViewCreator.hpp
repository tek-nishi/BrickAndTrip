#pragma once

//
// UIView生成
//

#include "UIView.hpp"


namespace ngs {

class UIViewCreator {
  ci::Camera& camera_;
  Autolayout& autolayout_;
  Event<std::vector<Touch> >& touch_event_;


public:
  UIViewCreator(ci::Camera& camera, Autolayout& autolayout, Event<std::vector<Touch> >& touch_event) :
    camera_(camera),
    autolayout_(autolayout),
    touch_event_(touch_event)
  { }


  std::unique_ptr<UIView> create(const std::string& path) {
    return std::unique_ptr<UIView>(new UIView(ci::JsonTree(ci::app::loadAsset(path)), camera_, autolayout_, touch_event_));
  }
  
};

}
