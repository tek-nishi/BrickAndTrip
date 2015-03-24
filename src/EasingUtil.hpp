#pragma once

//
// Ease関連
//

#include <map>
#include <string>
#include "cinder/Easing.h"


namespace ngs {

ci::EaseFn getEaseFunc(const std::string& name) {
  static std::map<std::string, ci::EaseFn> tbl = {
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
      
    { "EaseInElastic", ci::EaseInElastic(2, 1) },
    { "EaseOutElastic", ci::EaseOutElastic(1, 4) },
    { "EaseInOutElastic", ci::EaseInOutElastic(2, 1) },
    { "EaseOutInElastic", ci::EaseOutInElastic(4, 1) },
  };
    
  return tbl[name];
}

}
