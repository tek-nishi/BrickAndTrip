#pragma once

//
// stage上のスイッチ
// PickableCubeが踏むと指定ブロックの高さが書き換えられる
//


namespace ngs {

class Switch : private boost::noncopyable {
  ci::JsonTree& params_;
  Event<EventParam>& event_;

  bool alive_;
  bool active_;
  
  ci::Vec3i block_position_;
  std::vector<ci::Vec3i> targets_;

  ci::Anim<ci::Vec3f> position_;
  ci::Color color_;

  ci::Quatf rotation_;
  float rotate_speed_;

  bool on_stage_;
  bool started_;

  std::string fall_ease_;
  float fall_duration_;
  float fall_y_;

  ci::TimelineRef animation_timeline_;


public:
  Switch(ci::JsonTree& params,
         const ci::JsonTree& entry_params,
         ci::TimelineRef timeline,
         Event<EventParam>& event,
         const int offset_x, const int bottom_z) :
    params_(params),
    event_(event),
    alive_(true),
    active_(false),
    color_(Json::getColor<float>(params["game.switch.color"])),
    rotation_(ci::Quatf::identity()),
    rotate_speed_(params["game.switch.rotate_speed"].getValue<float>()),
    on_stage_(false),
    started_(false),
    fall_ease_(params["game.switch.fall_ease"].getValue<std::string>()),
    fall_duration_(params["game.switch.fall_duration"].getValue<float>()),
    fall_y_(params["game.switch.fall_y"].getValue<float>()),
    animation_timeline_(ci::Timeline::create())
  {
    DOUT << "Switch()" << std::endl;
    
    auto current_time = timeline->getCurrentTime();
    animation_timeline_->setStartTime(current_time);
    timeline->apply(animation_timeline_);
    
    ci::Vec3i offset(offset_x, 0, bottom_z);
    block_position_ = Json::getVec3<int>(entry_params["position"]) + offset;
    
    for (const auto& target : entry_params["target"]) {
      targets_.push_back(Json::getVec3<int>(target) + offset);
    }

    position_ = ci::Vec3f(block_position_);
    // block_positionが同じ高さなら、StageCubeの上に乗るように位置を調整
    position_().y += 1.0f;
  }

  ~Switch() {
    DOUT << "~Switch()" << std::endl;
    animation_timeline_->removeSelf();
  }


  void update(const double progressing_seconds) noexcept {
    rotation_ *= ci::Quatf(ci::Vec3f(0, 1, 0),
                           rotate_speed_ * progressing_seconds);
  }

  
  void entry() noexcept {
    active_ = true;
    
    // 登場演出
    auto entry_y = Json::getVec2<float>(params_["game.switch.entry_y"]);
    float y = ci::randFloat(entry_y.x, entry_y.y);
    ci::Vec3f start_value(position() + ci::Vec3f(0, y, 0));
    auto options = animation_timeline_->apply(&position_,
                                              start_value, position_(),
                                              params_["game.switch.entry_duration"].getValue<float>(),
                                              getEaseFunc(params_["game.switch.entry_ease"].getValue<std::string>()));

    options.finishFn([this]() {
        on_stage_ = true;
      });
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
        alive_  = false;
      });
  }

  bool checkStart(const ci::Vec3i& block_position) const noexcept {
    if (started_ || !on_stage_) return false;

    return block_position == block_position_;
  }

  void start() noexcept {
    started_ = true;
    active_  = false;
  }


  const std::vector<ci::Vec3i>& targets() const noexcept { return targets_; }


  const ci::Vec3f& position() const noexcept { return position_(); }
  const ci::Vec3i& blockPosition() const noexcept { return block_position_; }

  const ci::Quatf& rotation() const noexcept {
    return rotation_;
  }

  ci::Vec3f size() const noexcept { return ci::Vec3f::one(); }

  bool isActive() const noexcept { return active_; }
  bool isOnStage() const noexcept { return on_stage_; }
  bool isAlive() const noexcept { return alive_; }

  void alive(const bool live = true) noexcept { alive_ = live; }
  
  const ci::Color& color() const noexcept { return color_; }
  
  
private:

  

  
};

}
