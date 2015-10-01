#pragma once

//
// UIActivityViewControllerの薄いラッパー(iOS)
//

#include <string>
#include <functional>

namespace ngs {
namespace Share {

#if defined(CINDER_COCOA_TOUCH)

bool canPost() noexcept;
void post(const std::string& text, UIImage* image, std::function<void()> complete_callback) noexcept;

#else

template<typename T>
bool canPost(const T) noexcept { return false; }

#endif

}
}
