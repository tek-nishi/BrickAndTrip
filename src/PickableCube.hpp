#pragma once

//
// 操作可能なCube
//

#include <boost/noncopyable.hpp>
#include "cinder/Timeline.h"
#include "cinder/Rand.h"
#include "EasingUtil.hpp"
#include "Utility.hpp"


namespace ngs {

class PickableCube : private boost::noncopyable {
public:
  enum {
    MOVE_NONE = -1,

    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,

    MOVE_MAX
  };

  
private:
  ci::JsonTree& params_;
  Event<EventParam>& event_;
  
  bool active_;

  u_int id_;
  
  ci::Color orig_color_;
  ci::Anim<ci::Color> color_;

  ci::Vec3i block_position_;
  ci::Vec3i prev_block_position_;
  
  ci::Anim<ci::Vec3f> position_;
  ci::Anim<ci::Quatf> rotation_;

  // ドッスンに潰された時用
  ci::Vec3f scale_;
  // 操作時の"のりしろ"
  float padding_size_;
  
  ci::TimelineRef animation_timeline_;

  // on_stage_  stage上に存在
  // moving_    移動中
  // sleep_     操作不可
  bool on_stage_;
  bool moving_;
  bool sleep_;
  
  bool first_moved_;
  bool move_event_;

  int       move_direction_;
  ci::Vec3i move_vector_;
  int       move_speed_;
  
  std::string rotate_ease_;
  std::string rotate_ease_end_;
  float rotate_duration_;
  float rotate_power_;
  ci::Vec2f rotate_remap_;
  int rotate_speed_max_;
  
  ci::Anim<ci::Quatf> move_rotation_;
  ci::Quatf move_start_rotation_;
  ci::Quatf move_end_rotation_;

  std::string fall_ease_;
  float fall_duration_;
  float fall_y_;

  std::string idle_ease_;
  float idle_duration_;
  float idle_angle_;
  ci::Vec2f idle_delay_;

  ci::Color picking_color_;

  // FIXME:paramから直接読んでもいいんじゃね??
  std::string picking_start_ease_;
  float       picking_start_duration_;
  std::string picking_end_ease_;
  float       picking_end_duration_;

  std::string pressed_ease_;
  float       pressed_ease_duration_;
  float       pressed_ease_scale_;

  // 他のPickableと隣接している
  bool adjoin_other_;
  
