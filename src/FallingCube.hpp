#pragma once

//
// ドッスン
//

#include <boost/noncopyable.hpp>
#include "cinder/Timeline.h"
#include "cinder/Rand.h"
#include "EasingUtil.hpp"
#include "Utility.hpp"


namespace ngs {

class FallingCube : private boost::noncopyable {
  ci::JsonTree& params_;
  Event<EventParam>& event_;
  
  bool active_;

  u_int id_;
  
  ci::Color color_;

  ci::Vec3i block_position_;
  
  ci::Anim<ci::Vec3f> position_;
  ci::Quatf rotation_;

  ci::TimelineRef animation_timeline_;

  bool on_stage_;

  enum Status {
    IDLE,
    UP,
    DOWN,
  };
  Status status_;
  
  std::string fall_ease_;
  float fall_duration_;
  float fall_y_;

  float interval_;

  std::string up_ease_;
  float up_duration_;
  float up_y_;
  
  std::string down_ease_;
  float down_duration_;

  float quake_duration_;
  
  
public:
  FallingCube(ci::JsonTree& params,
              ci::TimelineRef timeline,
              Event<EventParam>& event,
              const ci::Vec3i& entry_pos,
              const float interval, const float delay) noexcept :
    params_(params),
    event_(event),
    active_(true),
    id_(getUniqueNumber()),
    color_(Json::getColor<float>(params["game.falling.color"])),
    block_position_(entry_pos),
    rotation_(ci::Quatf::identity()),
    animation_timeline_(ci::Timeline::create()),
    on_stage_(false),
    status_(Status::IDLE),
    fall_ease_(params["game.falling.fall_ease"].getValue<std::string>()),
    fall_duration_(params["game.falling.fall_duration"].getValue<float>()),
    fall_y_(params["game.falling.fall_y"].getValue<float>()),
    interval_(interval),
    up_ease_(params["game.falling.up_ease"].getValue<std::string>()),
    up_duration_(params["game.falling.up_duration"].getValue<float>()),
    up_y_(params["game.falling.up_y"].getValue<float>()),
    down_ease_(params["game.falling.down_ease"].getValue<std::string>()),
    down_duration_(params["game.falling.down_duration"].getValue<float>()),
    quake_duration_(params["game.falling.quake_duration"].getValue<float>())
  {
    DOUT << "FallingCube()" << std::endl;

    auto current_time = timeline->getCurrentTime();
    animation_timeline_->setStartTime(current_time);
    timeline->apply(animation_timeline_);

    position_ = ci::Vec3f(block_position_);
    // block_positionが同じ高さなら、StageCubeの上に乗るように位置を調整
    position_().y += 1.0f;

    // 登場演出
    auto entry_y = Json::getVec2<float>(params["game.falling.entry_y"]);
    float y = ci::randFloat(entry_y.x, entry_y.y);
    ci::Vec3f start_value(position() + ci::Vec3f(0, y, 0));
    auto options = animation_timeline_->apply(&position_,
                                              start_value, position_(),
                                              params["game.falling.entry_duration"].getValue<float>(),
                                              getEaseFunc(params["game.falling.entry_ease"].getValue<std::string>()));

    options.finishFn([this, delay]() noexcept {
        on_stage_ = true;
        
        EventParam params = {
          { "id", id_ },
          { "block_pos", block_position_ },
        };
        event_.signal("falling-on-stage", params);

        startUpEase(delay);
      });
  }
    
  ~FallingCube() {
    DOUT << "~FallingCube()" << std::endl;

    // 再生途中のものもあるので、手動で取り除く
    animation_timeline_->removeSelf();
  }


  void update(const double progressing_seconds) noexcept {
  }
  

  void fallFromStage() noexcept {
    on_stage_ = false;

    ci::Vec3f end_value(block_position_ + ci::Vec3f(0, fall_y_, 0));
    auto options = animation_timeline_->apply(&position_,
                                              end_value,
                                              fall_duration_,
                                              getEaseFunc(fall_ease_));

    options.finishFn([this]() noexcept {
        active_ = false;
      });
  }

  u_int id() const noexcept { return id_; }
  
  bool isActive() const noexcept { return active_; }
  bool isOnStage() const noexcept { return on_stage_; }

  const ci::Vec3f& position() const noexcept { return position_(); }
  const ci::Quatf& rotation() const noexcept { return rotation_; }

  const ci::Vec3i& blockPosition() const noexcept { return block_position_; }
  
  ci::Vec3f size() const noexcept { return ci::Vec3f::one(); }

  const ci::Color& color() const noexcept { return color_; }

  // Pickableを通せんぼする状態か??
  bool canBlock() const noexcept {
    if (status_ == Status::IDLE) return true;
    
    float y = position_().y - (block_position_.y + 1.5f);
    return y < 1.0f;
  }

  // Pickableを踏める状態か??
  bool canPress() const noexcept {
    float y = position_().y - (block_position_.y + 1.5f);
    return (status_ == Status::DOWN) && (y < 1.0f);
  }
  

  // std::findを利用するための定義
  bool operator==(const u_int rhs_id) const noexcept {
    return id_ == rhs_id;
  }

  bool operator==(const FallingCube& rhs) const noexcept {
    return id_ == rhs.id_;
  }


private:
  void startUpEase(const float delay = 0.0f) noexcept {
    status_ = Status::UP;

    auto up_pos = ci::Vec3f(block_position_);
    up_pos.y += up_y_;
    
    auto options = animation_timeline_->apply(&position_,
                                              up_pos,
                                              up_duration_,
                                              getEaseFunc(up_ease_));

    options.delay(interval_ + delay);
    options.finishFn([this]() noexcept {
        startDownEase();
      });
  }

  void startDownEase() noexcept {
    status_ = Status::DOWN;

    auto down_pos = ci::Vec3f(block_position_);
    // block_positionが同じ高さなら、StageCubeの上に乗るように位置を調整
    down_pos.y += 1.0f;
      
    auto options = animation_timeline_->apply(&position_,
                                              down_pos,
                                              down_duration_,
                                              getEaseFunc(down_ease_));
    options.delay(interval_);
    options.finishFn([this]() noexcept {
        EventParam params = {
          { "duration", quake_duration_ },
          { "pos",      position() },
          { "size",     size() },
          { "sound",    std::string("falling") },
        };
        
        event_.signal("falling-down", params);
        event_.signal("view-sound", params);
        startUpEase();
      });
  }
  
};

}
