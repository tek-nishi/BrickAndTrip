#pragma once

//
// 
//

#include "Event.hpp"
#include "EventParam.hpp"


namespace ngs {

struct ControllerBase {
  virtual ~ControllerBase() {}

  virtual bool isActive() const = 0;

  virtual Event<EventParam>& event() = 0;

  virtual void resize() = 0;
  
  virtual void update(const double progressing_seconds) = 0;
  virtual void draw() = 0;
};

}