  bool pressed_;
  ci::Anim<float> pressed_scale_;
  

public:
  PickableCube(ci::JsonTree& params,
               ci::TimelineRef timeline,
               Event<EventParam>& event,
               const ci::Vec3i& entry_pos, const bool sleep = false) :
    params_(params),
    event_(event),
    active_(true),
    id_(getUniqueNumber()),
    orig_color_(Json::getColor<float>(params["game.pickable.color"])),
    color_(orig_color_),
    block_position_(entry_pos),
    prev_block_position_(block_position_),
    rotation_(ci::Quatf::identity()),
    scale_(1, 1, 1),
    padding_size_(params["game.pickable.padding_size"].getValue<float>()),
    animation_timeline_(ci::Timeline::create()),
    on_stage_(false),
    moving_(false),
    sleep_(sleep),
    first_moved_(false),
    move_event_(true),
    move_direction_(MOVE_NONE),
    move_vector_(ci::Vec3i::zero()),
    move_speed_(0),
    rotate_ease_(params["game.pickable.rotate_ease"].getValue<std::string>()),
    rotate_ease_end_(params["game.pickable.rotate_ease_end"].getValue<std::string>()),
    rotate_duration_(params["game.pickable.rotate_duration"].getValue<float>()),
    rotate_power_(params["game.pickable.rotate_power"].getValue<float>()),
    rotate_remap_(Json::getVec2<float>(params["game.pickable.rotate_remap"])),
    rotate_speed_max_(params["game.pickable.rotate_speed_max"].getValue<int>()),
    move_start_rotation_(rotation_()),
    move_end_rotation_(rotation_()),
    fall_ease_(params["game.pickable.fall_ease"].getValue<std::string>()),
    fall_duration_(params["game.pickable.fall_duration"].getValue<float>()),
    fall_y_(params["game.pickable.fall_y"].getValue<float>()),
    idle_ease_(params["game.pickable.idle_ease"].getValue<std::string>()),
    idle_duration_(params["game.pickable.idle_duration"].getValue<float>()),
    idle_angle_(ci::toRadians(params["game.pickable.idle_angle"].getValue<float>())),
    idle_delay_(Json::getVec2<float>(params["game.pickable.idle_delay"])),
    picking_color_(Json::getColor<float>(params["game.pickable.picking_color"])),
    picking_start_ease_(params["game.pickable.picking_start.ease"].getValue<std::string>()),
    picking_start_duration_(params["game.pickable.picking_start.duration"].getValue<float>()),
    picking_end_ease_(params["game.pickable.picking_end.ease"].getValue<std::string>()),
    picking_end_duration_(params["game.pickable.picking_end.duration"].getValue<float>()),
    pressed_ease_(params["game.pickable.pressed_ease"].getValue<std::string>()),
    pressed_ease_duration_(params["game.pickable.pressed_duration"].getValue<float>()),
    pressed_ease_scale_(params["game.pickable.pressed_scale"].getValue<float>()),
    adjoin_other_(false),
    pressed_(false),
    pressed_scale_(1)
  {
    DOUT << "PickableCube " << sleep << std::endl;
    
    auto current_time = timeline->getCurrentTime();
    animation_timeline_->setStartTime(current_time);
    timeline->apply(animation_timeline_);

    position_ = ci::Vec3f(block_position_);
    // block_positionが同じ高さなら、StageCubeの上に乗るように位置を調整
    position_().y += 1.0f;

    // 登場演出
    auto entry_y = Json::getVec2<float>(params["game.pickable.entry_y"]);
    float y = ci::randFloat(entry_y.x, entry_y.y);
    ci::Vec3f start_value = position() + ci::Vec3f(0, y, 0);
    auto options = animation_timeline_->apply(&position_,
                                              start_value, position_(),
                                              params["game.pickable.entry_duration"].getValue<float>(),
                                              getEaseFunc(params["game.pickable.entry_ease"].getValue<std::string>()));

    options.finishFn([this]() {
        on_stage_ = true;
        
        EventParam params = {
          { "id", id_ },
          { "block_pos", block_position_ },
        };
        event_.signal("pickable-on-stage", params);

        // 時間差でidle動作
        animation_timeline_->add([this]() {
            if (isOnStage() && !first_moved_) {
              EventParam params = {
                { "id", id_ },
                { "block_pos", block_position_ },
              };
              event_.signal("pickable-start-idle", params);
            }
          },
          animation_timeline_->getCurrentTime() + ci::randFloat(idle_delay_.x, idle_delay_.y));
      });

    // sleep開始演出
    if (sleep) startSleepingColor();
  }

  ~PickableCube() {
    // 再生途中のものもあるので、手動で取り除く
    animation_timeline_->removeSelf();
  }
  

  void reserveRotationMove(const int direction, const ci::Vec3i& vector, const int speed) {
    move_vector_ = vector;

    if (move_direction_ == direction) {
      // 同じ方向の時だけは加算
      move_speed_ = std::min(speed + move_speed_, rotate_speed_max_);
    }
    else {
      move_direction_ = direction;
      move_speed_     = std::min(speed, rotate_speed_max_);
    }
  }
  
  bool willRotationMove() const {
    return (move_speed_ > 0) && !moving_ && on_stage_ && !sleep_;
  }

  void cancelRotationMove() {
    move_speed_ = 0;
  }

  const ci::Vec3i& moveVector() const { return move_vector_; }
  
  void enableMovedEvent(const bool enable = true) {
    move_event_ = enable;
  }
  
