#pragma once

//
// ゲーム舞台のView
//

#include <numeric>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/algorithm/clamp.hpp>
#include <boost/noncopyable.hpp>
#include <cinder/Camera.h>
#include <cinder/gl/Light.h>
#include <cinder/gl/gl.h>
#include <cinder/gl/Texture.h>
#include <cinder/ImageIo.h>
#include <cinder/Frustum.h>
#include "Field.hpp"
#include "ConnectionHolder.hpp"
#include "EventParam.hpp"
#include "ModelHolder.hpp"
#include "MaterialHolder.hpp"
#include "FieldLights.hpp"
#include "Quake.hpp"
#include "SoundRequest.hpp"


namespace ngs {

class FieldView : private boost::noncopyable {
  const ci::JsonTree& params_;
  Event<EventParam>& event_;

#ifdef DEBUG
  bool debug_info_;
#endif
  
  float fov_;
  float near_z_;
  float far_z_;
  ci::CameraPersp camera_;
  ci::Frustumf frustum_;

  ci::Anim<ci::Vec3f> interest_point_;
  ci::Anim<float> eye_rx_;
  ci::Anim<float> eye_ry_;
  ci::Anim<float> eye_distance_;

  float eye_distance_rate_;
  float eye_offset_rate_;

  float close_distance_rate_;
  ci::Anim<float> distance_rate_;
  
  float target_radius_;
  float new_target_radius_;

  ci::Vec3f target_point_;
  ci::Vec3f new_target_point_;

  ci::Vec3f eye_point_;
  
  float camera_speed_;
  bool camera_follow_target_;
  bool camera_look_id_;
  u_int looking_cube_id_;

  FieldLights lights_;

  ConnectionHolder connections_;

  Quake quake_;
  float quake_value_;
  
  // pick用情報
  struct TouchCube {
    u_int                id;
    ci::Vec3f            position;
    ci::Quatf            rotation;
    ci::AxisAlignedBox3f bbox;

    TouchCube(const u_int id_,
              const ci::Vec3f& position_, const ci::Quatf& rotation_,
              const ci::AxisAlignedBox3f& bbox_) noexcept :
      id(id_),
      position(position_),
      rotation(rotation_),
      bbox(bbox_)
    {}
  };

  std::vector<TouchCube> touch_cubes_;

  struct Pick {
    u_int     touch_id;
    ci::Vec2f touch_begin_pos;
    double    timestamp;
    u_int     cube_id;
    bool      began_move;

    Pick(const u_int touch_id_,
         const ci::Vec2f& touch_begin_pos_, const double timestamp_,
         const u_int cube_id_, const bool began_move_) :
      touch_id(touch_id_),
      touch_begin_pos(touch_begin_pos_),
      timestamp(timestamp_),
      cube_id(cube_id_),
      began_move(began_move_)
    {}
  };
  std::vector<Pick> pickings_;

  float move_begin_time_;
  float move_begin_threshold_;
  float move_threshold_;
  float move_speed_rate_;
  int   move_swipe_speed_;
  float distance_revise_;
  
  bool touch_input_;

  ci::TimelineRef animation_timeline_;
  double progressing_seconds_;

  MaterialHolder materials_;

  ci::gl::Texture bg_texture_;
  
  std::string bg_tween_type_;
  float bg_tween_duration_;

  ci::ColorA fog_color_rate_;
  float fog_start_;
  float fog_end_;
  float bg_fog_start_;
  float bg_fog_end_;

  ci::Anim<ci::Color> bg_color_;

  ci::Anim<ci::ColorA> fog_color_;

  std::vector<std::string> oneway_models_;
  ci::Anim<int> oneway_index_;

  float shadow_alpha_;

