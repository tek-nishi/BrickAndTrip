#pragma once

//
// ゲーム舞台のView
//

#include "cinder/Camera.h"
#include "cinder/gl/Light.h"
#include "cinder/gl/gl.h"
#include "Field.hpp"
#include "ConnectionHolder.hpp"
#include "EventParam.hpp"


namespace ngs {

class FieldView {
  const ci::JsonTree& params_;
  Event<EventParam>& event_;

  float fov_;
  float near_z_;
  float far_z_;
  ci::CameraPersp camera_;

  ci::Vec3f eye_point_;
  ci::Vec3f interest_point_;
  ci::Vec3f target_point_;

  std::vector<ci::gl::Light> lights_;

  ConnectionHolder connections_;


  // pick用情報
  struct TouchCube {
    u_int id;
    ci::Vec3f position;
    ci::Quatf rotation;
    float size;
    ci::AxisAlignedBox3f bbox;
  };

  std::vector<TouchCube> touch_cubes_;

  bool  picking_;
  u_int picking_touch_id_;
  double picking_touch_timestamp_;
  u_int picking_cube_id_;
  ci::Vec3f picking_plane_;
  ci::Vec3f picking_pos_;

  float move_threshold_;
  float move_speed_rate_;
  
  
public:
  FieldView(const ci::JsonTree& params, Event<EventParam>& event,
            Event<std::vector<ngs::Touch> >& touch_event) :
    params_(params),
    event_(event),
    fov_(params["game_view.fov"].getValue<float>()),
    near_z_(params["game_view.near_z"].getValue<float>()),
    far_z_(params["game_view.far_z"].getValue<float>()),
    camera_(ci::app::getWindowWidth(), ci::app::getWindowHeight(), fov_, near_z_, far_z_),
    eye_point_(Json::getVec3<float>(params["game_view.eye_point"])),
    interest_point_(Json::getVec3<float>(params["game_view.interest_point"])),
    target_point_(interest_point_),
    picking_(false),
    move_threshold_(params["game_view.move_threshold"].getValue<float>()),
    move_speed_rate_(params["game_view.move_speed_rate"].getValue<float>())
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
                                        std::bind(&FieldView::touchesBegan, this, std::placeholders::_1, std::placeholders::_2));
    // connections_ += touch_event.connect("touches-moved",
    //                                     std::bind(&FieldView::touchesMoved, this, std::placeholders::_1, std::placeholders::_2));
    connections_ += touch_event.connect("touches-ended",
                                        std::bind(&FieldView::touchesEnded, this, std::placeholders::_1, std::placeholders::_2));
  }

  
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

  // Fieldの表示
  void draw(const Field& field) {
    // FIXME:drawの中で、PikableCubeからTouch情報を生成している
    makeTouchCubeInfo(field.pickable_cubes);
    updateCamera(field.pickable_cubes);
    updateLight();
    
    ci::gl::pushMatrices();
    ci::gl::setMatrices(camera_);

    for (auto& light : lights_) {
      light.enable();
    }


    drawStageCubes(field.active_cubes);
    drawStageCubes(field.collapse_cubes);
    drawPickableCubes(field.pickable_cubes);

    
    for (auto& light : lights_) {
      light.disable();
    }

    ci::gl::popMatrices();
  }


  void cancelPicking(const u_int cube_id) {
    if (picking_ && (cube_id == picking_cube_id_)) {
      picking_ = false;
    }
  }
  

