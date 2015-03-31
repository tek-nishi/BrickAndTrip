#pragma once

//
// ゲーム舞台のEntity
//

#include <sstream>
#include <iomanip>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/optional.hpp>
#include "cinder/Json.h"
#include "cinder/Timeline.h"
#include "cinder/Rand.h"
#include "Stage.hpp"
#include "Field.hpp"
#include "PickableCube.hpp"
#include "EventParam.hpp"
#include "Records.hpp"


namespace ngs {

class FieldEntity {
  const ci::JsonTree& params_;
  Event<EventParam>& event_;

  std::vector<ci::Color> cube_stage_color_;
  ci::Color cube_line_color_;

  int stage_num_;
  Stage stage_;
  
  // VS2013には暗黙のmoveコンストラクタが無いのでstd::unique_ptrで保持
  // std::vectorに格納するときに、copyやmoveコンストラクタが呼ばれる
  using PickableCubePtr = std::unique_ptr<PickableCube>;
  std::vector<PickableCubePtr> pickable_cubes_;

  Records records_;

  int start_line_z_;
  int finish_line_z_;
  int next_start_line_z_;

  int entry_packable_num_;
  
  enum {
    NONE,
    START,
    FINISH,
    CLEAR,
    CLEANUP,
  };
  int mode_;
  bool in_game_;

  
  ci::TimelineRef event_timeline_;

  
public:
  FieldEntity(const ci::JsonTree& params, Event<EventParam>& event) :
    params_(params),
    event_(event),
    cube_line_color_(Json::getColor<float>(params["game.cube_line_color"])),
    stage_num_(0),
    stage_(params, event_),
    mode_(NONE),
    in_game_(false),
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
    records_.current_game.play_time += progressing_seconds;
    
    boost::remove_erase_if(pickable_cubes_,
                           [](const PickableCubePtr& cube) {
                             return !cube->isActive();
                           });

    decideEachPickableCubeFalling(in_game_);
    decideEachPickableCubeMoving();

#if 0
    // 全PickableCubeの落下判定は、毎フレーム判定を避けている
    if (did_fall && !isPickableCubeOnStage()) {
      event_.signal("fall-all-pickable", EventParam());
    }
#endif

    switch (mode_) {
    case START:
      // 全PickableCubeのstart判定
      if (isAllPickableCubesStarted()) {
        mode_ = FINISH;
        event_.signal("all-pickable-started", EventParam());
      }
      break;

    case FINISH:
      // 全PickableCubeのfinish判定
      if (isAllPickableCubesFinished()) {
        mode_ = CLEAR;
        event_.signal("all-pickable-finished", EventParam());
      }
      break;

    case CLEAR:
      // finish後、stageの生成と崩壊の完了判定
      if (stage_.isFinishedBuildAndCollapse()) {
        mode_ = NONE;
        event_.signal("stage-cleared", EventParam());
      }
      break;

    case CLEANUP:
      // gameover後、stage崩壊の完了判定
      if (stage_.isFinishedCollapse()) {
        mode_ = NONE;
        event_.signal("stage-all-collapsed", EventParam());
      }
      break;
    }
  }


  // 最初のStageとStartLine、PickableCubeを準備
  void setupStartStage() {
    auto stage_info = addCubeStage("startline.json");
    next_start_line_z_ = stage_info.first - 1;
    entry_packable_num_ = stage_info.second;
    
    stage_.buildStage(0.25f);

    int   entry_z = 0 + params_["game.pickable.entry_z"].getValue<int>();
    float delay   = params_["game.pickable.entry_start_delay"].getValue<float>();
    for (int i = 0; i < entry_packable_num_; ++i) {
      entryPickableCube(entry_z, delay);
    }
  }

