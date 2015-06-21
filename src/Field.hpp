#pragma once

//
// Fieldの定義
//

#include <deque>
#include <vector>
#include "StageCube.hpp"
#include "PickableCube.hpp"
#include "ItemCube.hpp"
#include "MovingCube.hpp"
#include "FallingCube.hpp"
#include "Switch.hpp"
#include "Bg.hpp"


namespace ngs {

struct Field {
  const std::deque<std::vector<StageCube> >& active_cubes;
  const std::deque<std::vector<StageCube> >& collapse_cubes;

  // VS2013には暗黙のmoveコンストラクタが無いのでstd::unique_ptrで保持
  // ※自前で用意するのではなくコンパイラ任せにしたい
  const std::vector<std::unique_ptr<PickableCube> >& pickable_cubes;

  const std::vector<std::unique_ptr<ItemCube> >& item_cubes;
  const std::vector<std::unique_ptr<MovingCube> >& moving_cubes;
  const std::vector<std::unique_ptr<FallingCube> >& falling_cubes;
  const std::vector<std::unique_ptr<Switch> >& switches;

  const std::vector<Bg::Cube>& bg_cubes;
  
#ifdef DEBUG
  ci::Vec3f bg_bbox_min;
  ci::Vec3f bg_bbox_max;
#endif
};

}
