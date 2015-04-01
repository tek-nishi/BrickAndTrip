﻿#pragma once

//
// File関連の雑多な処理
//

#include "cinder/app/App.h"


namespace ngs {

// Fileを書き出すpathを取得
#if defined(CINDER_COCOA_TOUCH)

ci::fs::path getDocumentPath();

#else

ci::fs::path getDocumentPath() {
  return ci::app::getAppPath();
}

#endif

}
