#pragma once

//
// ゲーム舞台のView
//

#include "cinder/Camera.h"
#include "cinder/gl/Light.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/ImageIo.h"
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/noncopyable.hpp>
#include <numeric>
#include "Field.hpp"
#include "ConnectionHolder.hpp"
#include "EventParam.hpp"
#include "ModelHolder.hpp"
#include "MaterialHolder.hpp"
#include "FieldLights.hpp"
#include "Quake.hpp"
// #include "CameraEditor.hpp"


namespace ngs {

class FieldView : private boost::noncopyable {
  const ci::JsonTree& params_;
  Event<EventParam>& event_;

  float fov_;
  float near_z_;
  float far_z_;
  ci::CameraPersp camera_;

  ci::Vec3f interest_point_;
  ci::Vec3f eye_point_;
  ci::Vec3f target_point_;
  float target_radius_;

  float eye_distance_rate_;
  float eye_offset_rate_;

  ci::Vec3f new_target_point_;
  ci::Vec3f new_eye_point_;
  
  float camera_speed_;
  bool camera_follow_target_;

  FieldLights lights_;

  ConnectionHolder connections_;

  Quake quake_;
  float quake_value_;
  
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
  double progressing_seconds_;
  
  // CameraEditor camera_editor_;

  MaterialHolder materials_;

  ci::gl::Texture bg_texture_;
  
  std::string bg_tween_type_;
  float bg_tween_duration_;

  ci::ColorA fog_color_rate_;

