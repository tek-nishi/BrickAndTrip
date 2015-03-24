#pragma once

//
// 舞台に登場する立方体の定義
//

#include "cinder/Tween.h"


namespace ngs {

struct Cube {
  ci::Anim<ci::Vec3f> position;
  ci::Anim<ci::Quatf> rotation;
  float size;

  ci::Vec3i block_position;
  
  ci::Color color;

  bool can_ride;
  bool active;
};

}
