#pragma once

//
// 実績の更新
//

#include "GameCenter.h"


namespace ngs {

void AchievementRequest(const std::string& id, const double value = 100.0) noexcept {
  GameCenter::submitAchievement(id, std::min(value, 100.0));
}

}