  ci::Area  bg_area_;
  ci::Rectf bg_rect_;
  
  
public:
  FieldView(ci::JsonTree& params,
            ci::TimelineRef timeline,
            Event<EventParam>& event,
            Event<std::vector<Touch> >& touch_event) noexcept :
    params_(params),
    event_(event),
#ifdef DEBUG
    debug_info_(params["game_view.debug_info"].getValue<bool>()),
#endif
    fov_(params["game_view.camera.fov"].getValue<float>()),
    near_z_(params["game_view.camera.near_z"].getValue<float>()),
    far_z_(params["game_view.camera.far_z"].getValue<float>()),
    camera_(ci::app::getWindowWidth(), ci::app::getWindowHeight(), fov_, near_z_, far_z_),
    frustum_(camera_),
    eye_distance_rate_(params["game_view.camera.eye_distance_rate"].getValue<float>()),
    eye_offset_rate_(params["game_view.camera.eye_offset_rate"].getValue<float>()),
    close_distance_rate_(params["game_view.camera.close_distance_rate"].getValue<float>()),
    distance_rate_(1.0f),
    target_radius_(0.0f),
    new_target_radius_(0.0f),
    camera_speed_(1.0 - params["game_view.camera.speed"].getValue<float>()),
    camera_follow_target_(true),
    camera_look_id_(false),
    lights_(params, timeline),
    quake_(params["game_view.quake"]),
    quake_value_(0.0f),
    move_begin_time_(params["game_view.move_begin_time"].getValue<float>()),
    move_begin_threshold_(params["game_view.move_begin_threshold"].getValue<float>()),
    move_threshold_(params["game_view.move_threshold"].getValue<float>()),
    move_speed_rate_(params["game_view.move_speed_rate"].getValue<float>()),
    move_swipe_speed_(params["game_view.move_swipe_speed"].getValue<int>()),
    distance_revise_(params["game_view.distance_revise"].getValue<float>()),
    touch_input_(true),
    animation_timeline_(ci::Timeline::create()),
    progressing_seconds_(0.0),
    bg_texture_(ci::loadImage(ci::app::loadAsset("bg.png"))),
    bg_tween_type_(params["game_view.bg_tween_type"].getValue<std::string>()),
    bg_tween_duration_(params["game_view.bg_tween_duration"].getValue<float>()),
    fog_color_rate_(Json::getColorA<float>(params["game_view.fog_color"])),
    fog_start_(params_["game_view.fog_start"].getValue<float>()),
    fog_end_(params_["game_view.fog_end"].getValue<float>()),
    bg_fog_start_(params_["game_view.bg_fog_start"].getValue<float>()),
    bg_fog_end_(params_["game_view.bg_fog_end"].getValue<float>()),
    bg_color_(ci::Color(0, 0, 0)),
    fog_color_(ci::ColorA(0, 0, 0, 1)),
    oneway_models_(Json::getArray<std::string>(params["game_view.oneway.model"])),
    shadow_alpha_(params_["game_view.shadow_alpha"].getValue<float>()),
    bg_rect_(0, 256, 256, 0)
  {
    setCameraParams(params["game_view.camera.start_camera"].getValue<std::string>());
    
    camera_.setCenterOfInterestPoint(target_point_ + interest_point_);
    camera_.setEyePoint(target_point_ + eye_point_);

    readMaterials(params["game_view.materials"]);
    
    auto current_time = timeline->getCurrentTime();
    animation_timeline_->setStartTime(current_time);
    timeline->apply(animation_timeline_);

    connections_ += touch_event.connect("touches-began",
                                        std::bind(&FieldView::touchesBegan, this, std::placeholders::_1, std::placeholders::_2));
    connections_ += touch_event.connect("touches-moved",
                                        std::bind(&FieldView::touchesMoved, this, std::placeholders::_1, std::placeholders::_2));
    connections_ += touch_event.connect("touches-ended",
                                        std::bind(&FieldView::touchesEnded, this, std::placeholders::_1, std::placeholders::_2));

    setupOneway(params);
    setQuality(params["app.low_efficiency_device"].getValue<bool>());

    float aspect = ci::app::getWindowAspectRatio();
    setupBg(aspect);
  }
  
  ~FieldView() {
    // 再生途中のものもあるので、手動で取り除く
    animation_timeline_->removeSelf();
  }

  
  void resize() noexcept {
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

    setupBg(aspect);
  }