  void startRotationMove() {
    moving_ = true;
    first_moved_ = true;
    
    // idle演出中に操作されてもよいように
    position_ = ci::Vec3f(block_position_);
    position_().y += 1.0f;
    rotation_ = move_start_rotation_;

    prev_block_position_ = block_position_;
    block_position_ += move_vector_;

    float speed = std::pow(rotate_power_, float(move_speed_ - 1));
    float speed_rate = remap(speed, ci::Vec2f(0.0f, 1.0f), rotate_remap_);

    DOUT << "speed_rate:" << speed_rate << std::endl;
    
    float duration = rotate_duration_ * speed_rate;
    
    move_speed_ -= 1;

    auto angle = ci::toRadians(90.0f);
    ci::Quatf rotation_table[] = {
      { ci::Vec3f(1, 0, 0),  angle },
      { ci::Vec3f(1, 0, 0), -angle },
      { ci::Vec3f(0, 0, 1), -angle },
      { ci::Vec3f(0, 0, 1),  angle },
    };

    auto move_roation = rotation_table[move_direction_];
    // easeで回転し続けると誤差が蓄積されるので
    // 正規化した回転後の向きをあらかじめ計算しておく
    move_end_rotation_ = (move_start_rotation_ * move_roation).normalized();

    const auto& ease = move_speed_ ? rotate_ease_ : rotate_ease_end_;
    
    auto options = animation_timeline_->apply(&move_rotation_,
                                              ci::Quatf::identity(), move_roation,
                                              duration,
                                              getEaseFunc(ease));
    ci::Vec3f pivot_table[] = {
      ci::Vec3f(        0, -1.0f / 2,  1.0f / 2),
      ci::Vec3f(        0, -1.0f / 2, -1.0f / 2),
      ci::Vec3f( 1.0f / 2, -1.0f / 2,         0),
      ci::Vec3f(-1.0f / 2, -1.0f / 2,         0)
    };
    
    auto pivot_rotation = pivot_table[move_direction_];
    auto rotation = rotation_();
    auto position = position_();
    options.updateFn([this, pivot_rotation, rotation, position]() {
        rotation_ = rotation * move_rotation_();

        // 立方体がエッジの部分で回転するよう平行移動を追加
        ci::Matrix44f mat = move_rotation_().toMatrix44();
        auto pivot_pos = mat.transformVec(pivot_rotation);
        position_ = position - pivot_pos + pivot_rotation;
      });
    
    options.finishFn([this]() {
        // 移動後に正確な位置を設定
        position_ = ci::Vec3f(block_position_);
        position_().y += 1.0f;
        rotation_ = move_end_rotation_;
        move_start_rotation_ = move_end_rotation_;

        moving_ = false;
        // move_speed_ -= 1;

        if (!move_event_) return;
        
        EventParam params = {
          { "id", id_ },
          { "block_pos", block_position_ },
        };
        event_.signal("pickable-moved", params);
      });
  }

  void startIdleMotion(const std::vector<int>& directions) {
    ci::Quatf rotation_table[] = {
      { ci::Vec3f(1, 0, 0),  idle_angle_ },
      { ci::Vec3f(1, 0, 0), -idle_angle_ },
      { ci::Vec3f(0, 0, 1), -idle_angle_ },
      { ci::Vec3f(0, 0, 1),  idle_angle_ },
    };

    int move_direction = directions[ci::randInt(int(directions.size()))];
    auto options = animation_timeline_->apply(&move_rotation_,
                                              ci::Quatf::identity(), rotation_table[move_direction],
                                              idle_duration_,
                                              getEaseFunc(idle_ease_));

    // options.delay(ci::randFloat(idle_delay_.x, idle_delay_.y));

    ci::Vec3f pivot_table[] = {
      ci::Vec3f(        0, -1.0f / 2,  1.0f / 2),
      ci::Vec3f(        0, -1.0f / 2, -1.0f / 2),
      ci::Vec3f( 1.0f / 2, -1.0f / 2,         0),
      ci::Vec3f(-1.0f / 2, -1.0f / 2,         0)
    };
    
    auto pivot_rotation = pivot_table[move_direction];
    auto rotation = rotation_();
    auto position = position_();
    options.updateFn([this, pivot_rotation, rotation, position]() {
        rotation_ = rotation * move_rotation_();

        // 立方体がエッジの部分で回転するよう平行移動を追加
        ci::Matrix44f mat = move_rotation_().toMatrix44();
        auto pivot_pos = mat.transformVec(pivot_rotation);
        position_ = position - pivot_pos + pivot_rotation;
      });

    options.finishFn([this]() {
        // 移動後に正確な位置を設定
        // FIXME:回転も正規化
        position_ = ci::Vec3f(block_position_);
        position_().y += 1.0f;

        // 時間差でidle動作
        animation_timeline_->add([this]() {
            if (isOnStage() && !first_moved_) {
              EventParam params = {
                { "id", id_ },
                { "block_pos", block_position_ },
              };
              event_.signal("pickable-start-idle", params);
            }
          },
          animation_timeline_->getCurrentTime() + ci::randFloat(idle_delay_.x, idle_delay_.y));
      });    
  }

