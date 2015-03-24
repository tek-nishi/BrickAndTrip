#pragma once

//
// ゲーム舞台のEntity
//

#include "cinder/Json.h"
#include "cinder/Timeline.h"
#include "cinder/Rand.h"
#include "Stage.hpp"
#include "Field.hpp"
#include "PickableCube.hpp"


namespace ngs {

class FieldEntity {
  const ci::JsonTree& params_;
  Event<const std::string>& event_;

  std::vector<ci::Color> cube_stage_color_;
  ci::Color cube_line_color_;
  
  Stage stage_;

  // VS2013には暗黙のmoveコンストラクタが無いのでstd::unique_ptrで保持
  std::vector<std::unique_ptr<PickableCube> > pickable_cubes_;

  ci::TimelineRef event_timeline_;

  
public:
  FieldEntity(const ci::JsonTree& params, Event<const std::string>& event) :
    params_(params),
    event_(event),
    cube_line_color_(Json::getColor<float>(params["game.cube_line_color"])),
    stage_(params),
    event_timeline_(ci::Timeline::create())
  {
    const auto& colors = params["game.cube_stage_color"];
    for (const auto& color : colors) {
      cube_stage_color_.push_back(Json::getColor<float>(color));
    }

    auto current_time = ci::app::timeline().getCurrentTime();
    event_timeline_->setStartTime(current_time);
    ci::app::timeline().apply(event_timeline_);
  }

  ~FieldEntity() {
    // 再生途中のものもあるので、手動で取り除く
    event_timeline_->removeSelf();
  }


  void update(const double progressing_seconds) {
  }
  

  void addCubeStage(const std::string& path) {
    auto stage = ci::JsonTree(ci::app::loadAsset(path));
    stage_.addCubes(stage,
                    cube_stage_color_, cube_line_color_);
  }


  void makeStartStage() {
    for (int i = 0; i < 5; ++i) {
      stage_.buildOneLine();
    }
    
    stage_.buildStage();
    // collapseStage();
  }

  void prepareStage() {
    stage_.buildStage(0.25f);

    event_timeline_->add([this]() {
        auto pos = ci::Vec3f(1, 1, 1);
        auto cube = std::unique_ptr<PickableCube>(new PickableCube(params_, pos));
        pickable_cubes_.push_back(std::move(cube));
        
      }, event_timeline_->getCurrentTime() + 3.0f);
  }

  
  // 現在のFieldの状態を作成
  Field fieldData() {
    Field field = {
      stage_.activeCubes(),
      stage_.collapseCubes(),
      pickable_cubes_,
    };

    return std::move(field);
  }
  
};

}