  // 時間経過での計算が必要なもの
  void update(const double progressing_seconds) noexcept {
    // 経過時間だけ記録しておいて、drawで計算する
    progressing_seconds_ = progressing_seconds;
  }

  
  // Fieldの表示
  void draw(const Field& field, ModelHolder& models) noexcept {
    // FIXME:drawの中で、PikableCubeからTouch情報を生成している
    makeTouchCubeInfo(field.pickable_cubes);
    updateCameraTarget(field.pickable_cubes);
    updateCamera(progressing_seconds_);
    lights_.updateLights(target_point_);

    // 遠景(一枚絵)
    {
      ci::CameraOrtho camera(0, 256, 0, 256, -1, 1);
      ci::gl::setMatrices(camera);
        
      ci::gl::disableDepthRead();
      ci::gl::disableDepthWrite();
      ci::gl::disable(GL_LIGHTING);
      ci::gl::color(bg_color_);
      ci::gl::draw(bg_texture_, bg_area_, bg_rect_);
    }

    ci::gl::enableDepthRead();
    ci::gl::enableDepthWrite();
    ci::gl::enable(GL_LIGHTING);

    ci::gl::enable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogfv(GL_FOG_COLOR, fog_color_().ptr());
    glFogf(GL_FOG_START, fog_start_);
    glFogf(GL_FOG_END, fog_end_);
    
    ci::gl::setMatrices(camera_);

    // 視錐台(クリッピングに利用)
    frustum_.set(camera_);

    lights_.enableLights();

    drawStageCubes(field.active_cubes, models, frustum_);
    drawStageCubes(field.collapse_cubes, models, frustum_);

    drawCubeShadow(field.item_cubes, models, "item_shadow", "item_shadow");

    auto pickable_matrix = [](const ci::Vec3f& position, const ci::Quatf& rotation, const ci::Vec3f& size) {
      ci::gl::translate(position);

      // FIXME:通常のscaleが1.0で、pickableが潰された時のみscaleが変わる
      //       ので、回転の後でscaleを掛けている
      ci::gl::scale(size);

      // TIPS:gl::rotate(Quarf)は、内部でglRotatefを使っている
      //      この計算が正しく求まらない状況があるため、Quarf->Matrix
      //      にしている。これだと問題ない
      glMultMatrixf(rotation.toMatrix44());
    };
    
    drawCubes(field.pickable_cubes, models,
              "pickable_cube", "pickable_cube",
              pickable_matrix);

    auto matrix = [](const ci::Vec3f& position, const ci::Quatf& rotation, const ci::Vec3f& size) {
      ci::gl::translate(position);
      glMultMatrixf(rotation.toMatrix44());
      ci::gl::scale(size);
    };
    
    drawCubes(field.item_cubes, models,
              "item_cube", "item_cube",
              matrix);
    
    drawCubes(field.moving_cubes, models,
              "pickable_cube",
              "moving_cube",
              matrix);
    
    drawCubes(field.falling_cubes, models,
              "pickable_cube", "falling_cube",
              matrix);
    
    drawCubes(field.switches, models,
              "switch", "switch",
              matrix);
    
    drawCubes(field.oneways, models,
              oneway_models_[oneway_index_()], "oneway",
              matrix);
    
    // bgのfogは別設定
    glFogf(GL_FOG_START, bg_fog_start_);
    glFogf(GL_FOG_END, bg_fog_end_);

    drawBgCubes(field.bg_cubes, models);

    lights_.disableLights();
    ci::gl::disable(GL_FOG);

#ifdef DEBUG
    if (debug_info_) {
      drawPickableCubesBBox(field.pickable_cubes);
      drawBgBbox(field.bg_bbox_min, field.bg_bbox_max);
      drawCameraTargetRange();
      drawLightInfo();
    }
#endif
  }


  void cancelPicking(const u_int cube_id) noexcept {
    boost::remove_erase_if(pickings_,
                           [cube_id](const Pick& pick) {
                             return pick.cube_id == cube_id;
                           });
  }

  void calcelAllPickings() noexcept {
    pickings_.clear();
  }

  
  // Touch入力の有効・無効
  void enableTouchInput(const bool input = true) noexcept {
    DOUT << "enableTouchInput:" << input << std::endl;
    if (input != touch_input_) calcelAllPickings();
    touch_input_ = input;
  }

  
  void resetCamera(int offset_z) noexcept {
    new_target_point_ = Json::getVec3<float>(params_["game_view.camera.target_point"]);
    new_target_point_.z += float(offset_z);
  }

  void setStageBgColor(const ci::Color& color) noexcept {
    animation_timeline_->apply(&bg_color_,
                               color,
                               bg_tween_duration_, getEaseFunc(bg_tween_type_));

    ci::ColorA fog_color = ci::ColorA(color.r, color.g, color.b, 1) * fog_color_rate_;
    
    animation_timeline_->apply(&fog_color_,
                               fog_color,
                               bg_tween_duration_, getEaseFunc(bg_tween_type_));
  }

  void setStageLightTween(const std::string& tween_name) noexcept {
    lights_.startLightTween(tween_name);
  }

