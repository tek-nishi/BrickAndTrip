#pragma once

//
// 実行環境ごとにAAのタイプを決める
//

#include "LowEfficiencyDevice.hpp"
#include "cinder/app/Renderer.h"


namespace ngs { namespace AntiAliasingType {

// iOSとPCでAAの有効値が違うので、別名を定義
#if defined(CINDER_COCOA_TOUCH)

enum { AA_TYPE_ALIAS = ci::app::RendererGl::AA_MSAA_4 };

#else

enum { AA_TYPE_ALIAS = ci::app::RendererGl::AA_MSAA_16 };

#endif


int get() noexcept {
  bool low_Lowefficiency = LowEfficiencyDevice::determine();
  
  return low_Lowefficiency ? ci::app::RendererGl::AA_NONE : AA_TYPE_ALIAS;
}

} }
