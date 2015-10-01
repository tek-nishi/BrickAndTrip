#pragma once

//
// 実行環境ごとにAAのタイプを決める
//

#include "DecideHard.hpp"
#include "cinder/app/Renderer.h"
#include <string>
#include <regex>


namespace ngs {

#if defined(CINDER_COCOA_TOUCH)

int getAntiAliasingType() noexcept {
  using namespace ci::app;
  
  auto hard_type = decideHard();
  
  // ざっくり機種判定
  std::regex iphone("^iPhone(\\d+),");
  std::regex ipad("^iPad(\\d+),");
  std::regex ipod("^iPod(\\d+),");

  std::smatch result;

  if (std::regex_search(hard_type, result, iphone)) {
    // iPhone4S以降でAA有効
    DOUT << result[0] << " " << result[1] << std::endl;
    
    return (std::stoi(result[1]) >= 4) ? RendererGl::AA_MSAA_4 : RendererGl::AA_NONE;
  }
  else if (std::regex_search(hard_type, result, ipad)) {
    DOUT << result[0] << " " << result[1] << std::endl;

    // iPad2より新しいiPadでAA有効
    return (std::stoi(result[1]) >= 2) ? RendererGl::AA_MSAA_4 : RendererGl::AA_NONE;
  }
  else if (std::regex_search(hard_type, result, ipod)) {
    DOUT << result[0] << " " << result[1] << std::endl;

    // iPod第４世代(Retina、カメラ搭載)以降でAA有効
    return (std::stoi(result[1]) >= 4) ? RendererGl::AA_MSAA_4 : RendererGl::AA_NONE;
  }

  // 新機種はAAありで
  return RendererGl::AA_MSAA_4;
}

#else

int getAntiAliasingType() noexcept {
  using namespace ci::app;

  // OSXとWindowsは初期値
  return RendererGl::AA_MSAA_16;
}

#endif

}
