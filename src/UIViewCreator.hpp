#pragma once

//
// UIView生成
//

#include "UIView.hpp"


namespace ngs {

class UIViewCreator {
  ci::Camera& camera_;
  std::vector<ci::gl::Light>& lights_;

  Autolayout& autolayout_;
  Event<std::vector<Touch> >& touch_event_;



public:
  UIViewCreator(ci::Camera& camera, std::vector<ci::gl::Light>& lights,
                Autolayout& autolayout, Event<std::vector<Touch> >& touch_event) :
    camera_(camera),
    lights_(lights),
    autolayout_(autolayout),
    touch_event_(touch_event)
  {  }


  std::unique_ptr<UIView> create(const std::string& path) {
    return std::unique_ptr<UIView>(new UIView(ci::JsonTree(ci::app::loadAsset(path)),
                                              camera_, lights_, autolayout_, touch_event_));
  }


private:

  
};

}
