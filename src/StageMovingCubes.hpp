#pragma once

//
// Stage上のMovingCube
//

#include "MovingCube.hpp"
#include <boost/noncopyable.hpp>


namespace ngs {

class StageMovingCubes : private boost::noncopyable {
  ci::JsonTree& params_;
  Event<EventParam>& event_;

  struct Entry {
    ci::Vec3i pos;
    std::vector<int> move_pattern;
  };
  std::vector<Entry> entry_cubes_;

  // VS2013には暗黙のmoveコンストラクタが無いのでstd::unique_ptrで保持
  // std::vectorに格納するときに、copyやmoveコンストラクタが呼ばれる
  using MovingCubePtr = std::unique_ptr<MovingCube>;
  std::vector<MovingCubePtr> cubes_;

  ci::TimelineRef timeline_;

  
public:
  StageMovingCubes(ci::JsonTree& params,
             ci::TimelineRef timeline,
                   Event<EventParam>& event) noexcept :
    params_(params),
    event_(event),
    timeline_(timeline)
  {
  }

  ~StageMovingCubes() {
  }

  
  void update(const double progressing_seconds,
              const Stage& stage,
              const std::vector<ci::Vec3i>& pickables) noexcept {
    for (auto& cube : cubes_) {
      cube->update(progressing_seconds);
    }
    
    decideEachCubeFalling(stage);
    decideEachCubeMoving(stage, pickables);
    
    boost::remove_erase_if(cubes_,
                           [](const MovingCubePtr& cube) {
                             return !cube->isActive();
                           });
  }


  void cleanup() noexcept { }
  
  void clear() noexcept {
    entry_cubes_.clear();
  }


  void addCubes(const ci::JsonTree& params, const int start_z, const int x_offset) noexcept {
    if (!params.hasChild("moving")) return;

    ci::Vec3i start_pos(x_offset, 0, start_z);

    for (const auto& p : params["moving"]) {
      Entry entry = {
        Json::getVec3<int>(p["entry"]) + start_pos,
      };
      
      for (auto& value : p["pattern"]) {
        entry.move_pattern.push_back(value.getValue<int>());
      }
      
      entry_cubes_.push_back(entry);
    }
  }

  void entryCube(const int current_z) noexcept {
    for (const auto& entry : entry_cubes_) {
      if (entry.pos.z == current_z) {
        auto cube = MovingCubePtr(new MovingCube(params_, timeline_, event_, entry.pos, entry.move_pattern));
        cubes_.push_back(std::move(cube));
      }
    }
  }

  bool isCubeExists(const ci::Vec3i& block_pos) const noexcept {
    for (const auto& cube : cubes_) {
      if (block_pos == cube->blockPosition()) return true;

      // 移動中の場合は直前の位置も判定
      if (cube->isMoving()
          && (block_pos == cube->prevBlockPosition())) return true;
    }
    return false;
  }

  void moveCube(const ci::Vec3i& block_pos) noexcept {
    for (auto& cube : cubes_) {
      const auto& cube_block_pos = cube->blockPosition();
      if ((cube_block_pos.x == block_pos.x) && (cube_block_pos.z == block_pos.z)) {
        cube->moveDown();
        return;
      }
    }
  }

  
  const std::vector<MovingCubePtr>& cubes() const noexcept { return cubes_; }

  
private:
  void decideEachCubeMoving(const Stage& stage,
                            const std::vector<ci::Vec3i>& pickables) noexcept {
    for (auto& cube : cubes_) {
      if (cube->willRotationMove()) {
        // 移動できなかったときにすこし間をおいて
        // 再移動するための前処理
        cube->removeRotationMoveReserve();
        
        auto moving_pos = cube->blockPosition() + cube->moveVector();

        // 他のMovingがいたらダメ
        if (isOtherMovingCubeExists(cube, moving_pos)) continue;

        // 他のPickableがいたらダメ
        if (isPickableCubeExists(moving_pos, pickables)) continue;

        // stageの高さが違ったらダメ
        if (!isStageHeightSame(moving_pos, stage)) continue;
        
        cube->startRotationMove();
      }
    }
  }
  
  void decideEachCubeFalling(const Stage& stage) noexcept {
    for (auto& cube : cubes_) {
      if (!cube->isOnStage() || cube->isMoving()) continue;
      
      auto height = stage.getStageHeight(cube->blockPosition());
      if (!height.first) {
        cube->fallFromStage();

        event_.signal("fall-moving", EventParam());
      }
    }
  }


  bool isOtherMovingCubeExists(const MovingCubePtr& cube,
                               const ci::Vec3i& block_pos) const noexcept {
    for (const auto& other_cube : cubes_) {
      if (*cube == *other_cube) continue;

      if (block_pos == other_cube->blockPosition()) return true;

      // 移動中の場合は直前の位置も判定
      if (other_cube->isMoving()
          && block_pos == other_cube->prevBlockPosition()) return true;
      
    }
    return false;
  }

  bool isPickableCubeExists(const ci::Vec3i& block_pos,
                            const std::vector<ci::Vec3i>& pickables) const noexcept {
    for (const auto& pos : pickables) {
      if (pos == block_pos) return true;
    }
    return false;
  }

  bool isStageHeightSame(const ci::Vec3i& block_pos,
                         const Stage& stage) const noexcept {
    auto height = stage.getStageHeight(block_pos);
    return height.first && (height.second == block_pos.y);
  }
  
};

}
