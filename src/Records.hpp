#pragma once

//
// プレイ記録
// 

#include <vector>


namespace ngs {

class Records {
public:
  struct StageRecord {
    double clear_time;
    int    tumble_num;
    int    score;
  };

  struct CurrentGame {
    double play_time;
    int    tumble_num;

    StageRecord current_stage;
  };
  
  CurrentGame current_game;

  
private:
  int    total_play_num_;
  double total_play_time_;
  int    total_tumble_num_;

  int current_stage_;
  double current_game_start_time_;

  std::vector<StageRecord> stage_records_;
  

public:
  Records() :
    total_play_num_(0),
    total_play_time_(0.0),
    total_tumble_num_(0),
    current_stage_(0)
  {
    current_game.play_time  = 0.0;
    current_game.tumble_num = 0;
  }


  void prepareCurrentGameRecord(const int stage_num, const double current_time) {
    current_game.play_time  = 0.0;
    current_game.tumble_num = 0;

    current_stage_           = stage_num;
    current_game_start_time_ = current_time;

    StageRecord record = {
      0.0,
      0,
      0
    };
    current_game.current_stage = record;
  }
  
  bool isFirstCleard() const {
    return current_stage_ == stage_records_.size();
  }

  int cleardStageNum() {
    return stage_records_.size();
  }

  void storeStageRecord(const double current_time) {
    current_game.play_time = current_time - current_game_start_time_;
    
    current_game.current_stage.clear_time = current_game.play_time;
    current_game.current_stage.tumble_num = current_game.tumble_num;
    
    if (isFirstCleard()) {
      stage_records_.push_back(current_game.current_stage);
    }
    else {
      stage_records_[current_stage_] = current_game.current_stage;
    }

    total_play_time_  += current_game.current_stage.clear_time;
    total_tumble_num_ += current_game.current_stage.tumble_num;
  }

  
  void storeRecord(const double current_time) {
    current_game.play_time = current_time - current_game_start_time_;

    // TIPS:GameOverでも記録が伸びる親切設計
    total_play_time_  += current_game.current_stage.clear_time;
    total_tumble_num_ += current_game.current_stage.tumble_num;
  }
  

  void countupPlayNum() {
    total_play_num_ += 1;
  }


  void write(const std::string& path) {
    ci::JsonTree record = ci::JsonTree::makeObject("records");

    record.addChild(ci::JsonTree("total_play_num", total_play_num_))
      .addChild(ci::JsonTree("total_play_time", total_play_time_))
      .addChild(ci::JsonTree("total_tumble_num", total_tumble_num_));

    if (!stage_records_.empty()) {
      ci::JsonTree stage = ci::JsonTree::makeArray("stage");
      for (const auto& s : stage_records_) {
        ci::JsonTree sr;
        sr.addChild(ci::JsonTree("clear_time", s.clear_time))
          .addChild(ci::JsonTree("tumble_num", s.tumble_num))
          .addChild(ci::JsonTree("score", s.score));

        stage.pushBack(sr);
      }
      
      record.addChild(stage);
    }
    
    record.write(ci::app::getAppPath() / path, ci::JsonTree::WriteOptions().createDocument(true));
  }
  
};

}
