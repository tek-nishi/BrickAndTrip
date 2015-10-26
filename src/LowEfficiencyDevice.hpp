#pragma once

//
// 性能の低いハードをモデル名から判別する(iOS)
// TODO:２段階くらい欲しい
//

#include "DecideHard.hpp"
#include <string>
#include <regex>


namespace ngs {
namespace LowEfficiencyDevice {

#if defined(CINDER_COCOA_TOUCH)

bool determine() noexcept {
  auto hard_type = decideHard();
  
  // ざっくり機種判定
  std::regex iphone("^iPhone(\\d+),");
  std::regex ipad("^iPad(\\d+),");
  std::regex ipod("^iPod(\\d+),");

  std::smatch result;

  if (std::regex_search(hard_type, result, iphone)) {
    // iPhone5以前を低性能と扱う
    DOUT << result[0] << " " << result[1] << std::endl;
    
    return (std::stoi(result[1]) >= 5) ? false : true;
  }
  else if (std::regex_search(hard_type, result, ipad)) {
    DOUT << result[0] << " " << result[1] << std::endl;

    // iPad2以前を低性能と扱う
    return (std::stoi(result[1]) >= 2) ? false : true;
  }
  else if (std::regex_search(hard_type, result, ipod)) {
    DOUT << result[0] << " " << result[1] << std::endl;

    // iPod第6世代(Retina、カメラ搭載)以前を低性能と扱う
    return (std::stoi(result[1]) >= 6) ? false : true;
  }

  // 新機種は全て高性能
  return false;
}

#else

bool determine() noexcept {
  // PCはすべて高性能と扱う
  return false;
}

#endif

}
}
