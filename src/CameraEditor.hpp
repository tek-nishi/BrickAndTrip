#pragma once

//
// Camera調整
//

#include "cinder/Camera.h"
#include "cinder/params/Params.h"


namespace ngs {

class CameraEditor {
  ci::Camera& camera_;
  
  ci::params::InterfaceGlRef editor_;

  ci::Vec3f& interest_point_;
  ci::Vec3f& eye_point_;
  
  float fov_;
  float near_z_;
  float far_z_;

  float eye_rx_;
  float eye_ry_;
  float eye_distance_;
  

public:
  CameraEditor(ci::Camera& camera,
               ci::Vec3f& interest_point, ci::Vec3f& eye_point) :
    camera_(camera),
    editor_(ci::params::InterfaceGl::create("Camera", ci::Vec2i(300, 500))),
    interest_point_(interest_point),
    eye_point_(eye_point),
    fov_(camera.getFov()),
    near_z_(camera.getNearClip()),
    far_z_(camera.getFarClip()),
    eye_rx_(0),
    eye_ry_(0),
    eye_distance_(1)
  {
    editor_->addParam("interest pos", &interest_point);
    editor_->addParam("FOV", &fov_);
    editor_->addParam("near", &near_z_);
    editor_->addParam("far", &far_z_);
    editor_->addParam("eye rx", &eye_rx_);
    editor_->addParam("eye ry", &eye_ry_);
    editor_->addParam("eye dist", &eye_distance_);
  }


  void setParams(const float eye_rx, const float eye_ry, const float eye_distance) {
    eye_rx_ = eye_rx;
    eye_ry_ = eye_ry;
    eye_distance_ = eye_distance;
  }

  
  void draw() {
    camera_.setFov(fov_);
    camera_.setNearClip(near_z_);
    camera_.setFarClip(far_z_);

    eye_point_ = ci::Quatf(ci::Vec3f(1, 0, 0), ci::toRadians(eye_rx_))
      * ci::Quatf(ci::Vec3f(0, 1, 0), ci::toRadians(eye_ry_))
      * ci::Vec3f(0, 0, eye_distance_) + interest_point_;
    
    editor_->draw();
  }

private:
  


};

}
