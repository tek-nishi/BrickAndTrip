﻿#pragma once

//
// Game Scoreを計算
//

namespace ngs {

class GameScore {
  // クリア時間に関する情報
  int   clear_time_score_;
  float clear_time_score_rate_;

  // 取得アイテムに関する情報
  int item_score_;

  // ステージ補正
  float stage_collect_;

  // ランク
  std::vector<int> rank_rate_table_;

  // ステージ情報
  int   stage_num_;
  int   stage_length_;
  float build_speed_;
  int   item_total_num_;
  

public:
  GameScore() = default;
  
  GameScore(const int clear_time_score, const float clear_time_score_rate,
            const int item_score,
            const float stage_collect,
            const std::vector<int>& rank_rate_table) :
    clear_time_score_(clear_time_score),
    clear_time_score_rate_(clear_time_score_rate),
    item_score_(item_score),
    stage_collect_(stage_collect),
    rank_rate_table_(rank_rate_table)
  {}

  
  void setStageInfo(const int stage_num,
                    const int stage_length, const float build_speed,
                    const int item_total_num) {
    stage_num_      = stage_num;
    stage_length_   = stage_length;
    build_speed_    = build_speed;
    item_total_num_ = item_total_num;

    DOUT << "stage length:" << stage_length << std::endl;
  }


  std::pair<int, int> operator() (const double clear_time,
                                  const int item_num) {
    int total_score = 0;
    // 最高スコア(理論値)
    int highest_total_score = 0;

    {
      // クリア時間による加点

      // ステージの長さと生成速度から、最短クリア時間が決まる
      float clear_fastest_time = stage_length_ * build_speed_;
      float time_remain = std::max(float(clear_time) - clear_fastest_time, 0.0f);

      // (x - 1)^-1 はx=0の時にy=1で、xが増加するとyは0に収束する
      float score_rate = std::pow((time_remain * clear_time_score_rate_ + 1.0f), -1.0f);

      auto score         = int(clear_time_score_ * score_rate);
      auto highest_score = clear_time_score_;
    
      total_score += score;
      highest_total_score += highest_score;

      DOUT << "score from clear-time:" << score
           << " highest:" << highest_score
           << std::endl;
    }

    {
      // アイテムによる加点
      // 収集率で決まる
      auto score         = item_score_ * item_num / item_total_num_;
      auto highest_score = item_score_;
    
      total_score += score;
      highest_total_score += highest_score;

      DOUT << "score from item:" << score
           << " highest:" << highest_score
           << std::endl;
    }

    {
      // ステージ補正
      total_score += total_score * stage_num_ * stage_collect_;
      highest_total_score += highest_total_score * stage_num_ * stage_collect_;

      DOUT << "total score:" << total_score
           << " highest:" << highest_total_score
           << std::endl;
    }

    // RANK確定
    int rank = 0;
    {
      int score_rate = total_score * 100 / highest_total_score;
      for (auto rank_rate : rank_rate_table_) {
        if (score_rate > rank_rate) break;
        rank += 1;
      }
    }

    DOUT << "SCORE:" << total_score
         << " RANK:" << rank
         << std::endl;

    return std::make_pair(total_score, rank);
  }
};

}