private:
  void touchesBegan(const Connection&, std::vector<ngs::Touch>& touches) {
    if (picking_) return;

    for (const auto& touch : touches) {
      auto ray = generateRay(touch.pos);
      for (auto& cube : touch_cubes_) {
        if (isPickedCube(cube.bbox, ray)) {
          picking_ = true;
          picking_touch_id_ = touch.id;
          picking_touch_timestamp_ = touch.timestamp;
          picking_cube_id_ = cube.id;

          // cubeの上平面との交点
          float cross_z;
          auto origin = ci::Vec3f(0, (cube.position.y + 0.5f) * cube.size, 0);
          ray.calcPlaneIntersection(origin, ci::Vec3f(0, 1, 0), &cross_z);
          picking_pos_   = ray.calcPosition(cross_z);
          picking_plane_ = origin;
          
          return;
        }
      } 
    }
  }
  
  void touchesMoved(const Connection&, std::vector<ngs::Touch>& touches) {
    if (!picking_) return;

    for (const auto& touch : touches) {
      if (touch.id != picking_touch_id_) continue;

      return;
    }
  }

  void touchesEnded(const Connection&, std::vector<ngs::Touch>& touches) {
    if (!picking_) return;
    for (const auto& touch : touches) {
      if (touch.id != picking_touch_id_) continue;

      for (auto& cube : touch_cubes_) {
        if (cube.id == picking_cube_id_) {
          // cubeの上平面との交点
          auto ray = generateRay(touch.pos);
          float cross_z;
          auto origin = ci::Vec3f(0, (cube.position.y + 0.5f) * cube.size, 0);
          ray.calcPlaneIntersection(origin, ci::Vec3f(0, 1, 0), &cross_z);
          auto picking_ofs = ray.calcPosition(cross_z) - picking_pos_;
          auto delta_time = touch.timestamp - picking_touch_timestamp_;

          int move_direction = PickableCube::MOVE_NONE;
          float move_threshold = cube.size * move_threshold_;
          int move_speed = 1;
          if (std::abs(picking_ofs.z) >= std::abs(picking_ofs.x)) {
            // 縦移動
            if (picking_ofs.z < -move_threshold) {
              move_direction = PickableCube::MOVE_DOWN;
              move_speed     = calcMoveSpeed(delta_time, -picking_ofs.z, move_threshold);
            }
            else if (picking_ofs.z > move_threshold) {
              move_direction = PickableCube::MOVE_UP;
              move_speed     = calcMoveSpeed(delta_time, picking_ofs.z, move_threshold);
            }
          }
          else {
            // 横移動
            if (picking_ofs.x < -move_threshold) {
              move_direction = PickableCube::MOVE_RIGHT;
              move_speed     = calcMoveSpeed(delta_time, -picking_ofs.x, move_threshold);
            }
            else if (picking_ofs.x > move_threshold) {
              move_direction = PickableCube::MOVE_LEFT;
              move_speed     = calcMoveSpeed(delta_time, picking_ofs.x, move_threshold);
            }
          }

          if (move_direction != PickableCube::MOVE_NONE) {
            EventParam params = {
              { "cube_id",        picking_cube_id_ },
              { "move_direction", move_direction },
              { "move_speed",     move_speed },
            };

            event_.signal("move-pickable", params);
          }
          
          picking_ = false;
          return;
        }
      }
    }
  }

  int calcMoveSpeed(const double delta_time, const float delta_position, const float move_threshold) {
    if (delta_position < move_threshold) {
      return 1;
    }
    return std::max(int(move_speed_rate_ * delta_position / delta_time), 1);
  }

  ci::Ray generateRay(const ci::Vec2f& pos) {
    float u = pos.x / (float) ci::app::getWindowWidth();
    float v = pos.y / (float) ci::app::getWindowHeight();
    // because OpenGL and Cinder use a coordinate system
    // where (0, 0) is in the LOWERleft corner, we have to flip the v-coordinate
    return std::move(camera_.generateRay(u, 1.0f - v, camera_.getAspectRatio()));
  }

  bool isPickedCube(ci::AxisAlignedBox3f& bbox, const ci::Ray& ray) {
    float z_cross[2];
    int num = bbox.intersect(ray, z_cross);
    return num > 0;
  }

  
  void makeTouchCubeInfo(const std::vector<std::unique_ptr<PickableCube> >& cubes) {
    touch_cubes_.clear();
    
    for (const auto& cube : cubes) {
      if (!cube->isActive() || !cube->canPick()) continue;

      const auto& pos = cube->position();
      const auto size = cube->size();
      TouchCube touch_cube = {
        cube->id(),
        pos,
        cube->rotation(),
        cube->cubeSize(),
        { -size / 2 + pos, size / 2 + pos }
      };
      
      touch_cubes_.push_back(std::move(touch_cube));
    }
  }

  // TODO:複数のPickableCubeをいい感じに捉える
  void updateCamera(const std::vector<std::unique_ptr<PickableCube> >& cubes) {
    ci::Vec3f target_pos = ci::Vec3f::zero();
    int cube_num = 0;
    for (const auto& cube : cubes) {
      if (!cube->isActive() || !cube->isOnStage()) continue;

      target_pos += cube->position();
      cube_num += 1;
    }

    if (cube_num > 0) {
      // FIXME:とりあえず中間点
      target_point_ = target_pos / cube_num;
    }

    auto d = (target_point_ - interest_point_) * 0.25f;
    interest_point_ += d;
    eye_point_ += d;

    camera_.setCenterOfInterestPoint(interest_point_);
    camera_.setEyePoint(eye_point_);
  }

  // TODO:複数のPickableCubeをいい感じに捉える
  void updateLight() {
    ci::Vec3f pos = lights_[0].getPosition();
    // pos.x = interest_point_.x;
    pos.z = interest_point_.z;
    lights_[0].setPosition(pos);
  }

  
  void drawStageCubes(const std::deque<std::vector<StageCube> >& cubes) {
    for (const auto& row : cubes) {
      for (const auto& cube : row) {
        if (!cube.active) continue;
        
        ci::gl::color(cube.color);

#if 0
        if (!cube.can_ride) {
          ci::gl::color(1, 0, 0);
        }
#endif

        ci::gl::pushModelView();
        ci::gl::translate(cube.position);
        ci::gl::rotate(cube.rotation);
      
        ci::gl::drawCube(ci::Vec3f::zero(), cube.size());
      
        ci::gl::popModelView();
      }
    }
  }

  void drawPickableCubes(const std::vector<std::unique_ptr<PickableCube> >& cubes) {
    for (const auto& cube : cubes) {
      if (!cube->isActive()) continue;

      if (picking_ && (picking_cube_id_ == cube->id())) {
        ci::gl::color(0, 0, 1);
      }
      else {
        ci::gl::color(cube->color());
      }      
      
      ci::gl::pushModelView();
      ci::gl::translate(cube->position());
      ci::gl::rotate(cube->rotation());
      
      ci::gl::drawCube(ci::Vec3f::zero(), cube->size());
      
      ci::gl::popModelView();
    }
  }

};

}
