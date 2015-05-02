﻿#pragma once

//
// 適当に動いているおじゃまCube
//

#include <boost/noncopyable.hpp>
#include "cinder/Timeline.h"
#include "cinder/Rand.h"
#include "EasingUtil.hpp"
#include "Utility.hpp"

namespace ngs {

class MovingCube : private boost::noncopyable {
  enum {
    MOVE_NONE = -1,

    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,

    MOVE_MAX
  };


  ci::JsonTree& params_;
  Event<EventParam>& event_;
  
  bool active_;

  u_int id_;
  
  float cube_size_;

  ci::Color color_;

  ci::Vec3i block_position_;
  ci::Vec3i prev_block_position_;
  
  ci::Anim<ci::Vec3f> position_;
  ci::Anim<ci::Quatf> rotation_;

  ci::TimelineRef animation_timeline_;

  // on_stage_  stage上に存在
  // moving_    移動中
  bool on_stage_;
  bool moving_;
  
  int       move_direction_;
  ci::Vec3i move_vector_;
  int       move_speed_;
  
  std::string rotate_ease_;
  float rotate_duration_;
  std::vector<float> rotate_speed_rate_;
  ci::Anim<ci::Quatf> move_rotation_;
  ci::Quatf move_start_rotation_;

  std::string fall_ease_;
  float fall_duration_;
  float fall_y_;


public:
  MovingCube(ci::JsonTree& params,
             ci::TimelineRef timeline,
             Event<EventParam>& event,
             const ci::Vec3i& entry_pos) :
    params_(params),
    event_(event),
    active_(true),
    id_(getUniqueNumber()),
    cube_size_(params["game.cube_size"].getValue<float>()),
    color_(Json::getColor<float>(params["game.moving.color"])),
    block_position_(entry_pos),
    prev_block_position_(block_position_),
    rotation_(ci::Quatf::identity()),
    animation_timeline_(ci::Timeline::create()),
    on_stage_(false),
    moving_(false),
    move_direction_(MOVE_NONE),
    move_vector_(ci::Vec3i::zero()),
    move_speed_(0),
    rotate_ease_(params["game.moving.rotate_ease"].getValue<std::string>()),
    rotate_duration_(params["game.moving.rotate_duration"].getValue<float>()),
    move_start_rotation_(rotation_()),
    fall_ease_(params["game.moving.fall_ease"].getValue<std::string>()),
    fall_duration_(params["game.moving.fall_duration"].getValue<float>()),
    fall_y_(params["game.moving.fall_y"].getValue<float>())
  {
    DOUT << "MovingCube" << std::endl;

    auto current_time = timeline->getCurrentTime();
    animation_timeline_->setStartTime(current_time);
    timeline->apply(animation_timeline_);

    position_ = ci::Vec3f(block_position_) * cube_size_;
    // block_positionが同じ高さなら、StageCubeの上に乗るように位置を調整
    position_().y += cube_size_;

    for (const auto& param : params["game.moving.rotate_speed_rate"]) {
      rotate_speed_rate_.push_back(param.getValue<float>());
    }

    // 登場演出
    auto entry_y = Json::getVec2<float>(params["game.moving.entry_y"]);
    float y = ci::randFloat(entry_y.x, entry_y.y) * cube_size_;
    ci::Vec3f start_value(position() + ci::Vec3f(0, y, 0));
    auto options = animation_timeline_->apply(&position_,
                                              start_value, position_(),
                                              params["game.moving.entry_duration"].getValue<float>(),
                                              getEaseFunc(params["game.moving.entry_ease"].getValue<std::string>()));

    options.finishFn([this]() {
        on_stage_ = true;
        
        EventParam params = {
          { "id", id_ },
          { "block_pos", block_position_ },
        };
        event_.signal("moving-on-stage", params);
      });
  }
    
  ~MovingCube() {
    // 再生途中のものもあるので、手動で取り除く
    animation_timeline_->removeSelf();
  }

  
  bool willRotationMove() const {
    return (move_speed_ > 0) && !moving_ && on_stage_;
  }

  void cancelRotationMove() {
    move_speed_ = 0;
  }

  const ci::Vec3i& moveVector() const { return move_vector_; }

  void startRotationMove() {
    moving_ = true;
    
    prev_block_position_ = block_position_;
    block_position_ += move_vector_;

    auto angle = ci::toRadians(90.0f);
    ci::Quatf rotation_table[] = {
      { ci::Vec3f(1, 0, 0),  angle },
      { ci::Vec3f(1, 0, 0), -angle },
      { ci::Vec3f(0, 0, 1), -angle },
      { ci::Vec3f(0, 0, 1),  angle },
    };

    float duration = rotate_duration_* rotate_speed_rate_[std::min(move_speed_, int(rotate_speed_rate_.size())) - 1];
    
    auto options = animation_timeline_->apply(&move_rotation_,
                                              ci::Quatf::identity(), rotation_table[move_direction_],
                                              duration,
                                              getEaseFunc(rotate_ease_));
    ci::Vec3f pivot_table[] = {
      ci::Vec3f(              0, -cube_size_ / 2,  cube_size_ / 2),
      ci::Vec3f(              0, -cube_size_ / 2, -cube_size_ / 2),
      ci::Vec3f( cube_size_ / 2, -cube_size_ / 2,               0),
      ci::Vec3f(-cube_size_ / 2, -cube_size_ / 2,               0)
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
        // FIXME:回転も正規化
        position_ = ci::Vec3f(block_position_) * cube_size_;
        position_().y += cube_size_;
        move_start_rotation_ = rotation_;

        moving_ = false;
        move_speed_ -= 1;
      });
  }

  void fallFromStage() {
    on_stage_ = false;

    // idle中の動作を中断
    move_rotation_.stop();
    
    ci::Vec3f end_value(position_() + ci::Vec3f(0, fall_y_ * cube_size_, 0));
    auto options = animation_timeline_->apply(&position_,
                                              end_value,
                                              fall_duration_,
                                              getEaseFunc(fall_ease_));

    options.finishFn([this]() {
        active_ = false;
      });
  }

  u_int id() const { return id_; }
  
  bool isActive() const { return active_; }
  bool isOnStage() const { return on_stage_; }
  bool isMoving() const { return moving_; }

  int moveSpeed() const { return move_speed_; }
  
  const ci::Vec3f& position() const { return position_(); }
  const ci::Quatf& rotation() const { return rotation_(); }

  const ci::Vec3i& blockPosition() const { return block_position_; }
  const ci::Vec3i& prevBlockPosition() const { return prev_block_position_; }
  
  float cubeSize() const { return cube_size_; }
  ci::Vec3f size() const { return ci::Vec3f(cube_size_, cube_size_, cube_size_); }

  const ci::Color& color() const { return color_; }
  
};

}
