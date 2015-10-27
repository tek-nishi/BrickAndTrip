#pragma once

//
// UIActivityViewControllerの薄いラッパー(iOS)
//

#include <string>
#include <functional>

namespace ngs { namespace Share {

#if defined(CINDER_COCOA_TOUCH)

bool canPost() noexcept;
void post(const std::string& text, UIImage* image, std::function<void()> complete_callback) noexcept;

#else

template <typename T = void>
bool canPost() noexcept { return false; }

template <typename T1, typename T2, typename T3>
void post(const T1& text, T2* image, T3 complete_callback) noexcept {}

#endif

} }
