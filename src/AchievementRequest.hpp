#pragma once

//
// 実績の更新
//

#include "Event.hpp"
#include "EventParam.hpp"


namespace ngs {

void AchievementRequest(Event<EventParam>& event,
                        const std::string& id, const double value = 100.0) noexcept {
  EventParam params = {
    { "id",    id },
    { "value", std::min(value, 100.0) },
  };
  event.signal("entry-achievement", params);
}

}
