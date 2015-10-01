#pragma once

//
// 振動
//

namespace ngs {

class Quake {
  float power_;
  float speed_;
  float w_st_;
  float k_;

  float* target_;
  ci::FnTweenRef<float> tween_;

  
public:
  Quake(const ci::JsonTree& params) :
    power_(params["power"].getValue<float>()),
    speed_(ci::toRadians(params["speed"].getValue<float>())),
    w_st_(ci::toRadians(params["w_st"].getValue<float>())),
    k_(params["k"].getValue<float>()),
    target_(nullptr)
  {}

  ~Quake() {
    if (tween_) tween_->removeSelf();
  }
  

  void start(ci::Timeline& timeline, float* target, const float duration) noexcept {
    if (tween_) tween_->removeSelf();
    target_ = target;
    
    tween_ = timeline.applyFn<float>([this](float time) {
        // 減衰振動の方程式をそのまま利用
        // y = e^-kx * sin(w0 + wx)
        float exp_value = std::exp(-k_ * time);
        *target_ = exp_value * std::sin(w_st_ + speed_ * time) * power_;
      },
      0.0f, duration,
      duration);
  }

  void stop() noexcept {
    if (tween_) tween_->removeSelf();
  }
  
};

}
