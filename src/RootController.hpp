#pragma once

//
// アプリ最上位のController定義
//

#include "ControllerBase.hpp"
#include <list>
#include <boost/range/algorithm_ext/erase.hpp>
#include "FontHolder.hpp"
#include "FieldController.hpp"
#include "EventParam.hpp"
#include "TitleController.hpp"
#include "GameoverController.hpp"
#include "StageclearController.hpp"
#include "ProgressController.hpp"
#include "PauseController.hpp"
#include "UIView.hpp"
#include "UIViewCreator.hpp"
// #include "TestPickController.hpp"


namespace ngs {

class RootController : public ControllerBase {
  ci::JsonTree& params_;
  ci::TimelineRef timeline_;
  Event<std::vector<Touch> >& touch_event_;
  
  Event<EventParam> event_;

  ci::CameraPersp ui_camera_;
  float fov_;
  float near_z_;
  float far_z_;
  
  std::vector<ci::gl::Light> ui_lights_;

  Autolayout autolayout_;
  UIViewCreator view_creator_;

  ci::Color background_;
  
  using ControllerPtr = std::unique_ptr<ControllerBase>;
  // TIPS:イテレート中にpush_backされるのでstd::listを使っている
  std::list<ControllerPtr> children_;


public:
  RootController(ci::JsonTree& params,
                 ci::TimelineRef timeline,
                 Event<std::vector<Touch> >& touch_event) :
    params_(params),
    timeline_(timeline),
    ui_camera_(createCamera(params["ui_view.camera"])),
    fov_(ui_camera_.getFov()),
    near_z_(ui_camera_.getNearClip()),
    far_z_(ui_camera_.getFarClip()),
    ui_lights_(createLights(params["ui_view.lights"])),
    autolayout_(ui_camera_),
    touch_event_(touch_event),
    view_creator_(timeline, ui_camera_, ui_lights_, autolayout_, event_, touch_event),
    background_(Json::getColor<float>(params["app.background"]))
  {
    // addController<TestPickController>(params, touch_event_, event_);
    addController<FieldController>(params, touch_event_, event_);
    addController<TitleController>(params, timeline_, event_, view_creator_.create("ui_title.json"));

    event_.connect("begin-progress",
                   [this](const Connection& connection, EventParam& param) {
                     addController<ProgressController>(params_, timeline_, event_, view_creator_.create("ui_progress.json"));
                   });
    
    event_.connect("begin-gameover",
                   [this](const Connection& connection, EventParam& param) {
                     addController<GameoverController>(params_, timeline_, event_, view_creator_.create("ui_gameover.json"));
                   });

    event_.connect("begin-stageclear",
                   [this](const Connection& connection, EventParam& param) {
                     addController<StageclearController>(params_, timeline_, event_, param, view_creator_.create("ui_stageclear.json"));
                   });

    event_.connect("begin-pause",
                   [this](const Connection& connection, EventParam& param) {
                     addController<PauseController>(params_, timeline_, event_, view_creator_.create("ui_pause.json"));
                   });

    event_.connect("begin-records",
                   [this](const Connection& connection, EventParam& param) {
                   });

    event_.connect("begin-credits",
                   [this](const Connection& connection, EventParam& param) {
                   });
  }


private:
  bool isActive() const override { return true; }

  Event<EventParam>& event() override { return event_; }

  void resize() {
    float aspect = ci::app::getWindowAspectRatio();
    ui_camera_.setAspectRatio(aspect);
    
    if (aspect < 1.0) {
      // 画面が縦長になったら、幅基準でfovを求める
      // fovとnear_zから投影面の幅の半分を求める
      float half_w = std::tan(ci::toRadians(fov_ / 2)) * near_z_;

      // 表示画面の縦横比から、投影面の高さの半分を求める
      float half_h = half_w / aspect;

      // 投影面の高さの半分とnear_zから、fovが求まる
      float fov = std::atan(half_h / near_z_) * 2;
      ui_camera_.setFov(ci::toDegrees(fov));
    }
    else {
      // 横長の場合、fovは固定
      ui_camera_.setFov(fov_);
    }

    autolayout_.resize(ui_camera_);
    
    for (auto& child : children_) {
      child->resize();
    }
  }
  
  void update(const double progressing_seconds) override {
    for (auto& controller : children_) {
      controller->update(progressing_seconds);
    }
    
    // 無効なControllerを削除
    boost::remove_erase_if(children_,
                           [](ControllerPtr& child) {
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


  // 汎用的なパラメーターからCameraを生成する
  static ci::CameraPersp createCamera(const ci::JsonTree& params) {
    ci::CameraPersp camera(ci::app::getWindowWidth(), ci::app::getWindowHeight(),
                           params["fov"].getValue<float>(),
                           params["near_z"].getValue<float>(),
                           params["far_z"].getValue<float>());

    camera.setEyePoint(Json::getVec3<float>(params["eye_point"]));

    return camera;
  }

  // 汎用的なパラメーターからLightを生成する
  static std::vector<ci::gl::Light> createLights(const ci::JsonTree& params) {
    int id = 0;
    std::vector<ci::gl::Light> lights;
    for (const auto& param : params) {
      auto light = ci::gl::Light(ci::gl::Light::POINT, id);

      light.setPosition(Json::getVec3<float>(param["pos"]));

      float constant_attenuation  = param["constant_attenuation"].getValue<float>();
      float linear_attenuation    = param["linear_attenuation"].getValue<float>();
      float quadratic_attenuation = param["quadratic_attenuation"].getValue<float>();
      light.setAttenuation(constant_attenuation,
                           linear_attenuation,
                           quadratic_attenuation);

      light.setDiffuse(Json::getColor<float>(param["diffuse"]));
      light.setAmbient(Json::getColor<float>(param["ambient"]));

      lights.push_back(std::move(light));
      
      ++id;
    }
    
    return lights;
  }
  
  
};

}
