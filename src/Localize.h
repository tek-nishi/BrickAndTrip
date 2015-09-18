#pragma once

//
// ローカライズ済み文字列を取り出す
//

#include <string>


namespace ngs {

#ifdef CINDER_COCOA_TOUCH
  
std::string localizedString(const std::string& key);

#else

template<typename T>
T localizedString(const T& key) {
  return key;
}

#endif

}
