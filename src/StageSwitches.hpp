#pragma once

//
// stage上のswitch
//

#include "Switch.hpp"


namespace ngs {

class StageSwitches {
  std::vector<Switch> switches_;

  
public:
  void addSwitches(const ci::JsonTree& params,
                   const int bottom_z, const int offset_x) {
    if (!params.hasChild("switches")) return;

    for (const auto& p : params["switches"]) {
      switches_.emplace_back(p, offset_x, bottom_z);
    }
  }

  // FIXME:nullptrでデータの有無を判定
  const std::vector<ci::Vec3i>* const startSwitch(const ci::Vec3i& block_pos) {
    for (auto& s : switches_) {
      if (s.checkStart(block_pos)) {
        s.start();

        return &s.targets();
      }
    }
    
    return nullptr;
  }

  void clear() { switches_.clear(); }


private:

  
};

}
