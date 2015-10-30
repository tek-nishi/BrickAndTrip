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

class Records {
public:
  enum {
    RANK_SATISFY = 1,            // Rank A
    RANK_DUMMY = 100
  };
  
  struct StageRecord {
    double clear_time;
    bool   all_item_get;
    int    score;
    int    rank;

    StageRecord() :
      clear_time(std::numeric_limits<double>::max()),
      all_item_get(false),
      score(0),
      rank(RANK_DUMMY)
    {}
  };

  struct CurrentStage {
    double play_time;

    int item_num;
    int item_total_num;
    int score;
    int rank;

    bool complete_item;
    bool fastest_time;
    bool highest_score;
    bool highest_rank;

    CurrentStage() :
      play_time(0.0),
      item_num(0),
      score(0),
      rank(RANK_DUMMY),
      complete_item(false),
      fastest_time(false),
      highest_score(false),
      highest_rank(false)
    {}
  };

  struct CurrentGame {
    int stage_num;

    int  score;
    bool highest_score;

    int  item_num;
    int  item_total_num;
    bool highest_item_num;

    bool continued;
    
    CurrentGame() :
      stage_num(0),
      score(0),
      highest_score(false),
      item_num(0),
      item_total_num(0),
      highest_item_num(false),
      continued(false)
    {}
  };

  
private:
  CurrentGame  current_game_;
  CurrentStage current_stage_;

  bool record_current_game_;

  size_t total_stage_num_;
  size_t regular_stage_num_;
  
  int    total_play_num_;
  double total_play_time_;
  int    high_score_;
  int    high_item_num_;
  int    total_item_num_;

  std::vector<StageRecord> stage_records_;

  
  // 全ステージの総アイテム数
  int regular_item_num_;
  int all_item_num_;

  // 11ステージが解放されるための取得ランク
  int rank_satisfy_;
  
  bool se_on_;
  bool bgm_on_;

  float version_;

  GameScore game_score_;
  

public:
  Records(const float version) :
    record_current_game_(false),
    total_stage_num_(0),
    regular_stage_num_(0),
    total_play_num_(0),
    total_play_time_(0.0),
    high_score_(0),
    high_item_num_(0),
    total_item_num_(0),
    regular_item_num_(0),
    all_item_num_(0),
    rank_satisfy_(RANK_SATISFY),
    se_on_(true),
    bgm_on_(true),
    version_(version)
  { }


  void setStageNum(const size_t regular_stage_num,
                   const size_t total_stage_num) noexcept {
    regular_stage_num_ = regular_stage_num;
    total_stage_num_   = total_stage_num;

#ifdef DEBUG
    if (stage_records_.size() < total_stage_num) {
      stage_records_.resize(total_stage_num);
    }
#endif
  }


  // スコア計算に必要な情報を格納
  void setScoreInfo(const int clear_time_score, const float clear_time_score_rate,
                    const int item_score, const int item_perfect_score,
                    const float stage_collect,
                    const std::vector<int>& rank_rate_table) noexcept {

    game_score_ = GameScore(clear_time_score, clear_time_score_rate,
                            item_score, item_perfect_score,
                            stage_collect,
                            rank_rate_table);
  }

  // 全ステージのアイテム数を格納
  void setItemNum(const int regular_item_num, const int all_item_num) noexcept {
    regular_item_num_ = regular_item_num;
    all_item_num_     = all_item_num;
  }

  // Stage11が登場するための要求ランク
  void setSatisfyRank(const int rank) noexcept {
    rank_satisfy_ = rank;
  }

  
  // ゲーム内記録はすべて関数経由で行う
  void enableRecordCurrentGame() noexcept {
    record_current_game_ = true;
  }

  void disableRecordCurrentGame() noexcept {
    record_current_game_ = false;
  }

  void increaseItemNumCurrentGame() noexcept {
    if (!record_current_game_) return;

    current_stage_.item_num += 1;
  }

  void progressPlayTimeCurrntGame(const double progressing_seconds) noexcept {
    if (!record_current_game_) return;

    current_stage_.play_time += progressing_seconds;
  }

  void continuedGame(const bool continued_game) noexcept {
    current_game_.continued = continued_game;
  }

  const CurrentStage& currentStage() const noexcept {
    return current_stage_;
  }
  
  const CurrentGame& currentGame() const noexcept {
    return current_game_;
  }

  
  // ゲーム開始時の初期化
  void prepareGameRecord() noexcept {
    current_game_ = CurrentGame();
  }

