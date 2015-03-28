﻿#pragma once

//
// アプリ最上位のController定義
//

#include "ControllerBase.hpp"
#include <boost/range/algorithm_ext/erase.hpp>
#include "FontHolder.hpp"
#include "FieldController.hpp"
#include "EventParam.hpp"
#include "TitleController.hpp"
#include "GameoverController.hpp"
#include "UIView.hpp"
#include "UIViewCreator.hpp"


namespace ngs {

class RootController : public ControllerBase {
  ci::JsonTree& params_;
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
  std::vector<ControllerPtr> children_;


public:
  RootController(ci::JsonTree& params, Event<std::vector<Touch> >& touch_event) :
    params_(params),
    ui_camera_(createCamera(params["ui_view"])),
    fov_(ui_camera_.getFov()),
    near_z_(ui_camera_.getNearClip()),
    far_z_(ui_camera_.getFarClip()),
    ui_lights_(createLights(params["ui_view.lights"])),
    autolayout_(ui_camera_),
    touch_event_(touch_event),
    view_creator_(ui_camera_, ui_lights_, autolayout_, touch_event),
    background_(Json::getColor<float>(params["app.background"]))
  {
    // TODO:最初のControllerを追加
    addController<FieldController>(params, touch_event_, event_);
    addController<TitleController>(params, event_, view_creator_.create("ui_title.json"));
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


  static ci::CameraPersp createCamera(const ci::JsonTree& params) {
    ci::CameraPersp camera(ci::app::getWindowWidth(), ci::app::getWindowHeight(),
                           params["fov"].getValue<float>(),
                           params["near_z"].getValue<float>(),
                           params["far_z"].getValue<float>());

    camera.setEyePoint(Json::getVec3<float>(params["eye_point"]));

    return std::move(camera);
  }

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
    
    return std::move(lights);
  }
  
  
};

}
