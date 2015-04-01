﻿#pragma once

//
// 機種判定
//

#include <string>
#if defined(CINDER_COCOA_TOUCH)
#include <sys/utsname.h>
#endif


namespace ngs {

#if defined(CINDER_COCOA_TOUCH)

std::string decideHard() {
  utsname system_info;
  uname(&system_info);

  return std::string(system_info.machine);
}

#elif defined(CINDER_MAC)

std::string decideHard() {
  return std::string("OSX");
}

#elif defined(CINDER_MSW)

std::string decideHard() {
  return std::string("Windows");
}

#else

std::string decideHard() {
  return std::string("unknown");
}

#endif

}
