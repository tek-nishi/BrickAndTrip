#pragma once

//
// stage上のスイッチ
// PickableCubeが踏むと指定ブロックの高さが書き換えられる
//

namespace ngs {

class Switch {
  ci::Vec3i pos_;
  std::vector<ci::Vec3i> targets_;

  bool started_;


public:
  Switch(const ci::JsonTree& params, const int offset_x, const int bottom_z) :
    started_(false)
  {
    ci::Vec3i offset(offset_x, 0, bottom_z);
    pos_ = Json::getVec3<int>(params["position"]) + offset;
    
    for (const auto& target : params["target"]) {
      targets_.push_back(Json::getVec3<int>(target) + offset);
    }
  }


  bool checkStart(const ci::Vec3i& block_position) const {
    if (started_) return false;

    return block_position == pos_;
  }

  void start() { started_ = true; }


  const std::vector<ci::Vec3i>& targets() const { return targets_; }
  
  
private:

  

  
};

}
