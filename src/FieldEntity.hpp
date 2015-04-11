#pragma once

//
// ゲーム舞台のEntity
//

#include <sstream>
#include <iomanip>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>
#include "cinder/Json.h"
#include "cinder/Timeline.h"
#include "cinder/Rand.h"
#include "Stage.hpp"
#include "Field.hpp"
#include "PickableCube.hpp"
#include "StageItems.hpp"
#include "EventParam.hpp"
#include "Records.hpp"
#include "Bg.hpp"


namespace ngs {

class FieldEntity : private boost::noncopyable {
  ci::JsonTree& params_;
  ci::TimelineRef timeline_;
  Event<EventParam>& event_;
  Records& records_;

  std::vector<ci::Color> cube_stage_color_;
  ci::Color cube_line_color_;
  ci::Color stage_color_;

  float finish_rate_;

  int total_stage_num_;
  int stage_num_;
  Stage stage_;

  StageItems items_;

  Bg bg_;
  
  // VS2013には暗黙のmoveコンストラクタが無いのでstd::unique_ptrで保持
  // std::vectorに格納するときに、copyやmoveコンストラクタが呼ばれる
  using PickableCubePtr = std::unique_ptr<PickableCube>;
  std::vector<PickableCubePtr> pickable_cubes_;

  bool record_play_;

  bool first_started_pickable_;
  bool first_fallen_pickable_;
  
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

  bool all_cleard_;
  bool game_aborted_;
  
  ci::TimelineRef event_timeline_;


  struct StageInfo {
    int top_z;
    int entry_num;
    ci::Color stage_color;
  };
  
  
public:
  FieldEntity(ci::JsonTree& params,
              ci::TimelineRef timeline,
              Event<EventParam>& event,
              Records& records) :
    params_(params),
    timeline_(timeline),
    event_(event),
    records_(records),
    cube_line_color_(ci::hsvToRGB(Json::getHsvColor(params["game.cube_line_color"]))),
    stage_color_(ci::hsvToRGB(Json::getHsvColor(params["game.stage_color"]))),
    stage_num_(0),
    stage_(params, timeline, event),
    items_(params, timeline, event),
    bg_(params, timeline, event),
    record_play_(false),
    first_started_pickable_(false),
    first_fallen_pickable_(false),
    finish_rate_(params["game.finish_rate"].getValue<float>()),
    total_stage_num_(params["game.total_stage_num"].getValue<int>()),
    mode_(NONE),
    all_cleard_(false),
    game_aborted_(false),
    event_timeline_(ci::Timeline::create())
  {
    const auto& colors = params["game.cube_stage_color"];
    for (const auto& color : colors) {
      cube_stage_color_.push_back(Json::getColor<float>(color));
    }

    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    timeline->apply(event_timeline_);
  }

  ~FieldEntity() {
    // 再生途中のものもあるので、手動で取り除く
    event_timeline_->removeSelf();
  }


  void update(const double progressing_seconds) {
    if (record_play_) {
      records_.current_game.play_time += progressing_seconds;
    }

    decideEachPickableCubeFalling();
    boost::remove_erase_if(pickable_cubes_,
                           [](const PickableCubePtr& cube) {
                             return !cube->isActive();
                           });

    decideEachPickableCubeMoving();

    items_.update(stage_);

#if 0
    // 全PickableCubeの落下判定は、毎フレーム判定を避けている
    if (did_fall && !isPickableCubeOnStage()) {
      event_.signal("fall-all-pickable", EventParam());
    }
#endif

    switch (mode_) {
    case START:
      // PickableCubeのstart判定
      if (isPickableCubeStarted()) {
        mode_ = FINISH;
        first_started_pickable_ = true;

        EventParam params = {
          { "stage_color", stage_color_ },
        };
        event_.signal("first-pickable-started", params);
      }
      break;

    case FINISH:
      // 全PickableCubeのfinish判定
      // FIXME:event処理で判定できる
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

        EventParam params = {
          { "all_cleard",   all_cleard_ },
          { "game_aborted", game_aborted_ },
        };
        event_.signal("stage-all-collapsed", params);
      }
      break;
    }

