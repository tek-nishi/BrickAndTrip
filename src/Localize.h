#pragma once

//
// ローカライズ済み文字列を取り出す
//

#include <string>


namespace ngs { namespace Localize {

#ifdef CINDER_COCOA_TOUCH
  
std::string get(const std::string& key) noexcept;

#else

template<typename T>
T get(const T& key) noexcept {
  return key;
}

#endif

} }
