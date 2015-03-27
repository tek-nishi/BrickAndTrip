#pragma once

//
// アプリ最上位のController定義
//

#include "ControllerBase.hpp"
#include <boost/range/algorithm_ext/erase.hpp>
#include "FontHolder.hpp"
#include "FieldController.hpp"
#include "EventParam.hpp"
#include "TitleController.hpp"
#include "UIView.hpp"
#include "UIViewCreator.hpp"


namespace ngs {

class RootController : public ControllerBase {
  ci::JsonTree& params_;
  Event<std::vector<Touch> >& touch_event_;
  
  Event<EventParam> event_;

  ci::CameraPersp ui_camera_;
  Autolayout autolayout_;
  UIViewCreator view_creator_;

  ci::Color background_;
  
  using ControllerPtr = std::unique_ptr<ControllerBase>;
  std::vector<ControllerPtr> children_;


public:
  RootController(ci::JsonTree& params, Event<std::vector<Touch> >& touch_event) :
    params_(params),
    touch_event_(touch_event),
    ui_camera_(ci::app::getWindowWidth(), ci::app::getWindowHeight(),
               params["ui_view.fov"].getValue<float>(),
               params["ui_view.near_z"].getValue<float>(),
               params["ui_view.far_z"].getValue<float>()),
    view_creator_(ui_camera_, autolayout_, touch_event),
    background_(Json::getColor<float>(params["app.background"]))
  {
    ui_camera_.setEyePoint(Json::getVec3<float>(params["ui_view.eye_point"]));
    // コンストラクタでCameraの値を全て決められないので、ここで生成している
    autolayout_ = Autolayout(ui_camera_);
    
    // TODO:最初のControllerを追加
    addController<FieldController>(params, touch_event_, event_);
    addController<TitleController>(params, event_, view_creator_.create("ui_title.json"));
  }


private:
  bool isActive() const override { return true; }

  Event<EventParam>& event() override { return event_; }

  void resize() {
    for (auto& child : children_) {
      child->resize();
    }
  }
  
  void update(const double progressing_seconds) override {
    // 更新しつつ、無効なControllerを削除
    boost::remove_erase_if(children_,
                           [progressing_seconds](ControllerPtr& child) {
                             child->update(progressing_seconds);
                             return !child->isActive();
                           });
  }
  
  void draw(FontHolder& fonts) override {
    ci::gl::clear(background_);

    for (auto& child : children_) {
      child->draw(fonts);
    }
  }


  template<typename T, typename... Args>
  void addController(Args&&... args) {
    auto controller = std::unique_ptr<ControllerBase>(new T(std::forward<Args>(args)...));
    children_.push_back(std::move(controller));
  }
  
};

}