    {
      EventParam params = {
        { "play-time", records_.current_game.play_time },
        { "tumble-num", records_.current_game.tumble_num },
        { "item-num", records_.current_game.item_num },
        { "operation_num", records_.current_game.operation_num },
      };
      event_.signal("update-record", params);
    }
  }


  // 最初のStageとStartLine、PickableCubeを準備
  void setupStartStage() {
    auto stage_info = addCubeStage("startline.json");
    next_start_line_z_ = stage_info.top_z - 1;
    entry_packable_num_ = stage_info.entry_num;
    
    stage_.buildStage();

    int   entry_z = 0 + params_["game.pickable.entry_z"].getValue<int>();
    float delay   = params_["game.pickable.entry_start_delay"].getValue<float>();
    for (int i = 0; i < entry_packable_num_; ++i) {
      entryPickableCube(entry_z, delay);
    }
    
    stage_num_    = 0;
    all_cleard_   = false;
    game_aborted_ = false;

    records_.prepareGameRecord();
  }

  // Stageの全Buildを始める
  void startStageBuild() {
    stage_.openStartLine();
    items_.clear();
    
    std::ostringstream path;
    // stage_num が 0 -> stage01.json 
    path << "stage" << std::setw(2) << std::setfill('0') << (stage_num_ + 1) << ".json";
    auto stage_info = addCubeStage(path.str());
    finish_line_z_ = stage_info.top_z - 1;
    entry_packable_num_ = stage_info.entry_num;
    stage_.setFinishLine(finish_line_z_);
    stage_color_ = stage_info.stage_color;

    start_line_z_ = next_start_line_z_;
    stage_info = addCubeStage("finishline.json");
    next_start_line_z_ = stage_info.top_z - 1;

    mode_ = START;
    first_started_pickable_ = false;
    first_fallen_pickable_  = false;

    records_.prepareCurrentGameRecord(stage_num_, event_timeline_->getCurrentTime());
    enableRecordPlay(false);

    stage_num_ += 1;

    stage_.buildStage();
    stage_.setupAutoCollapse(finish_line_z_ - 1);
  }

  // StageのFinishLineまでの崩壊を始める
  void startStageCollapse() {
    if (!stage_.isStartedCollapse()) {
      stage_.collapseStage(finish_line_z_ - 1);
    }
  }
  
  // FinishLineまで一気に崩壊 && FinishLineを一気に生成
  void completeBuildAndCollapseStage() {
    stage_.stopBuildAndCollapse();
    stage_.buildStage(finish_rate_);
    stage_.collapseStage(finish_line_z_ - 1, finish_rate_);

    enableRecordPlay(false);

    records_.storeStageRecord(event_timeline_->getCurrentTime());
    // 全ステージクリア判定
    all_cleard_ = stage_num_ == total_stage_num_;
    if (all_cleard_) {
      records_.cleardAllStage();
    }
    
    records_.write(params_["game.records"].getValue<std::string>());

    {
      // 次のステージ or 全ステージクリア
      EventParam params = {
        { "clear_time", records_.current_game.play_time },
        { "tumble_num", records_.current_game.tumble_num },
        { "item_num", records_.current_game.item_num },
        { "operation_num", records_.current_game.operation_num },
        { "play_time", records_.getCurrentGamePlayTime() },
        { "all_cleared", all_cleard_ },
      };

      event_.signal("begin-stageclear", params);
    }
    
    // sleep中のPickableCubeを起こす
    for (auto& cube : pickable_cubes_) {
      if (cube->isSleep()) {
        cube->awaken();
        cube->endSleepingColor();
      }
    }
  }

