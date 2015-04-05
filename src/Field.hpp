#pragma once

//
// Fieldの定義
//

#include <deque>
#include <vector>
#include "StageCube.hpp"
#include "PickableCube.hpp"
#include "ItemCube.hpp"


namespace ngs {

struct Field {
  const std::deque<std::vector<StageCube> >& active_cubes;
  const std::deque<std::vector<StageCube> >& collapse_cubes;

  // VS2013には暗黙のmoveコンストラクタが無いのでstd::unique_ptrで保持
  // ※自前で用意するのではなくコンパイラ任せにしたい
  const std::vector<std::unique_ptr<PickableCube> >& pickable_cubes;

  const std::vector<std::unique_ptr<ItemCube> >& item_cubes;
};

}