  void enableFollowCamera(const bool enable = true) noexcept {
    camera_follow_target_ = enable;
  }

  void startQuake(const float duration, const ci::Vec3f& pos, const ci::Vec3f& size) noexcept {
    // 視錐台外は無視
    if (!frustum_.intersects(pos, size)) return;
    
    quake_.start(*animation_timeline_, &quake_value_, duration);
  }

  void startViewSound(const std::string& sound, const ci::Vec3f& pos, const ci::Vec3f& size) noexcept {
    // 視錐台外は無視
    if (!frustum_.intersects(pos, size)) return;

    requestSound(event_, sound);
  }
  


  void setCameraParams(const std::string& name) noexcept {
    auto params = params_["game_view.camera." + name];

    target_point_     = Json::getVec3<float>(params_["game_view.camera.target_point"]);
    new_target_point_ = target_point_;

    interest_point_   = Json::getVec3<float>(params["interest_point"]);

    // 注視点からの距離、角度でcamera位置を決めている
    eye_rx_ = params["eye_rx"].getValue<float>();
    eye_ry_ = params["eye_ry"].getValue<float>();
    eye_distance_ = params["eye_distance"].getValue<float>();

    eye_point_ = calcEyePoint(0.0f);
  }

  void changeCameraParams(const std::string& name) noexcept {
    auto params = params_["game_view.camera." + name];

    auto ease_func     = getEaseFunc(params_["game_view.camera.ease_name"].getValue<std::string>());
    auto ease_duration = params_["game_view.camera.ease_duration"].getValue<float>();

    animation_timeline_->apply(&interest_point_,
                               Json::getVec3<float>(params["interest_point"]),
                               ease_duration, ease_func);

    animation_timeline_->apply(&eye_rx_,
                               params["eye_rx"].getValue<float>(),
                               ease_duration, ease_func);

    animation_timeline_->apply(&eye_ry_,
                               params["eye_ry"].getValue<float>(),
                               ease_duration, ease_func);

    animation_timeline_->apply(&eye_distance_,
                               params["eye_distance"].getValue<float>(),
                               ease_duration, ease_func);
  }


  void beginDistanceCloser() noexcept {
    auto ease_func     = getEaseFunc(params_["game_view.camera.distance_ease_name"].getValue<std::string>());
    auto ease_duration = params_["game_view.camera.distance_ease_duration"].getValue<float>();

    animation_timeline_->apply(&distance_rate_,
                               close_distance_rate_,
                               ease_duration, ease_func);
  }
  
  void endDistanceCloser() noexcept {
    auto ease_func     = getEaseFunc(params_["game_view.camera.distance_ease_name"].getValue<std::string>());
    auto ease_duration = params_["game_view.camera.distance_ease_duration"].getValue<float>();

    animation_timeline_->apply(&distance_rate_,
                               1.0f,
                               ease_duration, ease_func);
  }

  // 指定のPickableCubeに寄る
  void beginPickableCubeCloser(const u_int id) noexcept {
    camera_look_id_  = true;
    looking_cube_id_ = id;
  }
  
  void endPickableCubeCloser() noexcept {
    camera_look_id_ = false;
  }

  
private:
  void touchesBegan(const Connection&, std::vector<Touch>& touches) noexcept {
    if (!touch_input_) return;
    
    for (const auto& touch : touches) {
      auto  ray    = generateRay(touch.pos);
      float near_z = std::numeric_limits<float>::max();
      bool  picked = false;
      
      TouchCube* picked_cube = nullptr;
      
      for (auto& cube : touch_cubes_) {
        if (isTouching(cube.id)) continue;
        
        if (isPickedCube(cube.bbox, ray)) {
          // 複数のCubeをPickする可能性がある
          // その場合は一番手前のを選ぶ
          float z = getPickedCubeZ(cube.bbox, ray);
          if (z > near_z) continue;
          near_z = z;
          picked = true;
          picked_cube = &cube;
        }
      }
      
      if (picked) {
        assert(picked_cube);
          
        pickings_.emplace_back(touch.id,
                               touch.pos, touch.timestamp,
                               picked_cube->id,
                               false);

        EventParam params = {
          { "cube_id", picked_cube->id },
        };
        event_.signal("picking-start", params);
      }
    }
  }
  
