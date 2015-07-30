#pragma once

//
// Social.frameworkの薄いラッパー(iOS)
//

#include <string>
#include <functional>

namespace ngs {
namespace Social {

enum Type {
  TWITTER,
  FACEBOOK
};


#if defined(CINDER_COCOA_TOUCH)

bool canPost(const Type type);
void post(const Type type, const std::string& text, UIImage* image, std::function<void()> complete_callback);

#else

template<typename T>
bool canPost(const T) { return false; }

#endif

}
}
