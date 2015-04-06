#pragma once

//
// ゲーム舞台のView
//

#include "cinder/Camera.h"
#include "cinder/gl/Light.h"
#include "cinder/gl/gl.h"
#include <boost/range/algorithm_ext/erase.hpp>
#include "Field.hpp"
#include "ConnectionHolder.hpp"
#include "EventParam.hpp"
// #include "CameraEditor.hpp"


namespace ngs {

class FieldView {
  const ci::JsonTree& params_;
  Event<EventParam>& event_;

  float fov_;
  float near_z_;
  float far_z_;
  ci::CameraPersp camera_;

  ci::Vec3f interest_point_;
  ci::Vec3f eye_point_;
  ci::Vec3f target_point_;
  ci::Vec3f new_target_point_;
  float camera_speed_;

  struct Light {
    ci::gl::Light l;
    ci::Vec3f pos;
  };
  
  std::vector<Light> lights_;

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

  struct Pick {
    u_int touch_id;
    double timestamp;
    u_int cube_id;

    ci::Vec3f picking_plane;
    ci::Vec3f picking_pos;
  };
  std::vector<Pick> pickings_;

  float move_threshold_;
  float move_speed_rate_;
  
  bool touch_input_;

  ci::TimelineRef animation_timeline_;
  
  // CameraEditor camera_editor_;


  
public:
  FieldView(const ci::JsonTree& params,
            ci::TimelineRef timeline,
            Event<EventParam>& event,
            Event<std::vector<Touch> >& touch_event) :
    params_(params),
    event_(event),
    fov_(params["game_view.camera.fov"].getValue<float>()),
    near_z_(params["game_view.camera.near_z"].getValue<float>()),
    far_z_(params["game_view.camera.far_z"].getValue<float>()),
    camera_(ci::app::getWindowWidth(), ci::app::getWindowHeight(), fov_, near_z_, far_z_),
    interest_point_(Json::getVec3<float>(params["game_view.camera.interest_point"])),
    eye_point_(interest_point_),
    target_point_(Json::getVec3<float>(params["game_view.camera.target_point"])),
    new_target_point_(target_point_),
    camera_speed_(params["game_view.camera.speed"].getValue<float>()),
    move_threshold_(params["game_view.move_threshold"].getValue<float>()),
    move_speed_rate_(params["game_view.move_speed_rate"].getValue<float>()),
    touch_input_(true),
    animation_timeline_(ci::Timeline::create())
    // camera_editor_(camera_, interest_point_, eye_point_)
  {
    // 注視点からの距離、角度でcamera位置を決めている
    float eye_rx = params["game_view.camera.eye_rx"].getValue<float>();
    float eye_ry = params["game_view.camera.eye_ry"].getValue<float>();
    float eye_distance = params["game_view.camera.eye_distance"].getValue<float>();
    
    eye_point_ = ci::Quatf(ci::Vec3f(1, 0, 0), ci::toRadians(eye_rx))
      * ci::Quatf(ci::Vec3f(0, 1, 0), ci::toRadians(eye_ry))
      * ci::Vec3f(0, 0, eye_distance) + interest_point_;
    
    camera_.setCenterOfInterestPoint(interest_point_);
    camera_.setEyePoint(eye_point_);

    // camera_editor_.setParams(eye_rx, eye_ry, eye_distance);
    
    
    int id = 0;
    for (const auto& param : params["game_view.lights"]) {
      Light light = {
        { ci::gl::Light::POINT, id },
        Json::getVec3<float>(param["pos"])
      };
      light.l.setPosition(light.pos);

      float constant_attenuation  = param["constant_attenuation"].getValue<float>();
      float linear_attenuation    = param["linear_attenuation"].getValue<float>();
      float quadratic_attenuation = param["quadratic_attenuation"].getValue<float>();
      light.l.setAttenuation(constant_attenuation,
                             linear_attenuation,
                             quadratic_attenuation);

      light.l.setDiffuse(ci::hsvToRGB(Json::getHsvColor(param["diffuse"])));
      light.l.setAmbient(ci::hsvToRGB(Json::getHsvColor(param["ambient"])));

      lights_.push_back(std::move(light));
      
      ++id;
    }

    auto current_time = timeline->getCurrentTime();
    animation_timeline_->setStartTime(current_time);
    timeline->apply(animation_timeline_);

    connections_ += touch_event.connect("touches-began",
                                        std::bind(&FieldView::touchesBegan, this, std::placeholders::_1, std::placeholders::_2));
    // connections_ += touch_event.connect("touches-moved",
    //                                     std::bind(&FieldView::touchesMoved, this, std::placeholders::_1, std::placeholders::_2));
    connections_ += touch_event.connect("touches-ended",
                                        std::bind(&FieldView::touchesEnded, this, std::placeholders::_1, std::placeholders::_2));
  }
  
