#pragma once

//
// プレイ記録
// 

#include <vector>
#include <boost/noncopyable.hpp>
#include "FileUtil.hpp"
#include "TextCodec.hpp"
#include "GameScore.hpp"


namespace ngs {

class Records : private boost::noncopyable {
  enum { RANK_DUMMY = 100 };

  
public:
  struct StageRecord {
    double clear_time;
    bool   all_item_get;
    int    score;
    int    rank;

    StageRecord() :
      clear_time(0.0),
      all_item_get(false),
      score(0),
      rank(RANK_DUMMY)
    {}
  };

  struct CurrentStage {
    double start_time;
    double play_time;

    int item_num;
    int item_total_num;
    int score;
    int rank;

    bool complete_item;
    bool highest_score;
    bool highest_rank;

    CurrentStage() :
      start_time(0.0),
      play_time(0.0),
      item_num(0),
      score(0),
      rank(RANK_DUMMY),
      complete_item(false),
      highest_score(false),
      highest_rank(false)
    {}
  };

  struct CurrentGame {
    int    stage_num;
    double play_time;
    int    score;
    bool   highest_score;
    bool   continued;
    
    CurrentGame() :
      stage_num(0),
      play_time(0.0),
      score(0),
      continued(false),
      highest_score(false)
    {}
  };

  
private:
  CurrentGame current_game_;
  CurrentStage current_stage_;

  bool record_current_game_;

  size_t total_stage_num_;
  size_t regular_stage_num_;
  
  bool all_item_completed_;
  
  int    total_play_num_;
  double total_play_time_;
  int    high_score_;
  int    total_clear_num_;

  std::vector<StageRecord> stage_records_;

  bool se_on_;
  bool bgm_on_;

  float version_;

  GameScore game_score_;
  

public:
  Records(const float version) :
    record_current_game_(false),
    total_stage_num_(0),
    regular_stage_num_(0),
    all_item_completed_(false),
    total_play_num_(0),
    total_play_time_(0.0),
    high_score_(0),
    total_clear_num_(0),
    se_on_(true),
    bgm_on_(true),
    version_(version)
  { }


  void setStageNum(const size_t regular_stage_num,
                   const size_t total_stage_num) {
    regular_stage_num_ = regular_stage_num;
    total_stage_num_   = total_stage_num;
  }


  // スコア計算に必要な情報を格納
  void setScoreInfo(const int clear_time_score, const float clear_time_score_rate,
                    const int item_score,
                    const float stage_collect,
                    const std::vector<int>& rank_rate_table) {

    game_score_ = GameScore(clear_time_score, clear_time_score_rate,
                            item_score,
                            stage_collect,
                            rank_rate_table);
  }
  

  // ゲーム内記録はすべて関数経由で行う
  void enableRecordCurrentGame() {
    record_current_game_ = true;
  }

  void disableRecordCurrentGame() {
    record_current_game_ = false;
  }

  void increaseItemNumCurrentGame() {
    if (!record_current_game_) return;

    current_stage_.item_num += 1;
  }

  void progressPlayTimeCurrntGame(const double progressing_seconds) {
    if (!record_current_game_) return;

    current_stage_.play_time += progressing_seconds;
  }

  void continuedGame(const bool continued_game) {
    current_game_.continued = continued_game;
  }

  const CurrentStage& currentStage() const {
    return current_stage_;
  }
  
  const CurrentGame& currentGame() const {
    return current_game_;
  }
  
  
  // ゲーム開始時の初期化
  void prepareGameRecord() {
    current_game_ = CurrentGame();
  }

  // ステージ開始時の初期化
  void prepareCurrentGameRecord(const int stage_num,
                                const int stage_length, const float build_speed,
                                const double current_time,
                                const int item_num) {
    current_stage_ = CurrentStage();

    current_stage_.start_time     = current_time;
    current_stage_.item_total_num = item_num;

    current_game_.stage_num = stage_num;

    game_score_.setStageInfo(stage_num,
                             stage_length, build_speed,
                             item_num);
  }

  bool isContinuedGame() const {
    return current_game_.continued;
  }

  // ステージクリア時の記録の保存
  void storeStageRecord(const double current_time) {
    StageRecord record;

    double play_time = current_time - current_stage_.start_time;
    record.clear_time   = play_time;
    record.all_item_get = current_stage_.item_num == current_stage_.item_total_num;
    current_stage_.complete_item = record.all_item_get;

    auto stage_score = game_score_(play_time, current_stage_.item_num);
    current_stage_.score = stage_score.first;
    current_stage_.rank = stage_score.second;

    record.score = current_stage_.score;
    record.rank  = current_stage_.rank;
    
    if (isFirstCleard()) {
      stage_records_.push_back(record);
    }
    else {
      updateStageRecord(stage_records_[current_game_.stage_num], record);
    }

    // 記録更新判定
    // 同じ記録でもhighest扱い
    current_stage_.highest_score = current_stage_.score == stage_records_[current_game_.stage_num].score;
    current_stage_.highest_rank  = current_stage_.rank == stage_records_[current_game_.stage_num].rank;

    current_game_.play_time += play_time;
    current_game_.score += current_stage_.score;

    record_current_game_ = false;
  }

  // GameOver時の記録の保存
  void storeRecord(const double current_time) {
    double play_time = current_time - current_stage_.start_time;
    current_game_.play_time += play_time;

    total_play_time_ += current_game_.play_time;
    total_play_num_  += 1;

    current_game_.highest_score = current_game_.score > high_score_;
    high_score_ = std::max(high_score_, current_game_.score);
    
    record_current_game_ = false;
  }

  
  // 10ステージクリア
  void cleardRegularStages() {
    total_play_time_ += current_game_.play_time;
    total_play_num_  += 1;

    current_game_.highest_score = current_game_.score > high_score_;
    high_score_ = std::max(high_score_, current_game_.score);
  }
  
