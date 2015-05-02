#pragma once

//
// Stage上のMovingCube
//

#include "MovingCube.hpp"


namespace ngs {

class StageMovingCubes {
  ci::JsonTree& params_;
  Event<EventParam>& event_;

  std::vector<ci::Vec3i> entry_cubes_;

  // VS2013には暗黙のmoveコンストラクタが無いのでstd::unique_ptrで保持
  // std::vectorに格納するときに、copyやmoveコンストラクタが呼ばれる
  using MovingCubePtr = std::unique_ptr<MovingCube>;
  std::vector<MovingCubePtr> cubes_;

  ci::TimelineRef timeline_;
  ci::TimelineRef event_timeline_;

  
public:
  StageMovingCubes(ci::JsonTree& params,
             ci::TimelineRef timeline,
                   Event<EventParam>& event) :
    params_(params),
    event_(event),
    timeline_(timeline),
    event_timeline_(ci::Timeline::create())
  {
    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    timeline->apply(event_timeline_);
  }

  ~StageMovingCubes() {
  }

  
  void update(const Stage& stage) {
    decideEachCubeFalling(stage);
    
    boost::remove_erase_if(cubes_,
                           [](const MovingCubePtr& cube) {
                             return !cube->isActive();
                           });
  }

  void clear() {
    entry_cubes_.clear();
  }


  void addCubes(const ci::JsonTree& params, const int start_z, const int x_offset) {
    if (!params.hasChild("moving")) return;

    ci::Vec3i start_pos(x_offset, 0, start_z);

    for (const auto& entry : params["moving"]) {
      entry_cubes_.push_back(Json::getVec3<int>(entry) + start_pos);
    }
  }

  void entryCube(const int current_z) {
    for (const auto& entry : entry_cubes_) {
      if (entry.z == current_z) {
        auto cube = MovingCubePtr(new MovingCube(params_, timeline_, event_, entry));
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

  
  const std::vector<MovingCubePtr>& cubes() const { return cubes_; }

  
private:
  void decideEachCubeFalling(const Stage& stage) {
    for (auto& cube : cubes_) {
      if (!cube->isOnStage()) continue;
      
      auto height = stage.getStageHeight(cube->blockPosition());
      if (!height.first) {
        cube->fallFromStage();

        event_.signal("fall-moving", EventParam());
      }
    }
  }
  
};

}