  void touchesMoved(const Connection&, std::vector<Touch>& touches) noexcept {
    if (!touch_input_) return;

    for (const auto& touch : touches) {
      if (!isPicking(touch)) continue;

      auto pick = getPick(touch);
      assert(pick);
      if (pick->began_move) continue;

      auto delta_time = touch.timestamp - pick->timestamp;
      if (delta_time < move_begin_time_) continue;

      for (auto& cube : touch_cubes_) {
        if (cube.id == pick->cube_id) {
          // cubeの上平面との交点を求める
          auto origin = ci::Vec3f(0, cube.position.y + 0.5f, 0);
          auto ray = generateRay(touch.pos);
          float cross_z;
          ray.calcPlaneIntersection(origin, ci::Vec3f(0, 1, 0), &cross_z);

          // カメラが動いている事を考慮して、タッチ開始時の座標もここで求めている
          auto began_ray = generateRay(pick->touch_begin_pos);
          float began_z;
          began_ray.calcPlaneIntersection(origin, ci::Vec3f(0, 1, 0), &began_z);
          
          auto began_pos   = began_ray.calcPosition(began_z);
          auto picking_ofs = ray.calcPosition(cross_z) - began_pos;
          
          // カメラの離れ具合で、移動量の閾値を変える
          float move_threshold = reviseValueByCamera(move_begin_threshold_);
          
          int move_direction = PickableCube::MOVE_NONE;
          if (std::abs(picking_ofs.z) >= std::abs(picking_ofs.x)) {
            // 縦移動
            if (picking_ofs.z < -move_threshold) {
              move_direction = PickableCube::MOVE_DOWN;
            }
            else if (picking_ofs.z > move_threshold) {
              move_direction = PickableCube::MOVE_UP;
            }
          }
          else {
            // 横移動
            if (picking_ofs.x < -move_threshold) {
              move_direction = PickableCube::MOVE_RIGHT;
            }
            else if (picking_ofs.x > move_threshold) {
              move_direction = PickableCube::MOVE_LEFT;
            }
          }

          if (move_direction != PickableCube::MOVE_NONE) {
            EventParam params = {
              { "cube_id",        pick->cube_id },
              { "move_direction", move_direction },
              { "move_speed",     1 },
            };
            event_.signal("move-pickable", params);

            pick->began_move = true;
            removePick(touch);

            DOUT << "move:" << move_direction << std::endl;
            DOUT << "     " << (touch.pos - pick->touch_begin_pos).length() << std::endl;
          }
          break;
        }
      }      
    }
  }

  void touchesEnded(const Connection&, std::vector<Touch>& touches) noexcept {
    if (!touch_input_) return;
    
    for (const auto& touch : touches) {
      if (!isPicking(touch)) continue;

      auto pick = getPick(touch);
      assert(pick);
      
      for (auto& cube : touch_cubes_) {
        if (cube.id == pick->cube_id) {
          // cubeの上平面との交点
          auto origin = ci::Vec3f(0, cube.position.y + 0.5f, 0);
          auto ray = generateRay(touch.pos);
          float cross_z;
          ray.calcPlaneIntersection(origin, ci::Vec3f(0, 1, 0), &cross_z);

          // カメラが動いている事を考慮して、タッチ開始時の座標もここで求めている
          auto began_ray = generateRay(pick->touch_begin_pos);
          float began_z;
          began_ray.calcPlaneIntersection(origin, ci::Vec3f(0, 1, 0), &began_z);
          
          auto began_pos   = began_ray.calcPosition(began_z);
          auto picking_ofs = ray.calcPosition(cross_z) - began_pos;

          auto delta_time = touch.timestamp - pick->timestamp;

          // カメラの離れ具合で、移動量の閾値を変える
          float move_threshold = reviseValueByCamera(move_threshold_);
          
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

          DOUT << "move:" << move_direction << " speed:" << move_speed << std::endl;

          removePick(touch);
          break;
        }
      }
    }
  }

  
  bool isPicking(const Touch& touch) noexcept {
    for (const auto& pick : pickings_) {
      if (touch.id == pick.touch_id) return true;
    }
    return false;
  }

  bool isTouching(const u_int cube_id) noexcept {
    for (const auto& pick : pickings_) {
      if (cube_id == pick.cube_id) return true;
    }
    return false;
  }

  boost::optional<Pick&> getPick(const Touch& touch) noexcept {
    for (auto& pick : pickings_) {
      if (touch.id == pick.touch_id) return boost::optional<Pick&>(pick);
    }
    return boost::optional<Pick&>();
  }
  