  void fallFromStage() {
    on_stage_ = false;

    // idle中の動作を中断
    move_rotation_.stop();

    const auto& pos = position_();
    ci::Vec3f end_value(pos.x, block_position_.y + fall_y_, pos.z);
    auto options = animation_timeline_->apply(&position_,
                                              end_value,
                                              fall_duration_,
                                              getEaseFunc(fall_ease_));

    options.finishFn([this]() {
        active_ = false;
      });
  }

  // ドッスンに踏まれた
  void pressed() {
    // on_stage_ = false;
    pressed_  = true;
    cancelRotationMove();

    // 回転中の動作を取り消す
    move_rotation_.stop();
    position_ = ci::Vec3f(block_position_);
    position_().y += 1.0f;
    // 表示位置が-0.5ずれるので、落下位置もずらしておく
    fall_y_ -= 0.5f;

    rotation_ = move_end_rotation_;
    moving_ = false;

    auto options = animation_timeline_->apply(&pressed_scale_,
                                              pressed_ease_scale_,
                                              pressed_ease_duration_,
                                              getEaseFunc(pressed_ease_));

    const auto& position = position_();
    options.updateFn([this, position]() {
        scale_.y  = pressed_scale_();
        // 潰されたぶん、位置をずらす
        position_().y = position.y - (1.0f - scale_.y) * (1.0f / 2);
      });
  }

  // 昇天
  void rise() {
    on_stage_ = false;
    
    auto      ease_type = params_["game.pickable.rise_ease"].getValue<std::string>();
    float     duration  = params_["game.pickable.rise_duration"].getValue<float>();
    ci::Vec2f height    = Json::getVec2<float>(params_["game.pickable.rise_height"]);

    ci::Vec3f end_value = position() + ci::Vec3f(0.0, ci::randFloat(height.x, height.y), 0.0);
    
    auto options = animation_timeline_->apply(&position_,
                                              end_value,
                                              duration,
                                              getEaseFunc(ease_type));

    options.finishFn([this] {
        active_ = false;
      });
  }

  void startPickingColor() {
    animation_timeline_->apply(&color_,
                               picking_color_,
                               picking_start_duration_,
                               getEaseFunc(picking_start_ease_));
  }

  void endPickingColor() {
    animation_timeline_->apply(&color_,
                               orig_color_,
                               picking_end_duration_,
                               getEaseFunc(picking_end_ease_));
  }

  void startSleepingColor() {
    animation_timeline_->apply(&color_,
                               Json::getColor<float>(params_["game.pickable.sleeping_color"]),
                               params_["game.pickable.sleeping_start.duration"].getValue<float>(),
                               getEaseFunc(params_["game.pickable.sleeping_start.ease"].getValue<std::string>()));
  }
  
  void endSleepingColor() {
    animation_timeline_->apply(&color_,
                               orig_color_,
                               params_["game.pickable.sleeping_end.duration"].getValue<float>(),
                               getEaseFunc(params_["game.pickable.sleeping_end.ease"].getValue<std::string>()));
  }

  
  
  u_int id() const { return id_; }
  
  bool isActive() const { return active_; }
  bool isOnStage() const { return on_stage_; }
  bool isMoving() const { return moving_; }
  bool isPressed() const { return pressed_; }

  bool isSleep() const { return sleep_; }
  void awaken(const bool sleeing = false) { sleep_ = sleeing; }
  
  int moveSpeed() const { return move_speed_; }
  
  const ci::Vec3f& position() const { return position_(); }
  const ci::Quatf& rotation() const { return rotation_(); }

  const ci::Vec3i& blockPosition() const { return block_position_; }
  const ci::Vec3i& prevBlockPosition() const { return prev_block_position_; }
  
  float cubeSize() const { return 1.0f; }
  const ci::Vec3f& size() const { return scale_; }

  float getPaddingSize() const { return padding_size_; }

  const ci::Color& color() const { return color_(); }

  bool isAdjoinOther() const { return adjoin_other_; }
  void setAdjoinOther(const bool adjoin_other) { adjoin_other_ = adjoin_other; }

  
  // std::findを利用するための定義
  bool operator==(const u_int rhs_id) const {
    return id_ == rhs_id;
  }

  bool operator==(const PickableCube& rhs) const {
    return id_ == rhs.id_;
  }
  
  
private:
  
};

}