  ~FieldView() {
    // 再生途中のものもあるので、手動で取り除く
    animation_timeline_->removeSelf();
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


  // 時間経過での計算が必要なもの
  void update(const double progressing_seconds) {
    auto d = new_target_point_ - target_point_;
    target_point_ += d * 0.1f;
    
    camera_.setCenterOfInterestPoint(interest_point_ + target_point_);
    camera_.setEyePoint(eye_point_ + target_point_);
  }

  
  // Fieldの表示
  void draw(const Field& field) {
    // FIXME:drawの中で、PikableCubeからTouch情報を生成している
    makeTouchCubeInfo(field.pickable_cubes);
    updateCameraTarget(field.pickable_cubes);
    updateLight();

    ci::gl::enable(GL_LIGHTING);
    ci::gl::enableDepthRead();
    ci::gl::enableDepthWrite();
    
    ci::gl::setMatrices(camera_);

    for (auto& light : lights_) {
      light.l.enable();
    }


    drawStageCubes(field.active_cubes);
    drawStageCubes(field.collapse_cubes);
    drawPickableCubes(field.pickable_cubes);
    drawItemCubes(field.item_cubes);

    
    for (auto& light : lights_) {
      light.l.disable();
    }

#if 0
    ci::gl::disable(GL_LIGHTING);
    ci::gl::disableDepthRead();
    ci::gl::disableDepthWrite();
    
    camera_editor_.draw();
#endif
  }


  void cancelPicking(const u_int cube_id) {
    boost::remove_erase_if(pickings_,
                           [cube_id](const Pick& pick) {
                             return pick.cube_id == cube_id;
                           });
  }

  void calcelAllPickings() {
    pickings_.clear();
  }

  
  // Touch入力の有効・無効
  void enableTouchInput(const bool input = true) {
    if (input != touch_input_) calcelAllPickings();
    touch_input_ = input;
  }

  
  void resetCamera() {
    new_target_point_ = Json::getVec3<float>(params_["game_view.camera.target_point"]);
  }

  void setStageColor(const ci::Color& color) {
  }
  
  
private:
  void touchesBegan(const Connection&, std::vector<Touch>& touches) {
    if (!touch_input_) return;

    for (const auto& touch : touches) {
      // if (isPicking(touch)) continue;
      
      auto ray = generateRay(touch.pos);
      for (auto& cube : touch_cubes_) {
        if (isTouching(cube.id)) continue;
        
        if (isPickedCube(cube.bbox, ray)) {
          // cubeの上平面との交点
          float cross_z;
          auto origin = ci::Vec3f(0, (cube.position.y + 0.5f) * cube.size, 0);
          ray.calcPlaneIntersection(origin, ci::Vec3f(0, 1, 0), &cross_z);
          
          Pick pick = {
            touch.id,
            touch.timestamp,
            cube.id,
            origin,
            ray.calcPosition(cross_z),
          };
          pickings_.push_back(std::move(pick));

          EventParam params = {
            { "cube_id", cube.id },
          };
          event_.signal("picking-start", params);
          
          break;
        }
      } 
    }
  }
  
#if 0
  void touchesMoved(const Connection&, std::vector<Touch>& touches) {
    if (!picking_) return;

    for (const auto& touch : touches) {
      if (touch.id != picking_touch_id_) continue;

      return;
    }
  }
#endif

  void touchesEnded(const Connection&, std::vector<Touch>& touches) {
    if (!touch_input_) return;
    
    for (const auto& touch : touches) {
      if (!isPicking(touch)) continue;

      auto pick = getPick(touch);
      assert(pick);
      
      for (auto& cube : touch_cubes_) {
        // if (!isTouching(cube.id)) continue;

        if (cube.id == pick->cube_id) {
          // cubeの上平面との交点
          auto ray = generateRay(touch.pos);
          float cross_z;
          auto origin = ci::Vec3f(0, (cube.position.y + 0.5f) * cube.size, 0);
          ray.calcPlaneIntersection(origin, ci::Vec3f(0, 1, 0), &cross_z);
          auto picking_ofs = ray.calcPosition(cross_z) - pick->picking_pos;
          auto delta_time = touch.timestamp - pick->timestamp;

          float move_threshold = cube.size * move_threshold_;
          int move_direction = PickableCube::MOVE_NONE;
          int move_speed     = 0;
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

          EventParam params = {
            { "cube_id",        pick->cube_id },
            { "move_direction", move_direction },
            { "move_speed",     move_speed },
          };
          event_.signal("move-pickable", params);

          removePick(touch);
          break;
        }
      }
    }
  }

  
  bool isPicking(const Touch& touch) {
    for (const auto& pick : pickings_) {
      if (touch.id == pick.touch_id) return true;
    }
    return false;
  }

  bool isTouching(const u_int cube_id) {
    for (const auto& pick : pickings_) {
      if (cube_id == pick.cube_id) return true;
    }
    return false;
  }

  boost::optional<Pick&> getPick(const Touch& touch) {
    for (auto& pick : pickings_) {
      if (touch.id == pick.touch_id) return boost::optional<Pick&>(pick);
    }
    return boost::optional<Pick&>();
  }
  
  boost::optional<Pick&> getTouching(const u_int cube_id) {
    for (auto& pick : pickings_) {
      if (cube_id == pick.cube_id) return boost::optional<Pick&>(pick);
    }
    return boost::optional<Pick&>();
  }



  
  void removePick(const Touch& touch) {
    boost::remove_erase_if(pickings_,
                           [touch](const Pick& pick) {
                             return pick.touch_id == touch.id;
                           });
  }

  bool isCubeExists(const u_int id) {
    for (const auto& cube : touch_cubes_) {
      if (cube.id == id) return true;
    }
    return false;
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
    return camera_.generateRay(u, 1.0f - v, camera_.getAspectRatio());
  }

  bool isPickedCube(ci::AxisAlignedBox3f& bbox, const ci::Ray& ray) {
    float z_cross[2];
    int num = bbox.intersect(ray, z_cross);
    return num > 0;
  }

  
  void makeTouchCubeInfo(const std::vector<std::unique_ptr<PickableCube> >& cubes) {
    touch_cubes_.clear();
    
    for (const auto& cube : cubes) {
      if (!cube->isActive() || !cube->isOnStage() || cube->isSleep()) continue;

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

    // Pick中なのに含まれないCubeを削除
    boost::remove_erase_if(pickings_,
                           [this](const Pick& pick) {
                             return !isCubeExists(pick.cube_id);
                           });
  }

  void updateCameraTarget(const std::vector<std::unique_ptr<PickableCube> >& cubes) {
    ci::Vec3f target_pos = ci::Vec3f::zero();
    int cube_num = 0;
    for (const auto& cube : cubes) {
      if (!cube->isActive() || !cube->isOnStage() || cube->isSleep()) continue;

      target_pos += cube->position();
      cube_num += 1;
    }

    if (cube_num > 0) {
      // FIXME:とりあえず中間点
      new_target_point_ = target_pos / cube_num;
    }
  }

  void updateLight() {
    for (auto& light : lights_) {
      ci::Vec3f pos = light.pos;
      // 注視点のzだけ拝借
      pos.z += target_point_.z;
      light.l.setPosition(pos);
    }
  }

  
  void drawStageCubes(const std::deque<std::vector<StageCube> >& cubes) {
    for (const auto& row : cubes) {
      for (const auto& cube : row) {
        if (!cube.active) continue;
        
        ci::gl::color(cube.color);

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

      ci::gl::color(cube->color());
      
      ci::gl::pushModelView();
      ci::gl::translate(cube->position());
      ci::gl::rotate(cube->rotation());
      
      ci::gl::drawCube(ci::Vec3f::zero(), cube->size());
      
      ci::gl::popModelView();

#if 0
      ci::AxisAlignedBox3f bbox(cube->position() - cube->size() / 2,
                                cube->position() + cube->size() / 2);

      ci::gl::color(0, 1, 0);
      ci::gl::drawStrokedCube(bbox);
#endif
    }
  }

  void drawItemCubes(const std::vector<std::unique_ptr<ItemCube> >& cubes) {
    for (const auto& cube : cubes) {
      if (!cube->isActive()) continue;

      ci::gl::color(cube->color());
      
      ci::gl::pushModelView();
      ci::gl::translate(cube->position());
      ci::gl::rotate(cube->rotation());
      
      ci::gl::drawCube(ci::Vec3f::zero(), cube->size() * 0.5f);
      
      ci::gl::popModelView();
    }
  }

};

}
