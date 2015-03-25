#pragma once

//
// ゲーム舞台のEntity
//

#include <sstream>
#include <iomanip>
#include "cinder/Json.h"
#include "cinder/Timeline.h"
#include "cinder/Rand.h"
#include "Stage.hpp"
#include "Field.hpp"
#include "PickableCube.hpp"
#include "EventParam.hpp"


namespace ngs {

class FieldEntity {
  const ci::JsonTree& params_;
  Event<EventParam>& event_;

  std::vector<ci::Color> cube_stage_color_;
  ci::Color cube_line_color_;

  int stage_num_;
  Stage stage_;

  // VS2013には暗黙のmoveコンストラクタが無いのでstd::unique_ptrで保持
  std::vector<std::unique_ptr<PickableCube> > pickable_cubes_;

  int start_line_z_;
  int finish_line_z_;
  int next_start_line_z_;
  
  ci::TimelineRef event_timeline_;

  
public:
  FieldEntity(const ci::JsonTree& params, Event<EventParam>& event) :
    params_(params),
    event_(event),
    cube_line_color_(Json::getColor<float>(params["game.cube_line_color"])),
    stage_num_(0),
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
    for (auto& cube : pickable_cubes_) {
      if (!cube->isOnStage()) continue;
      
      auto height = stage_.getStageHeight(cube->blockPosition());
      if (!height.first) {
        cube->fallFromStage();

        EventParam params = {
          { "id", cube->id() },
        };
        event_.signal("fall-pickable", params);
      }
    }
  }


  // ゲーム開始時のステージを読み込んで生成
  // ゲーム開始時のみ実行
  void setupStartStage() {
    int top_z = addCubeStage("startline.json");
    start_line_z_ = top_z - 1;
    
    stage_.buildStage(0.25f);

    event_timeline_->add([this]() {
        auto pos = ci::Vec3f(1, 0, 1);
        auto cube = std::unique_ptr<PickableCube>(new PickableCube(params_, event_, pos));
        pickable_cubes_.push_back(std::move(cube));
        
      }, event_timeline_->getCurrentTime() + 2.5f);
  }

  void addNextStage() {
    std::ostringstream path;
    path << "stage" << std::setw(2) << std::setfill('0') << (stage_num_ + 1) << ".json";
    
    int top_z = addCubeStage(path.str());
    finish_line_z_ = top_z - 1;

    top_z = addCubeStage("finishline.json");
    next_start_line_z_ = top_z - 1;

    stage_num_ += 1;
  }

  int addCubeStage(const std::string& path) {
    auto stage = ci::JsonTree(ci::app::loadAsset(path));
    return stage_.addCubes(stage,
                           cube_stage_color_, cube_line_color_);
  }

  
  int startLine() const { return start_line_z_; }
  int finishLine() const { return finish_line_z_; }

  
  void buildStage() {
    stage_.buildStage();
  }
  
  void collapseStage(const int collapse_z, const float speed_rate = 1.0f) {
    stage_.collapseStage(collapse_z, speed_rate);
  }

  // すべてのPickableCubeがStartしたか判定
  bool isAllPickableCubesStarted() {
    if (pickable_cubes_.empty()) return false;
    
    bool started = true;
    for (const auto& cube : pickable_cubes_) {
      if (cube->blockPosition().z < start_line_z_) {
        started = false;
        break;
      }
    }
    
    return started;
  }

  // すべてのPickableCubeがFinishしたか判定
  bool isAllPickableCubesFinished() {
    if (pickable_cubes_.empty()) return false;

    bool finished = true;
    for (const auto& cube : pickable_cubes_) {
      if (cube->blockPosition().z < finish_line_z_) {
        finished = false;
        break;
      }
    }
    
    return finished;
  }
  

  void movePickableCube(const u_int id, const int direction) {
    // 複数PickableCubeから対象を探す
    auto it = std::find_if(std::begin(pickable_cubes_), std::end(pickable_cubes_),
                           [id](const std::unique_ptr<PickableCube>& obj) {
                             return *obj == id;
                           });

    assert(it != std::end(pickable_cubes_));
    
    // 移動可能かStageを調べる
    ci::Vec3i move_vec[] = {
      {  0, 0,  1 },
      {  0, 0, -1 },
      {  1, 0,  0 },
      { -1, 0,  0 },
    };

    auto moved_pos = (*it)->blockPosition() + move_vec[direction];
    auto height = stage_.getStageHeight(moved_pos);
    if (height.first && height.second == moved_pos.y) {
      (*it)->startRotationMove(direction, moved_pos);
    }
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
