#pragma once

//
// サウンドリクエスト
// 

#include "Event.hpp"
#include "EventParam.hpp"


namespace ngs {

void requestSound(Event<EventParam>& event, const std::string& sound_name) noexcept {
  EventParam params = {
    { "sound", sound_name }
  };
  
  event.signal("sound-play", params);
}

}
