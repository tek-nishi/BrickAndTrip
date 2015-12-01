#pragma once

//
// サウンド管理
// 同じタイミングで同じ音が発声しないよう管理
//

#include <map>
#include "Sound.hpp"


namespace ngs {

class SoundPlayer : private boost::noncopyable {
  Sound& sound_;

  std::map<std::string, float> reserved_;
  

public:
  SoundPlayer(Sound& sound) noexcept :
    sound_(sound)
  {
    DOUT << "SoundPlayer()" << std::endl;
  }
  

  void play(const std::string& name, const float gain = 1.0f) noexcept {
    auto it = reserved_.find(name);
    if (it == std::end(reserved_)) {
      reserved_.insert({ name, gain });
    }
    else {
      // 先約がある場合、gainの大きいのを選択
      it->second = std::max(it->second, gain);
    }
  }  
  
  void update() noexcept {
    if (reserved_.empty()) return;

    for (auto& p : reserved_) {
      sound_.play(p.first, p.second);
    }

    reserved_.clear();
  }
  
};

}
