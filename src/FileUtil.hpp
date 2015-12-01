#pragma once

//
// File関連の雑多な処理
//

#include <cinder/app/App.h>


namespace ngs {

// Fileを書き出すpathを取得
#if defined(CINDER_COCOA_TOUCH)

ci::fs::path getDocumentPath() noexcept;

#elif defined(CINDER_MAC)

template <typename T = void>
ci::fs::path getDocumentPath() noexcept {
  return ci::app::getAppPath() / "Contents/Resources";
}

#else

template <typename T = void>
ci::fs::path getDocumentPath() noexcept {
  return ci::app::getAppPath();
}

#endif

}
