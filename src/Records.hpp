#pragma once

//
// プレイ記録
// 

#include <vector>
#include <boost/noncopyable.hpp>
#include "FileUtil.hpp"
#include "TextCodec.hpp"


namespace ngs {

class Records : private boost::noncopyable {
public:
  struct StageRecord {
    double clear_time;
    int    tumble_num;
    int    item_num;
    bool   all_item_get;
    int    operation_num;
    int    score;

    StageRecord() :
      clear_time(0.0),
      tumble_num(0),
      item_num(0),
      all_item_get(false),
      operation_num(0),
      score()
    {}
  };
  
  struct CurrentGame {
    double start_time;
    double play_time;

    int tumble_num;
    int item_num;
    int item_total_num;
    int operation_num;

    CurrentGame() :
      start_time(0.0),
      play_time(0.0),
      tumble_num(0),
      item_num(0),
      operation_num(0)
    {}
  };

  
private:
  CurrentGame current_game_;

  bool record_current_game_;

  size_t total_stage_num_;
  size_t regular_stage_num_;
  
  bool all_item_completed_;
  
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

  float version_;
  

public:
  Records(const float version) :
    record_current_game_(false),
    total_stage_num_(0),
    regular_stage_num_(0),
    all_item_completed_(false),
    total_play_num_(0),
    total_play_time_(0.0),
    total_tumble_num_(0),
    total_item_num_(0),
    total_operation_num_(0),
    total_clear_num_(0),
    current_stage_(0),
    current_play_time_(0.0),
    se_on_(true),
    bgm_on_(true),
    version_(version)
  { }


  void setStageNum(const size_t regular_stage_num, const size_t total_stage_num) {
    regular_stage_num_ = regular_stage_num;
    total_stage_num_   = total_stage_num;
  }
  

  // ゲーム内記録はすべて関数経由で行う
  void enableRecordCurrentGame() {
    record_current_game_ = true;
  }

  void disableRecordCurrentGame() {
    record_current_game_ = false;
  }
  
  void increaseTumbleNumCurrentGame() {
    if (!record_current_game_) return;

    current_game_.tumble_num += 1;
  }

  void increaseItemNumCurrentGame() {
    if (!record_current_game_) return;

    current_game_.item_num += 1;
  }

  void increaseOperationNumCurrentGame() {
    if (!record_current_game_) return;

    current_game_.operation_num += 1;
  }

  void progressPlayTimeCurrntGame(const double progressing_seconds) {
    if (!record_current_game_) return;

    current_game_.play_time += progressing_seconds;
  }

  const CurrentGame& currentGame() const {
    return current_game_;
  }
  
  
  // 最初のステージでのみ必要な初期化
  void prepareGameRecord() {
    current_play_time_ = 0.0;
  }
  
  void prepareCurrentGameRecord(const int stage_num,
                                const double current_time,
                                const int item_num) {
    current_stage_ = stage_num;

    current_game_ = CurrentGame();
    current_game_.start_time = current_time;
    current_game_.item_total_num = item_num;
  }
  
  bool isFirstCleard() const {
    return current_stage_ == stage_records_.size();
  }

  int cleardStageNum() {
    return int(stage_records_.size());
  }

  void storeStageRecord(const double current_time) {
    StageRecord record;

    double play_time = current_time - current_game_.start_time;
    record.clear_time    = play_time;
    record.tumble_num    = current_game_.tumble_num;
    record.item_num      = current_game_.item_num;
    record.all_item_get  = current_game_.item_num == current_game_.item_total_num;
    record.operation_num = current_game_.operation_num;

    // TODO:score計算
    // record.score = 0;
    
    if (isFirstCleard()) {
      stage_records_.push_back(record);
    }
    else {
      updateStageRecord(stage_records_[current_stage_], record);
    }
    
    current_play_time_ += play_time;
    
    total_play_time_     += record.clear_time;
    total_tumble_num_    += current_game_.tumble_num;
    total_item_num_      += current_game_.item_num;
    total_operation_num_ += current_game_.operation_num;

    record_current_game_ = false;
  }

  
  void storeRecord(const double current_time) {
    double play_time = current_time - current_game_.start_time;

    current_play_time_ += play_time;

    // TIPS:GameOverでも記録が伸びる親切設計
    total_play_time_     += play_time;
    total_tumble_num_    += current_game_.tumble_num;
    total_item_num_      += current_game_.item_num;
    total_operation_num_ += current_game_.operation_num;
    total_play_num_      += 1;

    record_current_game_ = false;
  }