  boost::optional<Pick&> getTouching(const u_int cube_id) noexcept {
    for (auto& pick : pickings_) {
      if (cube_id == pick.cube_id) return boost::optional<Pick&>(pick);
    }
    return boost::optional<Pick&>();
  }

  
  void removePick(const Touch& touch) noexcept {
    boost::remove_erase_if(pickings_,
                           [touch](const Pick& pick) {
                             return pick.touch_id == touch.id;
                           });
  }

  bool isCubeExists(const u_int id) noexcept {
    for (const auto& cube : touch_cubes_) {
      if (cube.id == id) return true;
    }
    return false;
  }
  

  int calcMoveSpeed(const double delta_time, const float delta_position, const float move_threshold) noexcept {
    if (delta_position < move_threshold) {
      return 1;
    }
    
    int speed = int(move_speed_rate_ * delta_position / delta_time);
    return boost::algorithm::clamp(speed, 1, move_swipe_speed_);
  }

  ci::Ray generateRay(const ci::Vec2f& pos) noexcept {
    float u = pos.x / (float) ci::app::getWindowWidth();
    float v = pos.y / (float) ci::app::getWindowHeight();
    // because OpenGL and Cinder use a coordinate system
    // where (0, 0) is in the LOWERleft corner, we have to flip the v-coordinate
    return camera_.generateRay(u, 1.0f - v, camera_.getAspectRatio());
  }

  bool isPickedCube(ci::AxisAlignedBox3f& bbox, const ci::Ray& ray) noexcept {
    return bbox.intersects(ray);
  }

  float getPickedCubeZ(ci::AxisAlignedBox3f& bbox, const ci::Ray& ray) noexcept {
    float intersections[3];
    bbox.intersect(ray, intersections);

    return std::min(intersections[0], intersections[1]);
  }

  
  void makeTouchCubeInfo(const std::vector<std::unique_ptr<PickableCube> >& cubes) noexcept {
    touch_cubes_.clear();
    
    for (const auto& cube : cubes) {
      if (!cube->isActive() || !cube->isOnStage() || cube->isSleep() || cube->isPressed()) continue;

      const auto& pos = cube->position();
      const auto& size = cube->size();
      ci::Vec3f half_size = size / 2;
      
      if (!cube->isAdjoinOther()) {
        // 隣接がなければpaddingを加える
        // ※カメラの離れ具合で補正
        float padding_size = reviseValueByCamera(cube->getPaddingSize());

        half_size += ci::Vec3f(padding_size, padding_size, padding_size);
      }
      
      touch_cubes_.emplace_back(cube->id(),
                                pos, cube->rotation(),
                                ci::AxisAlignedBox3f(pos - half_size, pos + half_size));
    }

    // Pick中なのに含まれないCubeを削除
    boost::remove_erase_if(pickings_,
                           [this](const Pick& pick) noexcept {
                             return !isCubeExists(pick.cube_id);
                           });
  }

  void updateCameraTarget(const std::vector<std::unique_ptr<PickableCube> >& cubes) noexcept {
    if (!camera_follow_target_) return;
    
    std::vector<ci::Vec3f> cube_pos;
    if (camera_look_id_) {
      // 特定IDのCubeから注視点を決める
      cube_pos = searchCubeFromId(cubes, looking_cube_id_);
    }
    else {
      // 生きているすべてのCubeから注視点を決める
      cube_pos = searchAliveCube(cubes);
    }

    if (cube_pos.empty()) return;
    
    // FIXME:とりあえず中間点
    new_target_point_ = std::accumulate(std::begin(cube_pos), std::end(cube_pos), ci::Vec3f::zero()) / cube_pos.size();
    new_target_radius_ = 0.0f;
    for (const auto& pos : cube_pos) {
      new_target_radius_ = std::max(new_target_point_.distance(pos), new_target_radius_);
    }

    // 中心点から一番離れたpickable cubeへの距離に応じて注視点を移動
    {
      auto d = ci::Vec2f(eye_point_.x, eye_point_.z).normalized() * target_radius_ * eye_offset_rate_;
      new_target_point_.x += d.x;
      new_target_point_.z += d.y;
      if (camera_look_id_) {
        // 落下Cubeはそのまま追いかけないworkaround
        new_target_point_.y = target_point_.y;
      }
    }
  }

