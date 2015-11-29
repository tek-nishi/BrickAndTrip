#pragma once

//
// Ease関連
//

#include <map>
#include <string>
#include "cinder/Easing.h"


namespace ngs {

template<typename T>
class EasePingPong {
  T ease_;

public:
  EasePingPong() = default;
  EasePingPong(const float a, const float b) noexcept :
    ease_(a, b)
  { }

  // FIXME:constにしたいが、Cinder側で付け忘れている:D
  float operator()(float t) noexcept {
    t *= 2.0f;
    if (t > 1.0f) t = 2.0f - t;
    return ease_(t);
  }
};


// FIXME:グローバル変数を排除
float ease_in_elastic_a    = 2;
float ease_in_elastic_b    = 1;
float ease_out_elastic_a   = 2;
float ease_out_elastic_b   = 1;
float ease_inout_elastic_a = 2;
float ease_inout_elastic_b = 1;
float ease_outin_elastic_a = 2;
float ease_outin_elastic_b = 1;


ci::EaseFn getEaseFunc(const std::string& name) noexcept {
  static const std::map<std::string, ci::EaseFn> tbl = {
    { "EaseInQuad", ci::EaseInQuad() },
    { "EaseOutQuad", ci::EaseOutQuad() },
    { "EaseInOutQuad", ci::EaseInOutQuad() },
    { "EaseOutInQuad", ci::EaseOutInQuad() },
      
    { "EaseInCubic", ci::EaseInCubic() },
    { "EaseOutCubic", ci::EaseOutCubic() },
    { "EaseInOutCubic", ci::EaseInOutCubic() },
    { "EaseOutInCubic", ci::EaseOutInCubic() },
      
    { "EaseInQuart", ci::EaseInQuart() },
    { "EaseOutQuart", ci::EaseOutQuart() },
    { "EaseInOutQuart", ci::EaseInOutQuart() },
    { "EaseOutInQuart", ci::EaseOutInQuart() },
      
    { "EaseInQuint", ci::EaseInQuint() },
    { "EaseOutQuint", ci::EaseOutQuint() },
    { "EaseInOutQuint", ci::EaseInOutQuint() },
    { "EaseOutInQuint", ci::EaseOutInQuint() },
      
    { "EaseInSine", ci::EaseInSine() },
    { "EaseOutSine", ci::EaseOutSine() },
    { "EaseInOutSine", ci::EaseInOutSine() },
    { "EaseOutInSine", ci::EaseOutInSine() },
      
    { "EaseInExpo", ci::EaseInExpo() },
    { "EaseOutExpo", ci::EaseOutExpo() },
    { "EaseInOutExpo", ci::EaseInOutExpo() },
    { "EaseOutInExpo", ci::EaseOutInExpo() },
      
    { "EaseInCirc", ci::EaseInCirc() },
    { "EaseOutCirc", ci::EaseOutCirc() },
    { "EaseInOutCirc", ci::EaseInOutCirc() },
    { "EaseOutInCirc", ci::EaseOutInCirc() },
      
    { "EaseInAtan", ci::EaseInAtan() },
    { "EaseOutAtan", ci::EaseOutAtan() },
    { "EaseInOutAtan", ci::EaseInOutAtan() },
    { "EaseNone", ci::EaseNone() },
      
    { "EaseInBack", ci::EaseInBack() },
    { "EaseOutBack", ci::EaseOutBack() },
    { "EaseInOutBack", ci::EaseInOutBack() },
    { "EaseOutInBack", ci::EaseOutInBack() },

    { "EaseInBounce", ci::EaseInBounce() },
    { "EaseOutBounce", ci::EaseOutBounce() },
    { "EaseInOutBounce", ci::EaseInOutBounce() },
    { "EaseOutInBounce", ci::EaseOutInBounce() },
      
    { "EaseInElastic", ci::EaseInElastic(ease_in_elastic_a,
                                         ease_in_elastic_b) },
    { "EaseOutElastic", ci::EaseOutElastic(ease_out_elastic_a,
                                           ease_out_elastic_b) },
    { "EaseInOutElastic", ci::EaseInOutElastic(ease_inout_elastic_a,
                                               ease_inout_elastic_b) },
    { "EaseOutInElastic", ci::EaseOutInElastic(ease_outin_elastic_a,
                                               ease_outin_elastic_b) },


    { "EasePingPongInQuad", EasePingPong<ci::EaseInQuad>() },
    { "EasePingPongOutQuad", EasePingPong<ci::EaseOutQuad>() },
    { "EasePingPongInOutQuad", EasePingPong<ci::EaseInOutQuad>() },
    { "EasePingPongOutInQuad", EasePingPong<ci::EaseOutInQuad>() },
      
    { "EasePingPongInCubic", EasePingPong<ci::EaseInCubic>() },
    { "EasePingPongOutCubic", EasePingPong<ci::EaseOutCubic>() },
    { "EasePingPongInOutCubic", EasePingPong<ci::EaseInOutCubic>() },
    { "EasePingPongOutInCubic", EasePingPong<ci::EaseOutInCubic>() },
      
    { "EasePingPongInQuart", EasePingPong<ci::EaseInQuart>() },
    { "EasePingPongOutQuart", EasePingPong<ci::EaseOutQuart>() },
    { "EasePingPongInOutQuart", EasePingPong<ci::EaseInOutQuart>() },
    { "EasePingPongOutInQuart", EasePingPong<ci::EaseOutInQuart>() },
      
    { "EasePingPongInQuint", EasePingPong<ci::EaseInQuint>() },
    { "EasePingPongOutQuint", EasePingPong<ci::EaseOutQuint>() },
    { "EasePingPongInOutQuint", EasePingPong<ci::EaseInOutQuint>() },
    { "EasePingPongOutInQuint", EasePingPong<ci::EaseOutInQuint>() },
      
    { "EasePingPongInSine", EasePingPong<ci::EaseInSine>() },
    { "EasePingPongOutSine", EasePingPong<ci::EaseOutSine>() },
    { "EasePingPongInOutSine", EasePingPong<ci::EaseInOutSine>() },
    { "EasePingPongOutInSine", EasePingPong<ci::EaseOutInSine>() },
      
    { "EasePingPongInExpo", EasePingPong<ci::EaseInExpo>() },
    { "EasePingPongOutExpo", EasePingPong<ci::EaseOutExpo>() },
    { "EasePingPongInOutExpo", EasePingPong<ci::EaseInOutExpo>() },
    { "EasePingPongOutInExpo", EasePingPong<ci::EaseOutInExpo>() },
      
    { "EasePingPongInCirc", EasePingPong<ci::EaseInCirc>() },
    { "EasePingPongOutCirc", EasePingPong<ci::EaseOutCirc>() },
    { "EasePingPongInOutCirc", EasePingPong<ci::EaseInOutCirc>() },
    { "EasePingPongOutInCirc", EasePingPong<ci::EaseOutInCirc>() },
      
    { "EasePingPongInAtan", EasePingPong<ci::EaseInAtan>() },
    { "EasePingPongOutAtan", EasePingPong<ci::EaseOutAtan>() },
    { "EasePingPongInOutAtan", EasePingPong<ci::EaseInOutAtan>() },
    { "EasePingPongNone", EasePingPong<ci::EaseNone>() },
      
    { "EasePingPongInBack", EasePingPong<ci::EaseInBack>() },
    { "EasePingPongOutBack", EasePingPong<ci::EaseOutBack>() },
    { "EasePingPongInOutBack", EasePingPong<ci::EaseInOutBack>() },
    { "EasePingPongOutInBack", EasePingPong<ci::EaseOutInBack>() },

    { "EasePingPongInBounce", EasePingPong<ci::EaseInBounce>() },
    { "EasePingPongOutBounce", EasePingPong<ci::EaseOutBounce>() },
    { "EasePingPongInOutBounce", EasePingPong<ci::EaseInOutBounce>() },
    { "EasePingPongOutInBounce", EasePingPong<ci::EaseOutInBounce>() },
      
    { "EasePingPongInElastic", EasePingPong<ci::EaseInElastic>(ease_in_elastic_a,
                                                               ease_in_elastic_b) },
    { "EasePingPongOutElastic", EasePingPong<ci::EaseOutElastic>(ease_out_elastic_a,
                                                                 ease_out_elastic_b) },
    { "EasePingPongInOutElastic", EasePingPong<ci::EaseInOutElastic>(ease_inout_elastic_a,
                                                                     ease_inout_elastic_b) },
    { "EasePingPongOutInElastic", EasePingPong<ci::EaseOutInElastic>(ease_outin_elastic_a,
                                                                     ease_outin_elastic_b) },
  };

  return tbl.at(name);
}

}
