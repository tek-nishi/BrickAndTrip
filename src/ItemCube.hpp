#pragma once

//
// ステージ上に出現するCube
//


namespace ngs {

class ItemCube {
  ci::JsonTree& params_;
  Event<EventParam>& event_;

  bool active_;
  
  float cube_size_;
  ci::Color color_;

  ci::Vec3i block_position_;
  
  ci::Anim<ci::Vec3f> position_;
  ci::Anim<ci::Quatf> rotation_;

  bool on_stage_;

  std::string fall_ease_;
  float fall_duration_;
  float fall_y_;
  
  ci::TimelineRef animation_timeline_;


public:
  ItemCube(ci::JsonTree& params,
               ci::TimelineRef timeline,
               Event<EventParam>& event,
               const ci::Vec3i& entry_pos) :
    params_(params),
    event_(event),
    active_(true),
    cube_size_(params["game.cube_size"].getValue<float>()),
    color_(Json::getColor<float>(params["game.item.color"])),
    block_position_(entry_pos),
    rotation_(ci::Quatf::identity()),
    on_stage_(false),
    fall_ease_(params["game.item.fall_ease"].getValue<std::string>()),
    fall_duration_(params["game.item.fall_duration"].getValue<float>()),
    fall_y_(params["game.item.fall_y"].getValue<float>()),
    animation_timeline_(ci::Timeline::create())
  {
    DOUT << "ItemCube()" << std::endl;

    auto current_time = timeline->getCurrentTime();
    animation_timeline_->setStartTime(current_time);
    timeline->apply(animation_timeline_);

    position_ = ci::Vec3f(block_position_) * cube_size_;
    // block_positionが同じ高さなら、StageCubeの上に乗るように位置を調整
    position_().y += cube_size_;

    // 登場演出
    auto entry_y = Json::getVec2<float>(params["game.item.entry_y"]);
    float y = ci::randFloat(entry_y.x, entry_y.y) * cube_size_;
    ci::Vec3f start_value(position() + ci::Vec3f(0, y, 0));
    auto options = animation_timeline_->apply(&position_,
                                              start_value, position_(),
                                              params["game.item.entry_duration"].getValue<float>(),
                                              getEaseFunc(params["game.item.entry_ease"].getValue<std::string>()));

    options.finishFn([this]() {
        on_stage_ = true;
        
        EventParam params = {
          { "block_pos", block_position_ },
        };
        event_.signal("item-on-stage", params);
      });
  }

  ~ItemCube() {
    DOUT << "~ItemCube()" << std::endl;
    animation_timeline_->removeSelf();
  }


  void fallFromStage() {
    on_stage_ = false;

    ci::Vec3f end_value(position_() + ci::Vec3f(0, fall_y_ * cube_size_, 0));
    auto options = animation_timeline_->apply(&position_,
                                              end_value,
                                              fall_duration_,
                                              getEaseFunc(fall_ease_));

    options.finishFn([this]() {
        active_ = false;
      });
  }

  
  const ci::Vec3f& position() const { return position_(); }
  const ci::Quatf& rotation() const { return rotation_(); }

  const ci::Vec3i& blockPosition() const { return block_position_; }

  float cubeSize() const { return cube_size_; }
  ci::Vec3f size() const { return ci::Vec3f(cube_size_, cube_size_, cube_size_); }
  const ci::Color& color() const { return color_; }


  bool isActive() const { return active_; }
  bool isOnStage() const { return on_stage_; }

  
private:

  
};

}