  std::vector<ci::Vec3f> searchAliveCube(const std::vector<std::unique_ptr<PickableCube> >& cubes) noexcept {
    std::vector<ci::Vec3f> cube_pos;

    if (cube_pos.capacity() < 4) cube_pos.reserve(4);
    
    for (const auto& cube : cubes) {
      if (!cube->isActive() || !cube->isOnStage() || cube->isSleep()) continue;
      
      cube_pos.push_back(cube->position());
    }
    
    return cube_pos;
  }

  std::vector<ci::Vec3f> searchCubeFromId(const std::vector<std::unique_ptr<PickableCube> >& cubes, const u_int id) noexcept {
    std::vector<ci::Vec3f> cube_pos;

    for (const auto& cube : cubes) {
      if (cube->id() == id) {
        cube_pos.push_back(cube->position());
        break;
      }
    }
    
    return cube_pos;
  }
  
  void updateCamera(const double progressing_seconds) noexcept {
    // 等加速運動の近似
    float speed_rate = std::pow(camera_speed_, progressing_seconds / (1 / 60.0));

    target_point_ = new_target_point_ - (new_target_point_ - target_point_) * speed_rate;
    target_radius_ = new_target_radius_ - (new_target_radius_ - target_radius_) * speed_rate;

    // 中心点から一番離れたpickable cubeへの距離に応じてカメラを引く
    {
      float offset = target_radius_ * eye_distance_rate_;
      eye_point_ = calcEyePoint(offset);
    }

    auto offset = ci::Vec3f(0.0f, quake_value_, 0.0f);
    
    camera_.setCenterOfInterestPoint(target_point_ + offset + interest_point_);
    camera_.setEyePoint(target_point_ + offset + eye_point_);
  }

  
  void drawStageCubes(const std::deque<std::vector<StageCube> >& cubes,
                      ModelHolder& models,
                      const ci::Frustumf& frustum) noexcept {
    auto& material = materials_.get("stage_cube");
    material.apply();
    
    const auto& mesh = models.get("stage_cube").mesh();
    
    for (const auto& row : cubes) {
      for (const auto& cube : row) {
        if (!cube.active) continue;

        if (!frustum.intersects(cube.position, ci::Vec3f::one())) continue;
        
        ci::gl::color(cube.color);

        ci::gl::pushModelView();
        ci::gl::translate(cube.position);
        // TIPS:stagecubeはrotateとscalingが無い

        ci::gl::draw(mesh);
      
        ci::gl::popModelView();
      }
    }
  }


  template<typename T>
  void drawCubes(const std::vector<T>& cubes,
                 ModelHolder& models,
                 const std::string& model_name, const std::string& material_name,
                 std::function<void (const ci::Vec3f& position, const ci::Quatf& rotation, const ci::Vec3f& size)> matrix) noexcept {
    auto& material = materials_.get(material_name);
    material.apply();

    const auto& mesh = models.get(model_name).mesh();

    for (const auto& cube : cubes) {
      if (!cube->isActive()) continue;

      ci::gl::color(cube->color());
      
      ci::gl::pushModelView();

      // TIPS:行列計算は引数で与えられたのを使う
      matrix(cube->position(), cube->rotation(), cube->size());

      ci::gl::draw(mesh);
      
      ci::gl::popModelView();
    }    
  }

  template<typename T>
  void drawCubeShadow(const std::vector<T>& cubes,
                      ModelHolder& models,
                      const std::string& model_name, const std::string& material_name) noexcept {
    ci::gl::disableDepthRead();
    ci::gl::disableDepthWrite();
    ci::gl::disable(GL_LIGHTING);
    ci::gl::enable(GL_BLEND);

    auto& material = materials_.get(material_name);
    material.apply();

    const auto& mesh = models.get(model_name).mesh();
    for (const auto& cube : cubes) {
      if (!cube->isActive()) continue;

      auto alpha = cube->shadowAlpha();
      if (alpha == 0.0f) continue;
      
      ci::gl::color(ci::ColorA(0, 0, 0, shadow_alpha_ * alpha));

      ci::gl::pushModelView();

      // 位置は、stage cubeの上面
      auto position = cube->position();
      ci::gl::translate(ci::Vec3f(position.x, cube->stageHeight(), position.z));

      // 影は、縦方向をぺちゃんこにすればよい
      ci::gl::scale(1.0f, 0.0f, 1.0f);
      
      glMultMatrixf(cube->rotation().toMatrix44());
      ci::gl::scale(cube->size());

      ci::gl::draw(mesh);

      ci::gl::popModelView();
    }
    
    ci::gl::disable(GL_BLEND);
    ci::gl::enableDepthRead();
    ci::gl::enableDepthWrite();
    ci::gl::enable(GL_LIGHTING);
  }

