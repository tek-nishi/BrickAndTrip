#pragma once

//
// Appクラスの補助
//

namespace ngs { namespace AppSupport {

#if defined (CINDER_COCOA_TOUCH)

void pauseDraw(const bool pause) noexcept {
  auto* app = ci::app::AppCocoaTouch::get();
  app->pauseDraw(pause);
}

#else 

template <typename T>
void pauseDraw(const T pause) noexcept { }

#endif

} }
