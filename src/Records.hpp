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

};

}
