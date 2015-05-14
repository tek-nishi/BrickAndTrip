#pragma once

//
// Stage上のドッスン
//

#include "FallingCube.hpp"
#include <boost/noncopyable.hpp>


namespace ngs {

class StageFallingCubes : private boost::noncopyable {
  ci::JsonTree& params_;
  Event<EventParam>& event_;

  std::vector<ci::Vec3i> entry_cubes_;

  // VS2013には暗黙のmoveコンストラクタが無いのでstd::unique_ptrで保持
  // std::vectorに格納するときに、copyやmoveコンストラクタが呼ばれる
  using FallingCubePtr = std::unique_ptr<FallingCube>;
  std::vector<FallingCubePtr> cubes_;

  ci::TimelineRef timeline_;

  
public:
  StageFallingCubes(ci::JsonTree& params,
                    ci::TimelineRef timeline,
                    Event<EventParam>& event) :
    params_(params),
    event_(event),
    timeline_(timeline)
  {
  }

  ~StageFallingCubes() {
  }

  
  void update(const double progressing_seconds,
              const Stage& stage) {
    for (auto& cube : cubes_) {
      cube->update(progressing_seconds);
    }
    
    decideEachCubeFalling(stage);
    
    boost::remove_erase_if(cubes_,
                           [](const FallingCubePtr& cube) {
                             return !cube->isActive();
                           });
  }


  void cleanup() {
  }
  
  void clear() {
    entry_cubes_.clear();
  }


  void addCubes(const ci::JsonTree& params, const int start_z, const int x_offset) {
    if (!params.hasChild("falling")) return;

    ci::Vec3i start_pos(x_offset, 0, start_z);

    for (const auto& p : params["falling"]) {
      ci::Vec3i entry_pos = Json::getVec3<int>(p) + start_pos;
      entry_cubes_.push_back(entry_pos);
    }
  }

  void entryCube(const int current_z) {
    for (const auto& entry : entry_cubes_) {
      if (entry.z == current_z) {
        auto cube = FallingCubePtr(new FallingCube(params_, timeline_, event_, entry));
        cubes_.push_back(std::move(cube));
      }
    }
  }

  bool isCubeExists(const ci::Vec3i& block_pos) const {
    for (const auto& cube : cubes_) {
      if (block_pos == cube->blockPosition()) return true;
    }
    return false;
  }

  
  const std::vector<FallingCubePtr>& cubes() const { return cubes_; }

  
private:
  void decideEachCubeFalling(const Stage& stage) {
    for (auto& cube : cubes_) {
      if (!cube->isOnStage()) continue;
      
      auto height = stage.getStageHeight(cube->blockPosition());
      if (!height.first) {
        cube->fallFromStage();

        event_.signal("fall-falling", EventParam());
      }
    }
  }
  
};

}
