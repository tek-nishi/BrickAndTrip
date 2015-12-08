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

  std::string shadow_ease_;
  float shadow_duration_;
  ci::Anim<float> shadow_alpha_;
  
  ci::TimelineRef animation_timeline_;


public:
  ItemCube(ci::JsonTree& params,
           ci::TimelineRef timeline,
           Event<EventParam>& event,
           const ci::Vec3i& entry_pos) noexcept :
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
    shadow_ease_(params["game.item.shadow_ease"].getValue<std::string>()),
    shadow_duration_(params["game.item.shadow_duration"].getValue<float>()),
    shadow_alpha_(0.0f),
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

    options.finishFn([this]() noexcept {
        on_stage_ = true;
        startTween("idle_tween");

        startShadowAlphaTween(1.0f);
        
        EventParam params = {
          { "block_pos", block_position_ },
        };
        event_.signal("item-on-stage", params);
      });
    
    setFloatTween(*animation_timeline_,
                  rotation_speed_rate_, params["game.item.entry_rotate_speed"], true);
  }

  ~ItemCube() noexcept {
    DOUT << "~ItemCube()" << std::endl;
    animation_timeline_->removeSelf();
  }


  void update(const double progressing_seconds) noexcept {
    rotation_ += rotation_speed_ * rotation_speed_rate_() * progressing_seconds;

    std::fmod(rotation_.x, static_cast<float>(M_PI * 2.0));
    std::fmod(rotation_.y, static_cast<float>(M_PI * 2.0));
    std::fmod(rotation_.z, static_cast<float>(M_PI * 2.0));
  }
  

  void fallFromStage() noexcept {
    on_stage_  = false;
    getatable_ = false;

    offset_.stop();
    
    ci::Vec3f end_value(block_position_ + ci::Vec3f(0, fall_y_, 0));
    auto options = animation_timeline_->apply(&position_,
                                              end_value,
                                              fall_duration_,
                                              getEaseFunc(fall_ease_));

    options.finishFn([this]() noexcept {
        active_ = false;
      });

    startShadowAlphaTween(0.0f);
  }

  void pickup() noexcept {
    getatable_ = false;
    on_stage_  = false;

    startTween("pickup_tween");

    animation_timeline_->add([this]() noexcept {
        active_ = false;
      },
      animation_timeline_->getCurrentTime() + params_["game.item.pickup_duration"].getValue<float>());

    startShadowAlphaTween(0.0f);

    EventParam params = {
      { "pos",      position() },
      { "size",     size() },
      { "sound",    std::string("item-pickup") },
    };
    event_.signal("view-sound", params);
  }

  void moveDown() noexcept {
    block_position_new_.y -= 1;

    // StageCubeの上に乗るように位置を調整している
    auto end_value = ci::Vec3f(block_position_new_.x, block_position_new_.y + 1, block_position_new_.z);
    // 直前のeasingが完了してから動作
    auto option = animation_timeline_->appendTo(&position_, end_value,
                                                move_duration_, getEaseFunc(move_ease_));

    option.delay(move_delay_);

    option.finishFn([this]() noexcept {
        block_position_.y -= 1;
      });
  }
  
  
  ci::Vec3f position() const noexcept { return position_() + offset_(); }
  float stageHeight() const noexcept { return position_().y - 0.5f; }

  ci::Quatf rotation() const noexcept {
    // オイラー角からクオータニオンを生成
    //   cinderの実装が間違っている
    //   SOURCE: http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q60
    const float fSinPitch(std::sin(rotation_.x * 0.5f));
    const float fCosPitch(std::cos(rotation_.x * 0.5f));
    const float fSinYaw(std::sin(rotation_.y * 0.5f));
    const float fCosYaw(std::cos(rotation_.y * 0.5f));
    const float fSinRoll(std::sin(rotation_.z * 0.5f));
    const float fCosRoll(std::cos(rotation_.z * 0.5f));
    
    const float fCosPitchCosYaw(fCosPitch * fCosYaw);
    const float fSinPitchSinYaw(fSinPitch * fSinYaw);
    
    float x = fSinRoll * fCosPitchCosYaw     - fCosRoll * fSinPitchSinYaw;
    float y = fCosRoll * fSinPitch * fCosYaw + fSinRoll * fCosPitch * fSinYaw;
    float z = fCosRoll * fCosPitch * fSinYaw - fSinRoll * fSinPitch * fCosYaw;
    float w = fCosRoll * fCosPitchCosYaw     + fSinRoll * fSinPitchSinYaw;    

    return ci::Quatf(w, x, y, z);
  }

  const ci::Vec3i& blockPosition() const noexcept { return block_position_; }

  ci::Vec3f size() const noexcept { return scale_(); }

  ci::Color color() const noexcept {
    return ci::hsvToRGB(ci::Vec3f(std::fmod(color_().x, 1.0f), color_().y, color_().z));
  }

  u_int id() const noexcept { return id_; }

  bool isActive() const noexcept { return active_; }
  bool isOnStage() const noexcept { return on_stage_; }
  bool isGetatable() const noexcept { return getatable_; }

  float shadowAlpha() const noexcept { return shadow_alpha_(); }

  
  // std::findを利用するための定義
  bool operator==(const u_int rhs_id) const noexcept {
    return id_ == rhs_id;
  }

  
private:
  void startTween(const std::string& name) noexcept {
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

  void startShadowAlphaTween(const float target_alpha) noexcept {
    animation_timeline_->apply(&shadow_alpha_,
                               target_alpha,
                               shadow_duration_,
                               getEaseFunc(shadow_ease_));
  }
  
};

}
