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

  struct Entry {
    ci::Vec3i position;
    float interval;
    float delay;
  };
  
  std::vector<Entry> entry_cubes_;

  // VS2013には暗黙のmoveコンストラクタが無いのでstd::unique_ptrで保持
  // std::vectorに格納するときに、copyやmoveコンストラクタが呼ばれる
  using FallingCubePtr = std::unique_ptr<FallingCube>;
  std::vector<FallingCubePtr> cubes_;

  ci::TimelineRef timeline_;

  
public:
  StageFallingCubes(ci::JsonTree& params,
                    ci::TimelineRef timeline,
                    Event<EventParam>& event) noexcept :
    params_(params),
    event_(event),
    timeline_(timeline)
  {
  }

  ~StageFallingCubes() {
  }

  
  void update(const double progressing_seconds,
              const Stage& stage) noexcept {
    for (auto& cube : cubes_) {
      cube->update(progressing_seconds);
    }
    
    decideEachCubeFalling(stage);
    
    boost::remove_erase_if(cubes_,
                           [](const FallingCubePtr& cube) {
                             return !cube->isActive();
                           });
  }


  void cleanup() noexcept { }
  
  void clear() noexcept {
    entry_cubes_.clear();
  }


  void addCubes(const ci::JsonTree& params, const int start_z, const int x_offset) noexcept {
    if (!params.hasChild("falling")) return;

    ci::Vec3i start_pos(x_offset, 0, start_z);

    for (const auto& p : params["falling"]) {
      auto entry_pos = Json::getVec3<int>(p["entry"]) + start_pos;
      auto interval = p["interval"].getValue<float>();
      auto delay    = p["delay"].getValue<float>();

      Entry entry = {
        entry_pos,
        interval,
        delay
      };
      
      entry_cubes_.push_back(std::move(entry));
    }
  }

  void entryCube(const int current_z) noexcept {
    for (const auto& entry : entry_cubes_) {
      if (entry.position.z == current_z) {
        cubes_.emplace_back(new FallingCube(params_,
                                            timeline_, event_,
                                            entry.position,
                                            entry.interval, entry.delay));
      }
    }
  }

  bool isCubeExists(const ci::Vec3i& block_pos) const noexcept {
    for (const auto& cube : cubes_) {
      if (cube->canBlock() && (block_pos == cube->blockPosition())) return true;
    }
    return false;
  }

  bool isCubePressed(const ci::Vec3i& block_pos) const noexcept {
    for (const auto& cube : cubes_) {
      if (block_pos != cube->blockPosition()) continue;
      
      return cube->canPress();
    }
    return false;
  }

  
  const std::vector<FallingCubePtr>& cubes() const { return cubes_; }

  
private:
  void decideEachCubeFalling(const Stage& stage) noexcept {
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
