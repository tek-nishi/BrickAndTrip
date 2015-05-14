#pragma once

//
// stage上のswitch
//

#include "Switch.hpp"
#include <boost/noncopyable.hpp>


namespace ngs {

class StageSwitches : private boost::noncopyable {
  Event<EventParam>& event_;

  ci::TimelineRef timeline_;
  ci::TimelineRef event_timeline_;
  
  using SwitchPtr = std::unique_ptr<Switch>;
  std::vector<SwitchPtr> switches_;

  
public:
  StageSwitches(ci::TimelineRef timeline,
                Event<EventParam>& event) :
    event_(event),
    timeline_(timeline),
    event_timeline_(ci::Timeline::create())
  {
    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    timeline->apply(event_timeline_);
  }

  ~StageSwitches() {
    event_timeline_->removeSelf();
  }
  

  void update(const double progressing_seconds, const Stage& stage) {
    for (auto& cube : switches_) {
      cube->update(progressing_seconds);
    }
    
    decideEachSwitchFalling(stage);
    
    boost::remove_erase_if(switches_,
                           [](const SwitchPtr& cube) {
                             return !cube->isAlive();
                           });
  }

  
  void addSwitches(ci::JsonTree& params,
                   const ci::JsonTree& entry_params,
                   const int bottom_z, const int offset_x) {
    if (!entry_params.hasChild("switches")) return;

    for (const auto& p : entry_params["switches"]) {
      auto obj = std::unique_ptr<Switch>(new Switch(params, p,
                                                    timeline_, event_,
                                                    offset_x, bottom_z));
      switches_.push_back(std::move(obj));
    }
  }

  void entrySwitches(const int current_z) {
    for (const auto& s : switches_) {
      if (s->blockPosition().z == current_z) {
        s->entry();
      }
    }
  }
  
  // FIXME:nullptrでデータの有無を判定
  const std::vector<ci::Vec3i>* const startSwitch(const ci::Vec3i& block_pos) {
    for (auto& s : switches_) {
      if (s->checkStart(block_pos)) {
        s->start();

        return &s->targets();
      }
    }
    
    return nullptr;
  }

  // FIXME:stageの崩壊とともに全てのswitchが落下するので
  //       明示的にclearする必要はなさそう
  void clear() { switches_.clear(); }


  const std::vector<SwitchPtr>& switches() const { return switches_; }

  
private:
  void decideEachSwitchFalling(const Stage& stage) {
    for (auto& cube : switches_) {
      if (!cube->isOnStage()) continue;
      
      auto height = stage.getStageHeight(cube->blockPosition());
      if (!height.first) {
        cube->fallFromStage();

        event_.signal("fall-switch", EventParam());
      }
    }
  }
  
};

}
