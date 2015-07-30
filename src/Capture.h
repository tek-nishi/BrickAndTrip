#pragma once

//
// UIViewのキャプチャ(iOS専用)
//

#if defined(CINDER_COCOA_TOUCH)

namespace ngs {

bool canCaptureTopView();
bool canCapture(UIView* view);

UIImage* captureTopView();
UIImage* captureView(UIView* view);

}

#endif
