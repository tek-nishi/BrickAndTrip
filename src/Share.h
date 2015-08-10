#pragma once

//
// UIActivityViewControllerの薄いラッパー(iOS)
//

#include <string>
#include <functional>

namespace ngs {
namespace Share {

#if defined(CINDER_COCOA_TOUCH)

bool canPost();
void post(const std::string& text, UIImage* image, std::function<void()> complete_callback);

#else

template<typename T>
bool canPost(const T) { return false; }

#endif

}
}