  // GameOver時の処理
  void gameover() {
    stopBuildAndCollapse();
    enableRecordPlay(false);
    records_.storeRecord(event_timeline_->getCurrentTime());
    records_.write(params_["game.records"].getValue<std::string>());

    {
      EventParam params = {
        { "clear_time", records_.current_game.play_time },
        { "tumble_num", records_.current_game.tumble_num },
        { "operation_num", records_.current_game.operation_num },
        { "play_time", records_.getCurrentGamePlayTime() },
      };
      event_.signal("begin-gameover", params);
    }
  }

  // GameOver時などで生成&崩壊を止める
  void stopBuildAndCollapse() {
    stage_.stopBuildAndCollapse();
    mode_ = NONE;
  }

  // 中断
  void abortGame() {
    game_aborted_ = true;
    cleanupField();
  }
  
  // リスタート前のClean-up
  void cleanupField() {
    event_timeline_->clear();

    first_started_pickable_ = true;
    first_fallen_pickable_  = true;
    
    stage_.stopBuildAndCollapse();
    stage_.collapseStage(next_start_line_z_, finish_rate_);
    mode_ = CLEANUP;
  }
  
  void restart() {
    stage_.restart();
    mode_ = NONE;
  }


  void pickPickableCube(const u_int id) {
    auto it = findPickableCube(id);
    assert(it);
    auto& cube = *it;

    cube->startPickingColor();
  }

