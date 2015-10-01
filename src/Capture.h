#pragma once

//
// UIViewのキャプチャ(iOS専用)
//

#if defined(CINDER_COCOA_TOUCH)

namespace ngs {

bool canCaptureTopView() noexcept;
bool canCapture(UIView* view) noexcept;

UIImage* captureTopView() noexcept;
UIImage* captureView(UIView* view) noexcept;

}

#endif
