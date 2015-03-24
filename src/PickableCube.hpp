#pragma once

//
// 操作可能なCube
//

#include "cinder/Timeline.h"
#include "cinder/Rand.h"
#include "EasingUtil.hpp"
#include "Utility.hpp"


namespace ngs {

class PickableCube {
  bool active_;

  u_int id_;
  
  float cube_size_;
  ci::Color color_;

  ci::Vec3i block_position_;
  
  ci::Anim<ci::Vec3f> position_;
  ci::Anim<ci::Quatf> rotation_;

  ci::TimelineRef animation_timeline_;

  bool can_pick_;
  bool picking_;


public:
  PickableCube(const ci::JsonTree& params, const ci::Vec3i& entry_pos) :
    active_(true),
    id_(getUniqueNumber()),
    cube_size_(params["game.cube_size"].getValue<float>()),
    color_(Json::getColor<float>(params["game.pickable.color"])),
    block_position_(entry_pos),
    rotation_(ci::Quatf::identity()),
    animation_timeline_(ci::Timeline::create()),
    can_pick_(false),
    picking_(false)
  {
    position_ = ci::Vec3f(block_position_) * cube_size_;

    auto current_time = ci::app::timeline().getCurrentTime();
    animation_timeline_->setStartTime(current_time);
    ci::app::timeline().apply(animation_timeline_);

    // 登場演出
    auto entry_y = Json::getVec2<float>(params["game.pickable.entry_y"]);
    float y = ci::randFloat(entry_y.x, entry_y.y) * cube_size_;
    ci::Vec3f start_value(position() + ci::Vec3f(0, y, 0));
    auto options = animation_timeline_->apply(&position_,
                                              start_value, position_(),
                                              params["game.pickable.entry_duration"].getValue<float>(),
                                              getEaseFunc(params["game.pickable.entry_ease"].getValue<std::string>()));

    options.finishFn([this]() mutable {
        can_pick_ = true;
      });
  }

  ~PickableCube() {
    // 再生途中のものもあるので、手動で取り除く
    animation_timeline_->removeSelf();
  }


  u_int id() const { return id_; }
  
  bool isActive() const { return active_; }
  bool canPick() const { return can_pick_; }

  const ci::Vec3f& position() const { return position_(); }
  const ci::Quatf& rotation() const { return rotation_(); }
  float cubeSize() const { return cube_size_; }
  ci::Vec3f size() const { return ci::Vec3f(cube_size_, cube_size_, cube_size_); }
  const ci::Color& color() const { return color_; }


private:
  // TIPS:コピー不可
  PickableCube(const PickableCube&) = delete;
  PickableCube& operator=(const PickableCube&) = delete;
  
};

}