  void movePickableCube(const u_int id, const int direction, const int speed) {
    auto it = findPickableCube(id);
    assert(it);
    auto& cube = *it;

    cube->endPickingColor();

    // 動こうが動くまいが、操作はカウント
    records_.current_game.operation_num += 1;
    
    if ((direction == PickableCube::MOVE_NONE) || !speed) {
      cube->cancelRotationMove();
      return;
    }

    
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

  void cancelPickPickableCubes() {
    for (auto& cube : pickable_cubes_) {
      cube->endPickingColor();
    }
  }

  void movedPickableCube() {
    if (!record_play_) return;
    
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


  void entryItemCubes(const int active_z) {
    items_.entryItemCube(active_z);
  }

  void pickupedItemCube() {
    records_.current_game.item_num += 1;
  }

  // PickableCubeのIdle
  void startIdlePickableCube(const u_int id) {
    auto it = findPickableCube(id);
    if (!it) return;

    auto& cube = *it;
    const ci::Vec3i block_pos = cube->blockPosition();

    std::vector<int> directions;

    static ci::Vec3i move_vec[] = {
      {  0, 0,  1 },
      {  0, 0, -1 },
      {  1, 0,  0 },
      { -1, 0,  0 },
    };
    
    for (u_int i = 0; i < elemsof(move_vec); ++i) {
      if (!isPickableCube(block_pos + move_vec[i])) {
        directions.push_back(i);
      }
    }

    if (directions.empty()) return;

    cube->startIdleMotion(directions);
  }
  
  // プレイ中の情報を記録に取る
  void enableRecordPlay(const bool enable = true) {
    record_play_ = enable;
  }
  
  // 現在のFieldの状態を作成
  Field fieldData() {
    Field field = {
      stage_.activeCubes(),
      stage_.collapseCubes(),
      pickable_cubes_,
      items_.items()
    };

    return field;
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

          
          auto cube = PickableCubePtr(new PickableCube(params_, timeline_, event_, entry_pos,
                                                       (mode_ == CLEAR) ? false : sleep));
          pickable_cubes_.push_back(std::move(cube));
          return;
        }
        
      }, event_timeline_->getCurrentTime() + delay);
  }
  
  StageInfo addCubeStage(const std::string& path) {
    auto stage = ci::JsonTree(ci::app::loadAsset(path));
    int current_z = stage_.getTopZ();

    int x_offset = Json::getValue(stage, "x_offest", 0);
    int top_z = stage_.addCubes(stage,
                                x_offset,
                                cube_stage_color_, cube_line_color_);

    int entry_num = Json::getValue(stage, "pickable", 0);

    items_.addItemCubes(stage, current_z, x_offset);

    StageInfo info = {
      top_z,
      entry_num,
      ci::hsvToRGB(Json::getHsvColor(stage["color"])),
    };
    
    return info;
  }


  // 全てのPickableCubeの移動開始
  void decideEachPickableCubeMoving() {
    for (auto& cube : pickable_cubes_) {
      if (cube->willRotationMove()) {
        if (canPickableCubeMove(cube, cube->blockPosition() + cube->moveVector())) {
          cube->startRotationMove();
          // 回転開始時にitem pickup判定
          pickupStageItems(cube);
        }
        else {
          cube->cancelRotationMove();
        }
      }
    }
  }

  void pickupStageItems(const PickableCubePtr& cube) {
    auto result = items_.canGetItemCube(cube->blockPosition());
    if (result.first) {
      items_.pickupItemCube(result.second);
    }
  }

  bool canPickableCubeMove(const PickableCubePtr& cube, const ci::Vec3i& block_pos) const {
    // 移動先に他のPickableCubeがいたら移動できない
    for (auto& other_cube : pickable_cubes_) {
      if (*cube == *other_cube) continue;

      if (block_pos == other_cube->blockPosition()) return false;
    }

    // 移動先の高さが同じじゃないと移動できない
    auto height = stage_.getStageHeight(block_pos);
    return height.first && (height.second == block_pos.y);
  }

  bool isPickableCube(const ci::Vec3i& block_pos) const {
    for (auto& cube : pickable_cubes_) {
      const auto& pos = cube->blockPosition();
      
      if ((pos.x == block_pos.x) && (pos.z == block_pos.z)) {
        return true;
      }
    }
    return false;
  }
  
  // PickableCubeの落下判定
  void decideEachPickableCubeFalling() {
    for (auto& cube : pickable_cubes_) {
      if (!cube->isOnStage() || cube->isMoving()) continue;
      
      auto height = stage_.getStageHeight(cube->blockPosition());
      if (!height.first) {
        cube->endPickingColor();
        cube->fallFromStage();

        if (!cube->isSleep()) {
          EventParam params = {
            { "id", cube->id() },
          };
          event_.signal("fall-pickable", params);

          if (!first_fallen_pickable_) {
            first_fallen_pickable_ = true;
            event_.signal("first-fallen-pickable", EventParam());
          }
        }
      }
    }
  }

  
  // １つでもPickableCubeがStage上にいるか判定
  bool isPickableCubeOnStage() const {
    for (auto& cube : pickable_cubes_) {
      // sleep中なのは勘定しない
      if (cube->isSleep()) continue;
      
      if (cube->isOnStage()) {
        return true;
      }
    }
    
    return false;
  }

  
  bool isPickableCubeStarted() const {
    if (first_started_pickable_ || pickable_cubes_.empty()) return false;

    for (const auto& cube : pickable_cubes_) {
      if (cube->isSleep()) continue;

      // 移動中の場合は移動前の位置で判定
      const auto& position = cube->isMoving() ? cube->prevBlockPosition()
                                              : cube->blockPosition();
      
      if (position.z >= start_line_z_) {
        return true;
      }
    }
    return false;
  }

#if 0
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
#endif

  // すべてのPickableCubeがFinishしたか判定
  bool isAllPickableCubesFinished() {
    if (pickable_cubes_.empty()) return false;

    bool finished = true;
    for (const auto& cube : pickable_cubes_) {
      if (cube->isSleep()) continue;
      
      if (!cube->isOnStage()) {
        finished = false;
        break;
      }

      // 移動中の場合は移動前の位置で判定
      const auto& position = cube->isMoving() ? cube->prevBlockPosition()
                                              : cube->blockPosition();

      if (position.z < finish_line_z_) {
        // FinishできていないPickableCubeが1つでもあればfalse
        finished = false;
        break;
      }
    }
    
    return finished;
  }
  
};

}