  // クリア回数
  void cleardAllStage() {
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
    // JsonTree::serializeだとルートが削除される
    // その挙動に合わせている
    ci::JsonTree record = ci::JsonTree(ci::loadFile(full_path))["records"];
#endif

    total_play_num_      = Json::getValue(record, "total_play_num", 0);
    total_play_time_     = Json::getValue(record, "total_play_time", 0.0);
    total_tumble_num_    = Json::getValue(record, "total_tumble_num", 0);
    total_item_num_      = Json::getValue(record, "total_item_num", 0);
    total_operation_num_ = Json::getValue(record, "total_operation_num", 0);
    total_clear_num_     = Json::getValue(record, "total_clear_num", 0);

    all_item_completed_ = Json::getValue(record, "all_item_completed", false);

    se_on_  = Json::getValue(record, "se_on", true);
    bgm_on_ = Json::getValue(record, "bgm_on", true);

    if (record.hasChild("stage")) {
      const auto& stage = record["stage"];
      for (const auto& sr : stage) {
        StageRecord s;

        s.clear_time    = Json::getValue(sr, "clear_time", 0.0);
        s.tumble_num    = Json::getValue(sr, "tumble_num", 0);
        s.item_num      = Json::getValue(sr, "item_num", 0);
        s.all_item_get  = Json::getValue(sr, "all_item_get", false);
        s.operation_num = Json::getValue(sr, "operation_num", 0);
        s.score         = Json::getValue(sr, "score", 0);

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
      .addChild(ci::JsonTree("total_tumble_num", total_tumble_num_))
      .addChild(ci::JsonTree("total_item_num", total_item_num_))
      .addChild(ci::JsonTree("total_operation_num", total_operation_num_))
      .addChild(ci::JsonTree("total_clear_num", total_clear_num_))
      .addChild(ci::JsonTree("all_item_completed", all_item_completed_))
      .addChild(ci::JsonTree("se_on", se_on_))
      .addChild(ci::JsonTree("bgm_on", bgm_on_))
      .addChild(ci::JsonTree("version", version_));

    if (!stage_records_.empty()) {
      ci::JsonTree stage = ci::JsonTree::makeArray("stage");
      for (const auto& s : stage_records_) {
        ci::JsonTree sr;
        sr.addChild(ci::JsonTree("clear_time", s.clear_time))
          .addChild(ci::JsonTree("tumble_num", s.tumble_num))
          .addChild(ci::JsonTree("item_num", s.item_num))
          .addChild(ci::JsonTree("all_item_get", s.all_item_get))
          .addChild(ci::JsonTree("operation_num", s.operation_num))
          .addChild(ci::JsonTree("score", s.score));

        stage.pushBack(sr);
      }
      
      record.addChild(stage);
    }

    auto full_path = getDocumentPath() / path;
#if defined(OBFUSCATION_RECORD)
    TextCodec::write(full_path.string(), record.serialize());
#else
    record.write(full_path, ci::JsonTree::WriteOptions().createDocument(true));
#endif

    DOUT << "record writed. " << std::endl
         << full_path << std::endl;
  }

  double getCurrentGamePlayTime() const { return current_play_time_; }

  int getTotalPlayNum() const { return total_play_num_; }
  double getTotalPlayTime() const { return total_play_time_; }
  int getTotalTumbleNum() const { return total_tumble_num_; }
  int getTotalItemNum() const { return total_item_num_; }
  int getTotalOperationNum() const { return total_operation_num_; }
  int getTotalClearNum() const { return total_clear_num_; }

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
  void updateStageRecord(StageRecord& record, const StageRecord& new_record) {
    record.clear_time = std::min(record.clear_time, new_record.clear_time);
    record.tumble_num = std::min(record.tumble_num, new_record.tumble_num);

    record.item_num = std::max(record.item_num, new_record.item_num);
    if (new_record.all_item_get) record.all_item_get = true;
    
    record.operation_num = std::min(record.operation_num, new_record.operation_num);
    record.score = std::max(record.score, new_record.score);
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