  void drawBgCubes(const std::vector<Bg::Cube>& cubes,
                   ModelHolder& models) noexcept {
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
  void drawPickableCubesBBox(const std::vector<std::unique_ptr<PickableCube> >& cubes) const noexcept {
    ci::gl::color(0, 1, 0);
    
    for (const auto& cube : cubes) {
      if (!cube->isActive()) continue;

      const auto& pos = cube->position();
      const auto& size = cube->size();
      ci::Vec3f half_size = size / 2;

      if (!cube->isAdjoinOther()) {
        // 隣接がなければpaddingを加える
        // ※カメラの離れ具合で補正
        float padding_size = reviseValueByCamera(cube->getPaddingSize());

        half_size += ci::Vec3f(padding_size, padding_size, padding_size);
      }

      ci::AxisAlignedBox3f bbox(pos - half_size,
                                pos + half_size);

      ci::gl::drawStrokedCube(bbox);
    }    
  }

  void drawCameraTargetRange() const noexcept {
    ci::gl::pushModelView();
    ci::gl::translate(new_target_point_);
    ci::gl::rotate(ci::Vec3f(90, 0, 0));
    
    ci::gl::drawStrokedCircle(ci::Vec2f::zero(), target_radius_);
    ci::gl::popModelView();
  }
  
  void drawBgBbox(const ci::Vec3f& bbox_min, const ci::Vec3f& bbox_max) const noexcept {
    ci::AxisAlignedBox3f bbox(bbox_min, bbox_max);
    
    ci::gl::color(0, 1, 0);
    ci::gl::drawStrokedCube(bbox);
  }

  void drawLightInfo() const noexcept {
    ci::gl::color(1, 0, 0);
    
    const auto& lights = lights_.get();
    for (const auto& light : lights) {
      switch (light.type) {
      case ci::gl::Light::POINT:
        ci::gl::drawSphere(light.l.getPosition(), 0.1f);
        break;
      }
    }
  }
#endif

  
  void readMaterials(const ci::JsonTree& params) noexcept {
    for (const auto& p : params) {
      materials_.add(p.getKey(), p);
    }
  }

  
  ci::Vec3f calcEyePoint(const float distance_offset) const noexcept {
    ci::Vec3f pos = ci::Quatf(ci::Vec3f(1, 0, 0), ci::toRadians(eye_rx_))
                  * ci::Quatf(ci::Vec3f(0, 1, 0), ci::toRadians(eye_ry_))
                  * ci::Vec3f(0, 0, eye_distance_ + distance_offset) * distance_rate_() + interest_point_;

    return pos;
  }

  
  void setupOneway(const ci::JsonTree& params) noexcept {
    auto option = animation_timeline_->apply(&oneway_index_,
                                             0, int(oneway_models_.size()),
                                             params["game_view.oneway.easing.duration"].getValue<float>(),
                                             getEaseFunc(params["game_view.oneway.easing.name"].getValue<std::string>()));

    option.loop(true);
  }


  // カメラの距離に応じた補正
  float reviseValueByCamera(const float value) const noexcept {
    float distance = eye_distance_() + target_radius_ * eye_distance_rate_;
    return value * distance * distance_revise_ / eye_distance_();
  }

  // 画面の縦横サイズから、BGの描画サイズを決める
  // 画面の長い方のサイズにあわせ、短い方は切り取って表示する
  void setupBg(const float aspect) noexcept {
    if (aspect > 1.0f) {
      int height = 256.0f / aspect;

      bg_area_ = ci::Area(0, (256 - height) / 2, 256,
                          256 - (256 - height) / 2);
    }
    else {
      int width = 256.0f * aspect;

      bg_area_ = ci::Area((256 - width) / 2, 0,
                          256 - (256 - width) / 2, 256);        
    }
  }


  static void setQuality(const bool low_device) noexcept {
    GLenum target[] = {
      GL_FOG_HINT,
      GL_GENERATE_MIPMAP,
      GL_LINE_SMOOTH_HINT,
      GL_PERSPECTIVE_CORRECTION_HINT,
      GL_POINT_SMOOTH_HINT,
    };

    GLenum mode = low_device ? GL_FASTEST : GL_NICEST;
    
    for (const auto t : target) {
      glHint(t, mode);
    }
  }

};

}
