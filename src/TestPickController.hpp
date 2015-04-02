#pragma once

//
// Cubeのpickテスト
//

namespace ngs {

class TestPickController : public ControllerBase {
  Event<EventParam>& event_;
  
  bool active_;

  ConnectionHolder connections_;

  float fov_;
  float near_z_;
  float far_z_;
  ci::CameraPersp camera_;

  ci::Vec3f eye_point_;
  ci::Vec3f interest_point_;

  std::vector<ci::gl::Light> lights_;

  ci::Vec3f position_;
  ci::Quatf rotation_;
  ci::Vec3f size_;

  bool  picking_;
  u_int picking_id_;
  bool  pick_over_;

  ci::AxisAlignedBox3f bbox_;
  

public:
  TestPickController(const ci::JsonTree& params,
                     Event<std::vector<Touch> >& touch_event,
                     Event<EventParam>& event) :
    event_(event),
    active_(true),
    fov_(params["game_view.fov"].getValue<float>()),
    near_z_(params["game_view.near_z"].getValue<float>()),
    far_z_(params["game_view.far_z"].getValue<float>()),
    camera_(ci::app::getWindowWidth(), ci::app::getWindowHeight(), fov_, near_z_, far_z_),
    eye_point_(Json::getVec3<float>(params["game_view.eye_point"])),
    interest_point_(Json::getVec3<float>(params["game_view.interest_point"])),
    position_(1, 1, 2),
    rotation_(ci::Quatf::identity()),
    size_(1, 1, 1),
    picking_(false),
    pick_over_(false),
    bbox_(position_ -size_ / 2, position_ + size_ / 2)
  {
    camera_.setEyePoint(eye_point_);
    camera_.setCenterOfInterestPoint(interest_point_);

    int id = 0;
    for (const auto& param : params["game_view.lights"]) {
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

      lights_.push_back(std::move(light));
      
      ++id;
    }

    connections_ += touch_event.connect("touches-began",
                                        std::bind(&TestPickController::touchesBegan,
                                                  this, std::placeholders::_1, std::placeholders::_2));
    connections_ += touch_event.connect("touches-moved",
                                        std::bind(&TestPickController::touchesMoved,
                                                  this, std::placeholders::_1, std::placeholders::_2));
    connections_ += touch_event.connect("touches-ended",
                                        std::bind(&TestPickController::touchesEnded,
                                                  this, std::placeholders::_1, std::placeholders::_2));
  }


private:
  bool isActive() const { return active_; }

  Event<EventParam>& event() { return event_; }

  void resize() {
    float aspect = ci::app::getWindowAspectRatio();
    camera_.setAspectRatio(aspect);
    if (aspect < 1.0) {
      // 画面が縦長になったら、幅基準でfovを求める
      // fovとnear_zから投影面の幅の半分を求める
      float half_w = std::tan(ci::toRadians(fov_ / 2)) * near_z_;

      // 表示画面の縦横比から、投影面の高さの半分を求める
      float half_h = half_w / aspect;

      // 投影面の高さの半分とnear_zから、fovが求まる
      float fov = std::atan(half_h / near_z_) * 2;
      camera_.setFov(ci::toDegrees(fov));
    }
    else {
      // 横長の場合、fovは固定
      camera_.setFov(fov_);
    }
  }
  
  void update(const double progressing_seconds) {
    ci::Quatf rot(ci::Vec3f(1, 0, 0), (M_PI / 2) * progressing_seconds);
    rotation_ = rotation_ * rot;

    ci::Matrix44f transform;
    transform.translate(position_);
    transform = transform * rotation_.toMatrix44();
    
    bbox_ = ci::AxisAlignedBox3f(-size_ / 2, size_ / 2).transformed(transform);
  }
  
  void draw(FontHolder& fonts) {
    ci::gl::setMatrices(camera_);

    ci::gl::enable(GL_LIGHTING);
    for (auto& light : lights_) {
      light.enable();
    }

    if (pick_over_) {
      ci::gl::color(0, 0, 1);
    }
    else {
      ci::gl::color(1, 1, 1);
    }
    
    ci::gl::pushModelView();
    ci::gl::translate(position_);
    ci::gl::rotate(rotation_);
      
    ci::gl::drawCube(ci::Vec3f::zero(), size_);
    
    ci::gl::popModelView();

    for (auto& light : lights_) {
      light.disable();
    }
    ci::gl::disable(GL_LIGHTING);

    ci::gl::color(0, 1, 0);
    ci::gl::drawStrokedCube(bbox_);
  }
  

  void touchesBegan(const Connection&, std::vector<Touch>& touches) {
    if (picking_) return;
    
    for (const auto& touch : touches) {
      auto ray = generateRay(touch.pos);
      if (bbox_.intersects(ray)) {
        picking_    = true;
        pick_over_  = true;
        picking_id_ = touch.id;
        return;
      }
    }
  }

  void touchesMoved(const Connection&, std::vector<Touch>& touches) {
    if (!picking_) return;

    for (const auto& touch : touches) {
      if (touch.id != picking_id_) continue;
      auto ray = generateRay(touch.pos);
      pick_over_ = bbox_.intersects(ray);
      return;
    }
  }

  void touchesEnded(const Connection&, std::vector<Touch>& touches) {
    if (!picking_) return;

    for (const auto& touch : touches) {
      if (touch.id != picking_id_) continue;

      picking_   = false;
      pick_over_ = false;
    }
  }

  ci::Ray generateRay(const ci::Vec2f& pos) {
    float u = pos.x / (float) ci::app::getWindowWidth();
    float v = pos.y / (float) ci::app::getWindowHeight();
    // because OpenGL and Cinder use a coordinate system
    // where (0, 0) is in the LOWERleft corner, we have to flip the v-coordinate
    return camera_.generateRay(u, 1.0f - v, camera_.getAspectRatio());
  }

};

}