  // Stageの全Buildを始める
  void startStageBuild() {
    stage_.openStartLine();
    
    std::ostringstream path;
    // stage_num が 0 -> stage01.json 
    path << "stage" << std::setw(2) << std::setfill('0') << (stage_num_ + 1) << ".json";
    auto stage_info = addCubeStage(path.str());
    finish_line_z_ = stage_info.first - 1;
    entry_packable_num_ = stage_info.second;
    stage_.setFinishLine(finish_line_z_);

    start_line_z_ = next_start_line_z_;
    stage_info = addCubeStage("finishline.json");
    next_start_line_z_ = stage_info.first - 1;

    mode_    = START;
    in_game_ = true;

    records_.prepareCurrentGameRecord(stage_num_, event_timeline_->getCurrentTime());

    stage_num_ += 1;

    stage_.buildStage();
  }

  // StageのFinishLineまでの崩壊を始める
  void startStageCollapse() {
    stage_.collapseStage(finish_line_z_ - 1);
  }
  
  // FinishLineまで一気に崩壊 && FinishLineを一気に生成
  void completeBuildAndCollapseStage() {
    stage_.stopBuildAndCollapse();
    stage_.buildStage(0.1);
    stage_.collapseStage(finish_line_z_ - 1, 0.1);

    records_.storeStageRecord(event_timeline_->getCurrentTime());

    {
      EventParam params = {
        { "clear_time", records_.current_game.play_time },
        { "tumble_num", records_.current_game.tumble_num },
      };
      event_.signal("begin-stageclear", params);
    }
    
    // sleep中のPickableCubeを起こす
    for (auto& cube : pickable_cubes_) {
      cube->awaken();
    }
  }

  // GameOver時の処理
  void gameover() {
    records_.storeRecord(event_timeline_->getCurrentTime());
    stopBuildAndCollapse();
    in_game_ = false;

    {
      EventParam params = {
        { "clear_time", records_.current_game.play_time },
        { "tumble_num", records_.current_game.tumble_num },
      };
      event_.signal("begin-gameover", params);
    }
  }

  // GameOver時などで生成&崩壊を止める
  void stopBuildAndCollapse() {
    stage_.stopBuildAndCollapse();
    mode_    = NONE;
    in_game_ = false;
  }
  
  // リスタート前のClean-up
  void cleanupField() {
    event_timeline_->clear();
    
    stage_.stopBuildAndCollapse();
    stage_.collapseStage(next_start_line_z_, 0.1);
    mode_ = CLEANUP;
  }
  
  void restart() {
    stage_.restart();
    stage_num_ = 0;
    mode_      = NONE;
    in_game_   = false;
  }

  
  void movePickableCube(const u_int id, const int direction, const int speed) {
    auto it = findPickableCube(id);
    assert(it);
    auto& cube = *it;

    // 移動可能かStageを調べる
    static ci::Vec3i move_vec[] = {
      {  0, 0,  1 },
      {  0, 0, -1 },
      {  1, 0,  0 },
      { -1, 0,  0 },
    };

    const auto& move_vector = move_vec[direction];
    auto moved_pos = cube->blockPosition() + move_vector;
    if (canPickableCubeMove(cube, moved_pos)) {
      cube->reserveRotationMove(direction, move_vector, speed);
    }
  }

  void movedPickableCube() {
    records_.current_game.tumble_num += 1;
  }

