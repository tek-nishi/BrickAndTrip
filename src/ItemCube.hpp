#pragma once

//
// ステージ上に出現するCube
//

#include <set>
#include <boost/noncopyable.hpp>
#include "TweenUtil.hpp"


namespace ngs {

class ItemCube : private boost::noncopyable {
  ci::JsonTree& params_;
  Event<EventParam>& event_;

  bool active_;

  u_int id_;
  
  ci::Vec3i block_position_;
  ci::Vec3i block_position_new_;
  
  ci::Anim<ci::Vec3f> position_;
  ci::Anim<ci::Vec3f> scale_;
  ci::Anim<ci::Vec3f> color_;

  ci::Anim<ci::Vec3f> offset_;

  ci::Vec3f rotation_;
  ci::Vec3f rotation_speed_;
  ci::Anim<float> rotation_speed_rate_;
  
  bool on_stage_;
  bool getatable_;

  std::string fall_ease_;
  float fall_duration_;
  float fall_y_;

  std::string move_ease_;
  float move_duration_;
  float move_delay_;
  
  ci::TimelineRef animation_timeline_;


public:
  ItemCube(ci::JsonTree& params,
           ci::TimelineRef timeline,
           Event<EventParam>& event,
           const ci::Vec3i& entry_pos) :
    params_(params),
    event_(event),
    active_(true),
    id_(getUniqueNumber()),
    color_(Json::getHsvColor(params["game.item.color"])),
    offset_(ci::Vec3f::zero()),
    block_position_(entry_pos),
    block_position_new_(block_position_),
    rotation_(ci::Vec3f::zero()),
    rotation_speed_(Json::getVec3<float>(params["game.item.rotation_speed"])),
    rotation_speed_rate_(0),
    scale_(ci::Vec3f::one()),
    on_stage_(false),
    getatable_(true),
    fall_ease_(params["game.item.fall_ease"].getValue<std::string>()),
    fall_duration_(params["game.item.fall_duration"].getValue<float>()),
    fall_y_(params["game.item.fall_y"].getValue<float>()),
    move_ease_(params["game.stage.move_ease"].getValue<std::string>()),
    move_duration_(params["game.stage.move_duration"].getValue<float>()),
    move_delay_(params["game.stage.move_delay"].getValue<float>()),
    animation_timeline_(ci::Timeline::create())
  {
    DOUT << "ItemCube()" << std::endl;

    auto current_time = timeline->getCurrentTime();
    animation_timeline_->setStartTime(current_time);
    timeline->apply(animation_timeline_);

    position_ = ci::Vec3f(block_position_);
    // block_positionが同じ高さなら、StageCubeの上に乗るように位置を調整
    position_().y += 1.0f;

    // 登場演出
    auto entry_y = Json::getVec2<float>(params["game.item.entry_y"]);
    float y = ci::randFloat(entry_y.x, entry_y.y);
    ci::Vec3f start_value(position() + ci::Vec3f(0, y, 0));
    float duration = params["game.item.entry_duration"].getValue<float>();
    auto options = animation_timeline_->apply(&position_,
                                              start_value, position_(),
                                              duration,
                                              getEaseFunc(params["game.item.entry_ease"].getValue<std::string>()));

    options.finishFn([this]() {
        on_stage_ = true;
        startTween("idle_tween");
        
        EventParam params = {
          { "block_pos", block_position_ },
        };
        event_.signal("item-on-stage", params);
      });
    
    setFloatTween(*animation_timeline_,
                  rotation_speed_rate_, params["game.item.entry_rotate_speed"], true);
  }

  ~ItemCube() {
    DOUT << "~ItemCube()" << std::endl;
    animation_timeline_->removeSelf();
  }


  void update(const double progressing_seconds) {
    rotation_ += rotation_speed_ * rotation_speed_rate_() * progressing_seconds;

    std::fmod(rotation_.x, static_cast<float>(M_PI * 2.0));
    std::fmod(rotation_.y, static_cast<float>(M_PI * 2.0));
    std::fmod(rotation_.z, static_cast<float>(M_PI * 2.0));
  }
  

  void fallFromStage() {
    on_stage_  = false;
    getatable_ = false;

    offset_.stop();
    
    ci::Vec3f end_value(block_position_ + ci::Vec3f(0, fall_y_, 0));
    auto options = animation_timeline_->apply(&position_,
                                              end_value,
                                              fall_duration_,
                                              getEaseFunc(fall_ease_));

    options.finishFn([this]() {
        active_ = false;
      });
  }

  void pickup() {
    getatable_ = false;
    on_stage_  = false;

    startTween("pickup_tween");

    animation_timeline_->add([this]() {
        active_ = false;
      },
      animation_timeline_->getCurrentTime() + params_["game.item.pickup_duration"].getValue<float>());

    EventParam params = {
      { "pos",      position() },
      { "size",     size() },
      { "sound",    std::string("item-pickup") },
    };
    event_.signal("view-sound", params);
  }

  void moveDown() {
    block_position_new_.y -= 1;

    // StageCubeの上に乗るように位置を調整している
    auto end_value = ci::Vec3f(block_position_new_.x, block_position_new_.y + 1, block_position_new_.z);
    // 直前のeasingが完了してから動作
    auto option = animation_timeline_->appendTo(&position_, end_value,
                                                move_duration_, getEaseFunc(move_ease_));

    option.delay(move_delay_);

    option.finishFn([this]() {
        block_position_.y -= 1;
      });
  }
  
  
  ci::Vec3f position() const { return position_() + offset_(); }

  ci::Quatf rotation() const {
    return ci::Quatf(rotation_.x, rotation_.y, rotation_.z);
  }

  const ci::Vec3i& blockPosition() const { return block_position_; }

  ci::Vec3f size() const { return scale_(); }

  ci::Color color() const {
    return ci::hsvToRGB(ci::Vec3f(std::fmod(color_().x, 1.0f), color_().y, color_().z));
  }

  u_int id() const { return id_; }

  bool isActive() const { return active_; }
  bool isOnStage() const { return on_stage_; }
  bool isGetatable() const { return getatable_; }
  
  // std::findを利用するための定義
  bool operator==(const u_int rhs_id) const {
    return id_ == rhs_id;
  }

  
private:
  void startTween(const std::string& name) {
    auto tween_params = params_["game.item." + name];

    std::set<std::string> applyed_targets;
    for (const auto& params : tween_params) {
      std::map<std::string,
               std::function<void (const ci::JsonTree&, const bool)> > tween_setup = {
        { "position",
          [this](const ci::JsonTree& params, const bool is_first) {
            setVec3Tween(*animation_timeline_, offset_, params, is_first);
          }
        },
        { "color",
          [this](const ci::JsonTree& params, const bool is_first) {
            setHsvTween(*animation_timeline_, color_, params, is_first);
          }
        },
        {
          "scale",
          [this](const ci::JsonTree& params, const bool is_first) {
            setVec3Tween(*animation_timeline_, scale_, params, is_first);
          }
        },
        {
          "rotation_speed",
          [this](const ci::JsonTree& params, const bool is_first) {
            setFloatTween(*animation_timeline_, rotation_speed_rate_, params, is_first);
          }
        }
      };

      const auto& target = params["target"].getValue<std::string>();
      tween_setup[target](params, isFirstApply(target, applyed_targets));
    }
  }
  
};

}