  // 全ステージクリア
  void cleardAllStages() {
    total_play_time_ += current_game_.play_time;
    total_play_num_  += 1;

    current_game_.highest_score = current_game_.score > high_score_;
    high_score_ = std::max(high_score_, current_game_.score);

    total_clear_num_ += 1;
    checkAllItemCompleted();
  }

  std::deque<bool> stageItemComplete() {
    std::deque<bool> item_completed;
    
    for (const auto& record : stage_records_) {
      item_completed.push_back(record.all_item_get);
    }

    return item_completed;
  }
  
  
  void load(const std::string& path) {
    auto full_path = getDocumentPath() / path;
    if (!ci::fs::is_regular_file(full_path)) return;

#if defined(OBFUSCATION_RECORD)
    ci::JsonTree record(TextCodec::load(full_path.string()));
#else
    ci::JsonTree record = ci::JsonTree(ci::loadFile(full_path));
#endif

    total_play_num_  = Json::getValue(record, "total_play_num", 0);
    total_play_time_ = Json::getValue(record, "total_play_time", 0.0);
    total_clear_num_ = Json::getValue(record, "total_clear_num", 0);
    high_score_      = Json::getValue(record, "high_score", 0);

    all_item_completed_ = Json::getValue(record, "all_item_completed", false);

    se_on_  = Json::getValue(record, "se_on", true);
    bgm_on_ = Json::getValue(record, "bgm_on", true);

    if (record.hasChild("stage")) {
      const auto& stage = record["stage"];
      for (const auto& sr : stage) {
        StageRecord s;

        s.clear_time    = Json::getValue(sr, "clear_time", 0.0);
        s.all_item_get  = Json::getValue(sr, "all_item_get", false);
        s.score         = Json::getValue(sr, "score", 0);
        s.rank          = Json::getValue(sr, "rank", int(RANK_DUMMY));

        stage_records_.push_back(std::move(s));
      }
    }

    DOUT << "record loaded." << std::endl
         << full_path << std::endl;
  }
  
  void write(const std::string& path) {
    ci::JsonTree record = ci::JsonTree::makeObject("records");

    record.addChild(ci::JsonTree("total_play_num", total_play_num_))
      .addChild(ci::JsonTree("total_play_time", total_play_time_))
      .addChild(ci::JsonTree("total_clear_num", total_clear_num_))
      .addChild(ci::JsonTree("high_score", high_score_))
      .addChild(ci::JsonTree("all_item_completed", all_item_completed_))
      .addChild(ci::JsonTree("se_on", se_on_))
      .addChild(ci::JsonTree("bgm_on", bgm_on_))
      .addChild(ci::JsonTree("version", version_));

    if (!stage_records_.empty()) {
      ci::JsonTree stage = ci::JsonTree::makeArray("stage");
      for (const auto& s : stage_records_) {
        ci::JsonTree sr;

        sr.addChild(ci::JsonTree("clear_time", s.clear_time))
          .addChild(ci::JsonTree("all_item_get", s.all_item_get))
          .addChild(ci::JsonTree("score", s.score))
          .addChild(ci::JsonTree("rank", s.rank));

        stage.pushBack(sr);
      }
      
      record.addChild(stage);
    }

    auto full_path = getDocumentPath() / path;
#if defined(OBFUSCATION_RECORD)
    TextCodec::write(full_path.string(), record.serialize());
#else
    record.write(full_path);
#endif

    DOUT << "record writed. " << std::endl
         << full_path << std::endl;
  }

  int getTotalPlayNum() const { return total_play_num_; }
  double getTotalPlayTime() const { return total_play_time_; }
  int getTotalClearNum() const { return total_clear_num_; }
  int getHighScore() const { return high_score_; }

  // 10stageまでのitemをcompleteしたか??
  bool isRegularStageCompleted() const {
    if (stage_records_.size() < regular_stage_num_) return false;

    for (size_t i = 0; i < regular_stage_num_; ++i) {
      if (!stage_records_[i].all_item_get) return false;
    }
    return true;
  }

  bool isAllItemCompleted() const {
    return all_item_completed_;
  }

  
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


#ifdef DEBUG
  void forceRegularStageComplated() {
    if (stage_records_.size() < regular_stage_num_) {
      stage_records_.resize(regular_stage_num_);
    }

    for (auto& record : stage_records_) {
      record.all_item_get = true;
    }
  }

  void cancelRegularStageComplated() {
    if (stage_records_.size() < regular_stage_num_) {
      stage_records_.resize(regular_stage_num_);
    }
    
    for (auto& record : stage_records_) {
      record.all_item_get = false;
    }
  }
#endif
  

private:
  bool isFirstCleard() const {
    return current_game_.stage_num == stage_records_.size();
  }
  
  void updateStageRecord(StageRecord& record, const StageRecord& new_record) {
    record.clear_time = std::min(record.clear_time, new_record.clear_time);

    if (!record.all_item_get) record.all_item_get = new_record.all_item_get;
    
    record.score = std::max(record.score, new_record.score);
    record.rank  = std::min(record.rank, new_record.rank);
  }

  void checkAllItemCompleted() {
    if (all_item_completed_) return;
    
    for (size_t i = 0; i < total_stage_num_; ++i) {
      auto& record = stage_records_[i];
      if (!record.all_item_get) return;
    }

    // 全stageの記録でall_item_getならtrue
    all_item_completed_ = true;
  }
  
};

}