  // finish-line上のPickableCubeを生成
  void entryPickableCubes() {
    if (entry_packable_num_ == 0) return;

    // Finish lineの次が(z = 0)として生成
    int entry_z = finish_line_z_ + 1 + params_["game.pickable.entry_z"].getValue<int>();
    float delay = params_["game.pickable.entry_next_delay"].getValue<float>();
    for (int i = 0; i < entry_packable_num_; ++i) {
      entryPickableCube(entry_z, delay, true);
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

  
private:
  // TODO:参照の無効値をあらわすためにboost::optionalを利用
  boost::optional<PickableCubePtr&> findPickableCube(const u_int id) {
    auto it = std::find_if(std::begin(pickable_cubes_), std::end(pickable_cubes_),
                           [id](const PickableCubePtr& obj) {
                             return *obj == id;
                           });

    if (it == std::end(pickable_cubes_)) return boost::optional<PickableCubePtr&>();
    
    return boost::optional<PickableCubePtr&>(*it);
  }


  void entryPickableCube(const int entry_z, const float delay, const bool sleep = false) {
    event_timeline_->add([this, entry_z, sleep]() {
        while (1) {
          // Stageは(x >= 0)を保証しているので手抜きできる
          auto entry_pos = ci::Vec3i(ci::randInt(1, 8), 0, entry_z);
          if (isPickableCube(entry_pos)) continue;

          
          auto cube = PickableCubePtr(new PickableCube(params_, event_, entry_pos,
                                                       (mode_ == CLEAR) ? false : sleep));
          pickable_cubes_.push_back(std::move(cube));
          return;
        }
        
      }, event_timeline_->getCurrentTime() + delay);
  }
  
  std::pair<int, int> addCubeStage(const std::string& path) {
    auto stage = ci::JsonTree(ci::app::loadAsset(path));
    int top_z = stage_.addCubes(stage,
                                cube_stage_color_, cube_line_color_);

    int entry_num = Json::getValue(stage, "pickable", 0);

    return std::make_pair(top_z, entry_num);
  }


  // 全てのPickableCubeの移動開始
  void decideEachPickableCubeMoving() {
    for (auto& cube : pickable_cubes_) {
      if (cube->willRotationMove()) {
        if (canPickableCubeMove(cube, cube->blockPosition() + cube->moveVector())) {
          cube->startRotationMove();
        }
        else {
          cube->cancelRotationMove();
        }
      }
    }
  }

  bool canPickableCubeMove(const PickableCubePtr& cube, const ci::Vec3i& block_pos) {
    // 移動先に他のPickableCubeがいたら移動できない
    for (auto& other_cube : pickable_cubes_) {
      if (*cube == *other_cube) continue;

      if (block_pos == other_cube->blockPosition()) return false;
    }

    // 移動先の高さが同じじゃないと移動できない
    auto height = stage_.getStageHeight(block_pos);
    return height.first && (height.second == block_pos.y);
  }

  bool isPickableCube(const ci::Vec3i& block_pos) {
    for (auto& cube : pickable_cubes_) {
      const auto& pos = cube->blockPosition();
      
      if ((pos.x == block_pos.x) && (pos.z == block_pos.z)) {
        return true;
      }
    }
    return false;
  }
  
  
  // 全PickableCubeの落下判定
  void decideEachPickableCubeFalling(const bool signal_event) {
    for (auto& cube : pickable_cubes_) {
      if (!cube->isOnStage() || !cube->canPick()) continue;
      
      auto height = stage_.getStageHeight(cube->blockPosition());
      if (!height.first) {
        cube->fallFromStage();

        if (!cube->isSleep()) {
          if (signal_event) {
            EventParam params = {
              { "id", cube->id() },
            };
            event_.signal("fall-pickable", params);
          }
        }
      }
    }
  }

  
  // １つでもPickableCubeがStage上にいるか判定
  bool isPickableCubeOnStage() {
    bool on_stage = false;
    for (auto& cube : pickable_cubes_) {
      // sleep中なのは勘定しない
      if (cube->isSleep()) continue;
      
      if (cube->isOnStage()) {
        on_stage = true;
        break;
      }
    }
    
    return on_stage;
  }
  
  // すべてのPickableCubeがStartしたか判定
  bool isAllPickableCubesStarted() {
    if (pickable_cubes_.empty()) return false;

    bool started = true;
    for (const auto& cube : pickable_cubes_) {
      if (!cube->canPick()) {
        // FIXME:操作できないPickableCubeがあったらfalse
        started = false;
        break;
      }

      if (cube->blockPosition().z < start_line_z_) {
        // StartできていないPickableCubeが1つでもあればfalse
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
      if (cube->isSleep()) continue;
      
      if (!cube->canPick()) {
        finished = false;
        break;
      }

      if (cube->blockPosition().z < finish_line_z_) {
        // FinishできていないPickableCubeが1つでもあればfalse
        finished = false;
        break;
      }
    }
    
    return finished;
  }
  
};

}
