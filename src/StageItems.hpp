#pragma once

//
// Stage上のItem管理
//

#include "Stage.hpp"
#include "ItemCube.hpp"


namespace ngs {

class StageItems {
  ci::JsonTree& params_;
  Event<EventParam>& event_;
  
  std::vector<ci::Vec3i> entry_items_;

  // VS2013には暗黙のmoveコンストラクタが無いのでstd::unique_ptrで保持
  // std::vectorに格納するときに、copyやmoveコンストラクタが呼ばれる
  using ItemCubePtr = std::unique_ptr<ItemCube>;
  std::vector<ItemCubePtr> items_;

  ci::TimelineRef timeline_;
  ci::TimelineRef event_timeline_;
  

public:
  StageItems(ci::JsonTree& params,
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

  ~StageItems() {
    event_timeline_->removeSelf();
  }


  void update(const Stage& stage) {
    decideEachItemCubeFalling(stage);
    
    boost::remove_erase_if(items_,
                           [](const ItemCubePtr& cube) {
                             return !cube->isActive();
                           });
  }

  
  void clear() {
    entry_items_.clear();
  }
  
  void addItemCubes(const ci::JsonTree& params, const int start_z) {
    if (!params.hasChild("items")) return;

    ci::Vec3i start_pos(0, 0, start_z);
    
    for (const auto& entry : params["items"]) {
      entry_items_.push_back(Json::getVec3<int>(entry) + start_pos);
    }
  }

  void entryItemCube(const int current_z) {
    for (const auto& entry : entry_items_) {
      if (entry.z == current_z) {
        DOUT << "entry:" << current_z << std::endl;
        auto cube = ItemCubePtr(new ItemCube(params_, timeline_, event_, entry));
        items_.push_back(std::move(cube));
      }
    }
  }


  const std::vector<ItemCubePtr>& items() const { return items_; }
  

private:
  // TIPS:コピー不可
  StageItems(const StageItems&) = delete;
  StageItems& operator=(const StageItems&) = delete;


  void decideEachItemCubeFalling(const Stage& stage) {
    for (auto& cube : items_) {
      if (!cube->isOnStage()) continue;
      
      auto height = stage.getStageHeight(cube->blockPosition());
      if (!height.first) {
        cube->fallFromStage();

        event_.signal("fall-item", EventParam());
      }
    }
  }

  
};

}
