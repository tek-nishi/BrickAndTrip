#pragma once

//
// UIView生成
//

#include "UIView.hpp"


namespace ngs {

class UIViewCreator {
  Autolayout& autolayout_;
  Event<std::vector<Touch>, ci::Camera>& touch_event_;


public:
  UIViewCreator(Autolayout& autolayout, Event<std::vector<Touch> >& touch_event) :
    autolayout_(autolayout),
    touch_event_(touch_event)
  { }


  std::unique_ptr<UIView> create(const std::string& path) {
    return std::unique_ptr<UIView>(new UIView(ci::JsonTree(ci::app::loadAsset(path)), autolayout_, touch_event_));
  }
  
};

}
