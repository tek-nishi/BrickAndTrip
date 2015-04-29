#pragma once

//
// プレイ記録
// 

#include <vector>
#include <boost/noncopyable.hpp>
#include "FileUtil.hpp"


namespace ngs {

class Records : private boost::noncopyable {
public:
  struct StageRecord {
    double clear_time;
    int    tumble_num;
    int    item_num;
    int    operation_num;
    int    score;

    StageRecord() :
      clear_time(0.0),
      tumble_num(0),
      item_num(0),
      operation_num(0),
      score()
    {}
  };

  struct CurrentGame {
    double start_time;
    double play_time;

    int tumble_num;
    int item_num;
    int operation_num;

    CurrentGame() :
      start_time(0.0),
      play_time(0.0),
      tumble_num(0),
      item_num(0),
      operation_num(0)
    {}
  };
  
  CurrentGame current_game;

  
private:
  int    total_play_num_;
  double total_play_time_;
  int    total_tumble_num_;
  int    total_item_num_;
  int    total_operation_num_;
  int    total_clear_num_;

  int    current_stage_;
  double current_play_time_;

  std::vector<StageRecord> stage_records_;

  bool se_on_;
  bool bgm_on_;
  

public:
  Records() :
    total_play_num_(0),
    total_play_time_(0.0),
    total_tumble_num_(0),
    total_item_num_(0),
    total_operation_num_(0),
    total_clear_num_(0),
    current_stage_(0),
    current_play_time_(0.0),
    se_on_(true),
    bgm_on_(true)
  { }


  // 最初のステージでのみ必要な初期化
  void prepareGameRecord() {
    current_play_time_ = 0.0;
  }

  void prepareCurrentGameRecord(const int stage_num, const double current_time) {
    current_stage_ = stage_num;

    current_game = CurrentGame();
    current_game.start_time = current_time;
  }
  
  bool isFirstCleard() const {
    return current_stage_ == stage_records_.size();
  }

  int cleardStageNum() {
    return int(stage_records_.size());
  }

  void storeStageRecord(const double current_time) {
    StageRecord record;

    double play_time = current_time - current_game.start_time;
    record.clear_time    = play_time;
    record.tumble_num    = current_game.tumble_num;
    record.item_num      = current_game.item_num;
    record.operation_num = current_game.operation_num;

    // TODO:score計算
    // record.score = 0;
    
    if (isFirstCleard()) {
      stage_records_.push_back(record);
    }
    else {
      // TODO:上書きする条件を定義
      stage_records_[current_stage_] = record;
    }

    current_play_time_ += play_time;
    
    total_play_time_     += record.clear_time;
    total_tumble_num_    += current_game.tumble_num;
    total_item_num_      += current_game.item_num;
    total_operation_num_ += current_game.operation_num;
  }

  
  void storeRecord(const double current_time) {
    double play_time = current_time - current_game.start_time;

    current_play_time_ += play_time;

    // TIPS:GameOverでも記録が伸びる親切設計
    total_play_time_     += play_time;
    total_tumble_num_    += current_game.tumble_num;
    total_item_num_      += current_game.item_num;
    total_operation_num_ += current_game.operation_num;
    total_play_num_      += 1;
  }

  // クリア回数
  void cleardAllStage() {
    total_clear_num_ += 1;
  }

  
  void load(const std::string& path) {
    auto full_path = getDocumentPath() / path;
    if (!ci::fs::is_regular_file(full_path)) return;

    DOUT << "record loaded." << std::endl;
    DOUT << full_path << std::endl;
    
    ci::JsonTree record = ci::JsonTree(ci::loadFile(full_path));

    total_play_num_      = record["records.total_play_num"].getValue<int>();
    total_play_time_     = record["records.total_play_time"].getValue<double>();
    total_tumble_num_    = record["records.total_tumble_num"].getValue<int>();
    total_item_num_      = record["records.total_item_num"].getValue<int>();
    total_operation_num_ = record["records.total_operation_num"].getValue<int>();
    total_clear_num_     = record["records.total_clear_num"].getValue<int>();

    se_on_  = record["records.se_on"].getValue<bool>();
    bgm_on_ = record["records.bgm_on"].getValue<bool>();

    if (record.hasChild("records.stage")) {
      const auto& stage = record["records.stage"];
      for (const auto& sr : stage) {
        StageRecord s;

        s.clear_time    = sr["clear_time"].getValue<double>();
        s.tumble_num    = sr["tumble_num"].getValue<int>();
        s.item_num      = sr["item_num"].getValue<int>();
        s.operation_num = sr["operation_num"].getValue<int>();
        s.score         = sr["score"].getValue<int>();

        stage_records_.push_back(std::move(s));
      }
    }
  }
  
  void write(const std::string& path) {
    ci::JsonTree record = ci::JsonTree::makeObject("records");

    record.addChild(ci::JsonTree("total_play_num", total_play_num_))
      .addChild(ci::JsonTree("total_play_time", total_play_time_))
      .addChild(ci::JsonTree("total_tumble_num", total_tumble_num_))
      .addChild(ci::JsonTree("total_item_num", total_item_num_))
      .addChild(ci::JsonTree("total_operation_num", total_operation_num_))
      .addChild(ci::JsonTree("total_clear_num", total_clear_num_))
      .addChild(ci::JsonTree("se_on", se_on_))
      .addChild(ci::JsonTree("bgm_on", bgm_on_));

    if (!stage_records_.empty()) {
      ci::JsonTree stage = ci::JsonTree::makeArray("stage");
      for (const auto& s : stage_records_) {
        ci::JsonTree sr;
        sr.addChild(ci::JsonTree("clear_time", s.clear_time))
          .addChild(ci::JsonTree("tumble_num", s.tumble_num))
          .addChild(ci::JsonTree("item_num", s.item_num))
          .addChild(ci::JsonTree("operation_num", s.operation_num))
          .addChild(ci::JsonTree("score", s.score));

        stage.pushBack(sr);
      }
      
      record.addChild(stage);
    }

    auto full_path = getDocumentPath() / path;
    DOUT << "record writed. " << full_path << std::endl;
    record.write(full_path, ci::JsonTree::WriteOptions().createDocument(true));
  }

  double getCurrentGamePlayTime() const { return current_play_time_; }

  int getTotalPlayNum() const { return total_play_num_; }
  double getTotalPlayTime() const { return total_play_time_; }
  int getTotalTumbleNum() const { return total_tumble_num_; }
  int getTotalItemNum() const { return total_item_num_; }
  int getTotalOperationNum() const { return total_operation_num_; }
  int getTotalClearNum() const { return total_clear_num_; }

  bool isSeOn() const { return se_on_; }
  bool isBgmOn() const { return bgm_on_; }
  
  bool toggleSeOn() {
    se_on_ = !se_on_;
    return se_on_;
  }
  
  bool toggleBgmOn() {
    bgm_on_ = !bgm_on_;
    return bgm_on_;
  }
  
};

}
