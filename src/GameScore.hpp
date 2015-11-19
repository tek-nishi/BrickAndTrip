#pragma once

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
  int item_perfect_score_;

  // ステージ補正
  float stage_collect_;

  // 移動量によるスコア加算
  int move_step_;
  
  // ランク
  std::vector<int> rank_rate_table_;

  // ステージ情報
  int   stage_num_;
  int   stage_length_;
  float build_speed_;
  float build_time_;
  int   item_total_num_;
  

public:
  GameScore() = default;
  
  GameScore(const int clear_time_score, const float clear_time_score_rate,
            const int item_score, const int item_perfect_score,
            const float stage_collect,
            const int move_step,
            const std::vector<int>& rank_rate_table) noexcept :
    clear_time_score_(clear_time_score),
    clear_time_score_rate_(clear_time_score_rate),
    item_score_(item_score),
    item_perfect_score_(item_perfect_score),
    stage_collect_(stage_collect),
    move_step_(move_step),
    rank_rate_table_(rank_rate_table)
  {}

  
  void setStageInfo(const int stage_num,
                    const int stage_length, const float build_speed,
                    const float build_time,
                    const int item_total_num) noexcept {
    stage_num_      = stage_num;
    stage_length_   = stage_length;
    build_speed_    = build_speed;
    build_time_     = build_time;
    item_total_num_ = item_total_num;
  }


  std::pair<int, int> operator() (const double clear_time,
                                  const int item_num,
                                  const int move_step) const noexcept {
    int total_score = 0;
    // 最高スコア(理論値)
    int highest_total_score = 0;

    {
      // クリア時間による加点

      float time_remain = std::max(float(clear_time) - build_time_, 0.0f);

      // (x - 1)^-1 はx=0の時にy=1で、xが増加するとyは0に収束する
      float score_rate = std::pow((time_remain * clear_time_score_rate_ + 1.0f), -1.0f);

      auto score         = int(clear_time_score_ * score_rate);
      auto highest_score = clear_time_score_;
    
      total_score += score;
      highest_total_score += highest_score;

      DOUT << "score from clear-time:" << score << std::endl
           << "              highest:" << highest_score << std::endl
           << "           score rate:" << score_rate << std::endl
           << "              fastest:" << build_time_ << std::endl
           << "           clear time:" << clear_time << std::endl
           << "            move_step:" << move_step << std::endl
           << std::endl;
    }

    {
      // アイテムによる加点
      // 収集率で決まる
      auto score         = itemScore(item_num);
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

      DOUT << "sore rate:" << score_rate << std::endl;
    }

    // 最高移動量によるボーナス(RANKには影響しない)
    total_score += move_step * move_step_;

    // アイテム100% BONUS(ランク判定に寄与しない)
    if (item_num == item_total_num_) total_score += item_perfect_score_;

    DOUT << "SCORE:" << total_score
         << " RANK:" << rank
         << std::endl;

    return std::make_pair(total_score, rank);
  }


  // GameOver時に加点したいので関数化
  int itemScore(const int item_num) const noexcept {
    return item_score_ * item_num / item_total_num_;
  }
  
};

}
