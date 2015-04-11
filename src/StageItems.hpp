#pragma once

//
// Stage上のItem管理
//

#include <boost/noncopyable.hpp>
#include "Stage.hpp"
#include "ItemCube.hpp"


namespace ngs {

class StageItems : private boost::noncopyable {
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
  
  int addItemCubes(const ci::JsonTree& params, const int start_z, const int x_offset) {
    if (!params.hasChild("items")) return 0;

    ci::Vec3i start_pos(x_offset, 0, start_z);

    int entry_num = 0;
    for (const auto& entry : params["items"]) {
      entry_items_.push_back(Json::getVec3<int>(entry) + start_pos);
    }

    return entry_num;
  }

  void entryItemCube(const int current_z) {
    for (const auto& entry : entry_items_) {
      if (entry.z == current_z) {
        auto cube = ItemCubePtr(new ItemCube(params_, timeline_, event_, entry));
        items_.push_back(std::move(cube));
      }
    }
  }


  std::pair<bool, u_int> canGetItemCube(const ci::Vec3i& block_pos) {
    if (items_.empty()) return std::make_pair(false, 0);
    
    for (const auto& cube : items_) {
      if (cube->isOnStage()
          && (block_pos == cube->blockPosition())) {
        return std::make_pair(true, cube->id());
      }
    }
    return std::make_pair(false, 0);
  }

  void pickupItemCube(const u_int id) {
    auto it = std::find_if(std::begin(items_), std::end(items_),
                           [id](const ItemCubePtr& obj) {
                             return *obj == id;
                           });
    assert(it != std::end(items_));
    (*it)->pickup();

    // 演出上、signalは時間差で
    event_timeline_->add([this]() {
        event_.signal("pickuped-item", EventParam());
      },
      event_timeline_->getCurrentTime() + params_["game.item.pickup_delay"].getValue<float>());
  }
  

  const std::vector<ItemCubePtr>& items() const { return items_; }
  

private:
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
