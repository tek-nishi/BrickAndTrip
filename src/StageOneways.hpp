#pragma once

//
// Stage上の一方通行
//

#include "Oneway.hpp"
#include <boost/noncopyable.hpp>


namespace ngs {

class StageOneways : private boost::noncopyable {
  Event<EventParam>& event_;

  ci::TimelineRef timeline_;
  ci::TimelineRef event_timeline_;
  
  using OnewayPtr = std::unique_ptr<Oneway>;
  std::vector<OnewayPtr> objects_;

  
public:
  StageOneways(ci::TimelineRef timeline,
               Event<EventParam>& event) noexcept :
    event_(event),
    timeline_(timeline),
    event_timeline_(ci::Timeline::create())
  {
    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    timeline->apply(event_timeline_);
  }

  ~StageOneways() {
    event_timeline_->removeSelf();
  }
  

  void update(const double progressing_seconds,
              const Stage& stage) noexcept {
    for (auto& obj : objects_) {
      obj->update(progressing_seconds);
    }
    
    decideEachOnewayFalling(stage);
    
    boost::remove_erase_if(objects_,
                           [](const OnewayPtr& obj) {
                             return !obj->isAlive();
                           });
  }

  
  void addOneways(ci::JsonTree& params,
                  const ci::JsonTree& entry_params,
                  const int bottom_z, const int offset_x) noexcept {
    if (!entry_params.hasChild("oneways")) return;

    for (const auto& p : entry_params["oneways"]) {
      auto obj = std::unique_ptr<Oneway>(new Oneway(params, p,
                                                    timeline_, event_,
                                                    offset_x, bottom_z));
      objects_.push_back(std::move(obj));
    }
  }

  void entryOneways(const int current_z) noexcept {
    for (const auto& obj : objects_) {
      if (obj->blockPosition().z == current_z) {
        obj->entry();
      }
    }
  }

  std::pair<int, int> startOneway(const ci::Vec3i& block_pos) noexcept {
    for (auto& obj : objects_) {
      if (obj->checkStart(block_pos)) {
        // obj->start();
        return std::make_pair(obj->direction(), obj->power());
      }
    }
    
    return std::make_pair(Oneway::NONE, 0);
  }

  void clear() noexcept {
    // 待機中のonewayをすべて削除
    for (auto& cube : objects_) {
      if (cube->isAlive() && !cube->isActive()) {
        cube->alive(false);
      }
    }
  }


  const std::vector<OnewayPtr>& oneways() const noexcept { return objects_; }

  
private:
  void decideEachOnewayFalling(const Stage& stage) noexcept {
    for (auto& obj : objects_) {
      if (!obj->isOnStage()) continue;
      
      auto height = stage.getStageHeight(obj->blockPosition());
      if (!height.first) {
        obj->fallFromStage();

        event_.signal("fall-oneway", EventParam());
      }
    }
  }
  
};

}