  // ステージ開始時の初期化
  void prepareCurrentGameRecord(const int stage_num,
                                const int stage_length, const float build_speed,
                                const float build_time,
                                const double current_time,
                                const int item_num) noexcept {
    current_stage_ = CurrentStage();

    current_stage_.item_total_num = item_num;
    current_game_.stage_num = stage_num;

    game_score_.setStageInfo(stage_num,
                             stage_length, build_speed,
                             build_time,
                             item_num);

    DOUT << "Records stage:" << stage_num << std::endl
         << "       length:" << stage_length << std::endl
         << "   build time:" << build_time << std::endl
         << "     item_num:" << item_num << std::endl;
  }

  bool isContinuedGame() const noexcept {
    return current_game_.continued;
  }

  // ステージクリア時の記録の保存
  void storeStageRecord() noexcept {
    StageRecord record;

    record.clear_time   = current_stage_.play_time;
    record.all_item_get = current_stage_.item_num == current_stage_.item_total_num;
    current_stage_.complete_item = record.all_item_get;

    auto stage_score = game_score_(current_stage_.play_time, current_stage_.item_num);
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
    // 同じ記録でもhighest扱い(ステージの記録は更新済みなので、値が一致→記録更新とみなせる)
    current_stage_.fastest_time  = current_stage_.play_time == stage_records_[current_game_.stage_num].clear_time;
    current_stage_.highest_score = current_stage_.score == stage_records_[current_game_.stage_num].score;
    current_stage_.highest_rank  = current_stage_.rank == stage_records_[current_game_.stage_num].rank;

    current_game_.score += current_stage_.score;
    current_game_.item_num += current_stage_.item_num;

    record_current_game_ = false;
  }

  // GameOver時の記録の保存
  void storeGameOverRecords() noexcept {
    storeRecord();

    total_play_time_ += current_stage_.play_time;
    total_item_num_  += current_stage_.item_num;
    
    current_game_.score += game_score_.itemScore(current_stage_.item_num);
  }
  
  // 10ステージクリア
  void cleardRegularStages() noexcept {
    storeRecord();

    high_item_num_ = std::max(current_game_.item_num, high_item_num_);
    // 同じ値も記録更新
    current_game_.highest_item_num = current_game_.item_num == high_item_num_;
    // 結果画面で使う
    current_game_.item_total_num = regular_item_num_;
  }
  
  // 全11ステージクリア
  void cleardAllStages() noexcept {
    storeRecord();

    high_item_num_ = std::max(current_game_.item_num, high_item_num_);
    current_game_.highest_item_num = current_game_.item_num > high_item_num_;
    current_game_.item_total_num = all_item_num_;
  }

  std::deque<bool> stageItemComplete() const noexcept {
    std::deque<bool> item_completed;
    
    for (const auto& record : stage_records_) {
      item_completed.push_back(record.all_item_get);
    }

    return item_completed;
  }
  
  std::vector<int> stageRanks() const noexcept {
    std::vector<int> ranks;
    
    for (const auto& record : stage_records_) {
      ranks.push_back(record.rank);
    }

    return ranks;
  }
  
  void load(const std::string& path) noexcept {
    auto full_path = getDocumentPath() / path;
    if (!ci::fs::is_regular_file(full_path)) return;

#if defined(OBFUSCATION_RECORD)
    ci::JsonTree record(TextCodec::load(full_path.string()));
#else
    ci::JsonTree record = ci::JsonTree(ci::loadFile(full_path));
#endif

    total_play_num_  = Json::getValue(record, "total_play_num", 0);
    total_play_time_ = Json::getValue(record, "total_play_time", 0.0);
    high_score_      = Json::getValue(record, "high_score", 0);
    high_item_num_   = Json::getValue(record, "high_item_num", 0);
    total_item_num_  = Json::getValue(record, "total_item_num", 0);

    se_on_  = Json::getValue(record, "se_on", true);
    bgm_on_ = Json::getValue(record, "bgm_on", true);

#ifdef DEBUG
    stage_records_.clear();
#endif
    
    if (record.hasChild("stage")) {
      const auto& stage = record["stage"];
      for (const auto& sr : stage) {
        StageRecord s;

        s.clear_time   = Json::getValue(sr, "clear_time", 0.0);
        s.all_item_get = Json::getValue(sr, "all_item_get", false);
        s.score        = Json::getValue(sr, "score", 0);
        s.rank         = Json::getValue(sr, "rank", int(RANK_DUMMY));

        stage_records_.push_back(std::move(s));
      }
    }

    DOUT << "record loaded." << std::endl
         << "stage:" << stage_records_.size() << std::endl
         << full_path << std::endl;
  }
  
