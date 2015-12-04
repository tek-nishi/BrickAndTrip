#pragma once

//
// サウンド管理
// 同じタイミングで同じ音が発声しないよう管理
//

#include <set>
#include "Sound.hpp"


namespace ngs {

class SoundPlayer : private boost::noncopyable {
  std::set<std::string> reserved_;
  

public:
  SoundPlayer()  = default;
  

  void play(const std::string& name) noexcept {
    reserved_.insert(name);
  }  
  
  void update(Sound& sound) noexcept {
    if (reserved_.empty()) return;

    for (const auto& name : reserved_) {
      sound.play(name);
    }

    reserved_.clear();
  }
  
};

}
