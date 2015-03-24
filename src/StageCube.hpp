#pragma once

//
// Stageを構成するCube
//

#include "cinder/Tween.h"


namespace ngs {

struct StageCube {
  ci::Anim<ci::Vec3f> position;
  ci::Anim<ci::Quatf> rotation;
  float cube_size;

  ci::Vec3i block_position;
  
  ci::Color color;

  bool can_ride;
  bool active;

  ci::Vec3f size() const { return ci::Vec3f(cube_size, cube_size, cube_size); }
};

}