  void write(const std::string& path) const noexcept {
    ci::JsonTree record = ci::JsonTree::makeObject("records");

    record.addChild(ci::JsonTree("total_play_num", total_play_num_))
      .addChild(ci::JsonTree("total_play_time", total_play_time_))
      .addChild(ci::JsonTree("high_score", high_score_))
      .addChild(ci::JsonTree("high_item_num", high_item_num_))
      .addChild(ci::JsonTree("total_item_num", total_item_num_))
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
         << "stage:" << stage_records_.size() << std::endl
         << full_path << std::endl;
  }

  int getTotalPlayNum() const noexcept { return total_play_num_; }

  double getTotalPlayTime() const noexcept { return total_play_time_; }

  int getHighScore() const noexcept { return high_score_; }

  int getTotalItemNum() const noexcept { return total_item_num_; }

  
  // 10stageまでのitemをcompleteしたか??
  bool isRegularStageCompleted() const noexcept {
    if (stage_records_.size() < regular_stage_num_) return false;

    for (size_t i = 0; i < regular_stage_num_; ++i) {
      if (!stage_records_[i].all_item_get) return false;
    }
    return true;
  }
  
  // 10stageまでのrankが規定以上か??
  bool isSatisfyRegularStageRank(const int rank_satisfy) const noexcept {
    if (stage_records_.size() < regular_stage_num_) return false;
    
    for (size_t i = 0; i < regular_stage_num_; ++i) {
      if (stage_records_[i].rank > rank_satisfy) return false;
    }
    return true;
  }

  bool isSatisfyRegularStageRank() const noexcept {
    return isSatisfyRegularStageRank(rank_satisfy_);
  }
  
  bool isSeOn() const noexcept { return se_on_; }
  bool isBgmOn() const noexcept { return bgm_on_; }
  
  bool toggleSeOn() noexcept {
    se_on_ = !se_on_;
    return se_on_;
  }
  
  bool toggleBgmOn() noexcept {
    bgm_on_ = !bgm_on_;
    return bgm_on_;
  }
  

#ifdef DEBUG
  void forceRegularStageComplated() noexcept {
    if (stage_records_.size() < regular_stage_num_) {
      stage_records_.resize(regular_stage_num_);
    }

    for (auto& record : stage_records_) {
      record.rank = std::min(record.rank, rank_satisfy_);
    }
  }

  void cancelRegularStageComplated() noexcept {
    if (stage_records_.size() < regular_stage_num_) {
      stage_records_.resize(regular_stage_num_);
    }
    
    for (auto& record : stage_records_) {
      record.rank = std::max(record.rank, rank_satisfy_ + 1);
    }
  }
#endif
  

private:
  void storeRecord() noexcept {
    total_play_num_  += 1;

    high_score_ = std::max(high_score_, current_game_.score);
    // 同じ値も記録更新
    current_game_.highest_score = current_game_.score == high_score_;
    total_item_num_ += current_game_.item_num;
    
    record_current_game_ = false;
  }
  
  bool isFirstCleard() const noexcept {
    return current_game_.stage_num == stage_records_.size();
  }
  
  void updateStageRecord(StageRecord& record, const StageRecord& new_record) noexcept {
    record.clear_time = std::min(record.clear_time, new_record.clear_time);

    if (!record.all_item_get) record.all_item_get = new_record.all_item_get;
    
    record.score = std::max(record.score, new_record.score);
    record.rank  = std::min(record.rank,  new_record.rank);
  }

#if 0
  void checkAllItemCompleted() noexcept {
    if (all_item_completed_) return;
    
    for (size_t i = 0; i < total_stage_num_; ++i) {
      auto& record = stage_records_[i];
      if (!record.all_item_get) return;
    }

    // 全stageの記録でall_item_getならtrue
    all_item_completed_ = true;
  }

  void checkAllRankSatisfied() noexcept {
    if (all_rank_satisfy_) return;
    
    for (size_t i = 0; i < total_stage_num_; ++i) {
      auto& record = stage_records_[i];
      if (record.rank > rank_satisfy_) return;
    }

    all_rank_satisfy_ = true;
  }
#endif
  
};

}
