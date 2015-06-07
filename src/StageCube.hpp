#pragma once

//
// Stageを構成するCube
//

#include "cinder/Tween.h"
#include <boost/noncopyable.hpp>


namespace ngs {

struct StageCube {
  ci::Anim<ci::Vec3f> position;
  ci::Anim<ci::Quatf> rotation;

  ci::Vec3i block_position;
  // Switchでの移動用
  ci::Vec3i block_position_new;
  
  ci::Color color;

  bool can_ride;
  bool active;
};

}