  ci::Anim<ci::Color> bg_color_;
  ci::Anim<ci::ColorA> fog_color_;
  
  
public:
  FieldView(ci::JsonTree& params,
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
    target_point_(Json::getVec3<float>(params["game_view.camera.target_point"])),
    new_target_point_(target_point_),
    target_radius_(0.0f),
    eye_distance_rate_(params["game_view.camera.eye_distance_rate"].getValue<float>()),
    eye_offset_rate_(params["game_view.camera.eye_offset_rate"].getValue<float>()),
    camera_speed_(1.0 - params["game_view.camera.speed"].getValue<float>()),
    camera_follow_target_(true),
    lights_(params, timeline),
    quake_(params["game_view.quake"]),
    quake_value_(0.0f),
    move_threshold_(params["game_view.move_threshold"].getValue<float>()),
    move_speed_rate_(params["game_view.move_speed_rate"].getValue<float>()),
    touch_input_(true),
    animation_timeline_(ci::Timeline::create()),
    progressing_seconds_(0.0),
    bg_texture_(ci::loadImage(ci::app::loadAsset("bg.png"))),
    bg_tween_type_(params["game_view.bg_tween_type"].getValue<std::string>()),
    bg_tween_duration_(params["game_view.bg_tween_duration"].getValue<float>()),
    fog_color_rate_(Json::getColorA<float>(params["game_view.fog_color"])),
    bg_color_(ci::Color(0, 0, 0)),
    fog_color_(ci::ColorA(0, 0, 0, 1))
    // camera_editor_(camera_, interest_point_, eye_point_)
  {
    // 注視点からの距離、角度でcamera位置を決めている
    float eye_rx = params["game_view.camera.eye_rx"].getValue<float>();
    float eye_ry = params["game_view.camera.eye_ry"].getValue<float>();
    float eye_distance = params["game_view.camera.eye_distance"].getValue<float>();
    
    eye_point_ = ci::Quatf(ci::Vec3f(1, 0, 0), ci::toRadians(eye_rx))
               * ci::Quatf(ci::Vec3f(0, 1, 0), ci::toRadians(eye_ry))
               * ci::Vec3f(0, 0, eye_distance) + interest_point_;
    
    camera_.setCenterOfInterestPoint(interest_point_ + target_point_);
    camera_.setEyePoint(eye_point_ + target_point_);

    new_eye_point_ = eye_point_;

    // camera_editor_.setParams(eye_rx, eye_ry, eye_distance);

    readMaterials(params["game_view.materials"]);
    
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
    // 経過時間だけ記録しておいて、drawで計算する
    progressing_seconds_ = progressing_seconds;
  }

  
  // Fieldの表示
  void draw(const Field& field, ModelHolder& models) {
    // FIXME:drawの中で、PikableCubeからTouch情報を生成している
    makeTouchCubeInfo(field.pickable_cubes);
    updateCameraTarget(field.pickable_cubes);
    updateCamera(progressing_seconds_);
    lights_.updateLights(target_point_);

    // 遠景(一枚絵)
    {
      ci::CameraOrtho camera(0, 255, 255, 0, -1, 1);
      ci::gl::setMatrices(camera);
        
      ci::gl::disableDepthRead();
      ci::gl::disableDepthWrite();
      ci::gl::disable(GL_LIGHTING);
      ci::gl::color(bg_color_);
      ci::gl::draw(bg_texture_);
    }

    ci::gl::enableDepthRead();
    ci::gl::enableDepthWrite();
    ci::gl::enable(GL_LIGHTING);

    ci::gl::enable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogfv(GL_FOG_COLOR, fog_color_().ptr());
    glFogf(GL_FOG_START, params_["game_view.fog_start"].getValue<float>());
    glFogf(GL_FOG_END, params_["game_view.fog_end"].getValue<float>());
    
    ci::gl::setMatrices(camera_);

    lights_.enableLights();

    drawStageCubes(field.active_cubes, models);
    drawStageCubes(field.collapse_cubes, models);

    drawCubes(field.pickable_cubes, models, "pickable_cube", "pickable_cube");
    drawCubes(field.item_cubes, models, "item_cube", "item_cube");
    drawCubes(field.moving_cubes, models, "pickable_cube", "moving_cube");
    drawCubes(field.falling_cubes, models, "pickable_cube", "falling_cube");
    drawCubes(field.switches, models, "switch", "switch");
    
    // bgのfogは別設定
    glFogf(GL_FOG_START, params_["game_view.bg_fog_start"].getValue<float>());
    glFogf(GL_FOG_END, params_["game_view.bg_fog_end"].getValue<float>());

    drawBgCubes(field.bg_cubes, models);

#ifdef DEBUG
    // drawBgBbox(field.bg_bbox_min, field.bg_bbox_max);
#endif

    lights_.disableLights();
    ci::gl::disable(GL_FOG);

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

  void setStageBgColor(const ci::Color& color) {
    animation_timeline_->apply(&bg_color_,
                               color,
                               bg_tween_duration_, getEaseFunc(bg_tween_type_));

    ci::ColorA fog_color = ci::ColorA(color.r, color.g, color.b, 1) * fog_color_rate_;
    
    animation_timeline_->apply(&fog_color_,
                               fog_color,
                               bg_tween_duration_, getEaseFunc(bg_tween_type_));
  }

  void setStageLightTween(const std::string& tween_name) {
    lights_.startLightTween(tween_name);
  }

  void enableFollowCamera(const bool enable = true) {
    camera_follow_target_ = enable;
  }

  void startQuake(const float duration) {
    quake_.start(*animation_timeline_, &quake_value_, duration);
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
    return bbox.intersects(ray);
  }

  
  void makeTouchCubeInfo(const std::vector<std::unique_ptr<PickableCube> >& cubes) {
    touch_cubes_.clear();
    
    for (const auto& cube : cubes) {
      if (!cube->isActive() || !cube->isOnStage() || cube->isSleep() || cube->isPressed()) continue;

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
    if (!camera_follow_target_) return;
    
    std::vector<ci::Vec3f> cube_pos;
    for (const auto& cube : cubes) {
      if (!cube->isActive() || !cube->isOnStage() || cube->isSleep()) continue;

      cube_pos.push_back(cube->position());
    }

    if (cube_pos.empty()) return;
    
    // FIXME:とりあえず中間点
    new_target_point_ = std::accumulate(std::begin(cube_pos), std::end(cube_pos), ci::Vec3f::zero()) / cube_pos.size();
    target_radius_ = 0.0f;
    for (const auto& pos : cube_pos) {
      target_radius_ = std::max(new_target_point_.distance(pos), target_radius_);
    }

    // 中心点から一番離れたpickable cubeへの距離に応じて注視点を移動
    {
      auto d = ci::Vec2f(eye_point_.x, eye_point_.z).normalized() * target_radius_ * eye_offset_rate_;
      new_target_point_.x += d.x;
      new_target_point_.z += d.y;
    }
    
    // 中心点から一番離れたpickable cubeへの距離に応じてカメラを引く
    float eye_rx = params_["game_view.camera.eye_rx"].getValue<float>();
    float eye_ry = params_["game_view.camera.eye_ry"].getValue<float>();
    float eye_distance = params_["game_view.camera.eye_distance"].getValue<float>() + target_radius_ * eye_distance_rate_;
      
    new_eye_point_ = ci::Quatf(ci::Vec3f(1, 0, 0), ci::toRadians(eye_rx))
      * ci::Quatf(ci::Vec3f(0, 1, 0), ci::toRadians(eye_ry))
      * ci::Vec3f(0, 0, eye_distance) + interest_point_;
  }

  void updateCamera(const double progressing_seconds) {
    // 等加速運動の近似
    float speed_rate = std::pow(camera_speed_, progressing_seconds / (1 / 60.0));
    {
      auto d = new_target_point_ - target_point_;
      target_point_ = new_target_point_ - d * speed_rate;
    }
    {
      auto d = new_eye_point_ - eye_point_;
      eye_point_ = new_eye_point_ - d * speed_rate;
    }

    auto offset = ci::Vec3f(0.0f, quake_value_, 0.0f);
    
    camera_.setCenterOfInterestPoint(interest_point_ + target_point_ + offset);
    camera_.setEyePoint(eye_point_ + target_point_ + offset);
  }

  
  void drawStageCubes(const std::deque<std::vector<StageCube> >& cubes,
                      ModelHolder& models) {
    auto& material = materials_.get("stage_cube");
    material.apply();

    const auto& mesh = models.get("stage_cube").mesh();
    
    for (const auto& row : cubes) {
      for (const auto& cube : row) {
        if (!cube.active) continue;
        
        ci::gl::color(cube.color);

        ci::gl::pushModelView();
        ci::gl::translate(cube.position);
        // TIPS:stagecubeはrotateとscalingが無い

        ci::gl::draw(mesh);
      
        ci::gl::popModelView();
      }
    }

#if 0
    ci::gl::pushModelView();
    ci::gl::translate(new_target_point_);
    ci::gl::rotate(ci::Vec3f(90, 0, 0));
    
    ci::gl::drawStrokedCircle(ci::Vec2f::zero(), target_radius_);
    ci::gl::popModelView();
#endif
  }


  template<typename T>
  void drawCubes(const std::vector<T>& cubes,
                 ModelHolder& models,
                 const std::string& model_name, const std::string& material_name) {
    auto& material = materials_.get(material_name);
    material.apply();

    const auto& mesh = models.get(model_name).mesh();

    for (const auto& cube : cubes) {
      if (!cube->isActive()) continue;

      ci::gl::color(cube->color());
      
      ci::gl::pushModelView();
      ci::gl::translate(cube->position());
      // FIXME:通常のscaleが1.0で、pickableが潰された時のみscaleが変わる
      //       ので、回転の後でscaleを掛けている
      ci::gl::scale(cube->size());

      // TIPS:gl::rotate(Quarf)は、内部でglRotatefを使っている
      //      この計算が正しく求まらない状況があるため、Quarf->Matrix
      //      にしている。これだと問題ない
      glMultMatrixf(cube->rotation().toMatrix44());

      ci::gl::draw(mesh);
      
      ci::gl::popModelView();
    }    
  }


#if 0
  void drawPickableCubes(const std::vector<std::unique_ptr<PickableCube> >& cubes,
                         ModelHolder& models) {
    auto& material = materials_.get("pickable_cube");
    material.apply();
    
    for (const auto& cube : cubes) {
      if (!cube->isActive()) continue;

      ci::gl::color(cube->color());
      
      ci::gl::pushModelView();
      ci::gl::translate(cube->position());
      ci::gl::rotate(cube->rotation());
      ci::gl::scale(cube->size());

      ci::gl::draw(models.get("pickable_cube").mesh());
      
      ci::gl::popModelView();

#if 0
      ci::AxisAlignedBox3f bbox(cube->position() - cube->size() / 2,
                                cube->position() + cube->size() / 2);

      ci::gl::color(0, 1, 0);
      ci::gl::drawStrokedCube(bbox);
#endif
    }
  }
#endif
  
  void drawBgCubes(const std::vector<Bg::Cube>& cubes,
                   ModelHolder& models) {
    auto& material = materials_.get("bg_cube");
    material.apply();

    const auto& mesh = models.get("bg_cube").mesh();
    
    for (const auto& cube : cubes) {
      ci::gl::color(cube.color);
      
      ci::gl::pushModelView();
      ci::gl::translate(cube.position);
      ci::gl::scale(cube.size);

      ci::gl::draw(mesh);
      
      ci::gl::popModelView();
    }
  }


#ifdef DEBUG
  void drawBgBbox(const ci::Vec3f& bbox_min, const ci::Vec3f& bbox_max) {
    ci::AxisAlignedBox3f bbox(bbox_min, bbox_max);
    
    ci::gl::color(0, 1, 0);
    ci::gl::drawStrokedCube(bbox);
  }
#endif
  

  void readMaterials(const ci::JsonTree& params) {
    for (const auto& p : params) {
      materials_.add(p.getKey(), p);
    }
  }

};

}
