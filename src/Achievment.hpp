#pragma once

//
// 実績判定
//

#include "Records.hpp"
#include "GameCenter.h"


namespace ngs { namespace Achievment {

// StageClear時のチェック
void atStageClear(const Records& records) noexcept {
  // STAGEクリア実績
  std::vector<std::pair<int, std::string> > achievements = {
    {  0, "BRICKTRIP.ACHIEVEMENT.CLEARED_STAGE01" },
    {  2, "BRICKTRIP.ACHIEVEMENT.CLEARED_STAGE03" },
    {  5, "BRICKTRIP.ACHIEVEMENT.CLEARED_STAGE06" },
    {  9, "BRICKTRIP.ACHIEVEMENT.CLEARED_STAGE10" },
    { 10, "BRICKTRIP.ACHIEVEMENT.CLEARED_STAGE11" },
  };
    
  for (const auto& a : achievements) {
    if (a.first == records.currentGame().stage_num) {
      GameCenter::submitAchievement(a.second);
      break;
    }
  }
    
  // 10ステージRANK S判定
  if (records.isSatisfyRegularStageRank(0)) {
    GameCenter::submitAchievement("BRICKTRIP.ACHIEVEMENT.GET_10_RANK_S");
  }

  // Itemを１つも取らずにクリア
  if (!records.currentStage().item_num) {
    GameCenter::submitAchievement("BRICKTRIP.ACHIEVEMENT.NO_ITEM");
  }
}

// GameOver時のチェック
void atGameOver(const Records& records) noexcept {
  {
    // プレイ回数による実績
    static std::vector<std::tuple<int, int, std::string> > achievements = {
      {    10,    0, "BRICKTRIP.ACHIEVEMENT.PLAYED_10_TIMES" },
      {    50,    0, "BRICKTRIP.ACHIEVEMENT.PLAYED_50_TIMES" },
      {   100,    0, "BRICKTRIP.ACHIEVEMENT.PLAYED_100_TIMES" },
      {  1000,  100, "BRICKTRIP.ACHIEVEMENT.PLAYED_1000_TIMES" },
      { 10000, 1000, "BRICKTRIP.ACHIEVEMENT.PLAYED_10000_TIMES" },
    };

    int play_num = records.getTotalPlayNum();
    for (const auto& a : achievements) {
      // 後半の項目は、前半のを全て達成してから開示
      if (play_num < std::get<1>(a)) break;
        
      // 毎回更新して、達成率を少しずつあげる
      double rate = play_num * 100.0 / std::get<0>(a);
      GameCenter::submitAchievement(std::get<2>(a), rate);
    }
  }

  {
    // 総取得ITEM数による実績
    static std::vector<std::tuple<int, int, std::string> > achievements = {
      {    10,    0, "BRICKTRIP.ACHIEVEMENT.GOT_10_BRICKS" },
      {    50,    0, "BRICKTRIP.ACHIEVEMENT.GOT_50_BRICKS" },
      {   100,    0, "BRICKTRIP.ACHIEVEMENT.GOT_100_BRICKS" },
      {  1000,  100, "BRICKTRIP.ACHIEVEMENT.GOT_1000_BRICKS" },
      { 10000, 1000, "BRICKTRIP.ACHIEVEMENT.GOT_10000_BRICKS" },
    };
      
    int item_num = records.getTotalItemNum();
    for (const auto& a : achievements) {
      // 後半の項目は、前半のを全て達成してから開示
      if (item_num < std::get<1>(a)) break;
        
      // 毎回更新して、達成率を少しずつあげる
      double rate = item_num * 100.0 / std::get<0>(a);
      GameCenter::submitAchievement(std::get<2>(a), rate);
    }
  }
}

} }
