//
// GameCenter操作
//

#import <GameKit/GameKit.h>
#include <sstream>
#include <iomanip>
#import "GameCenter.h"


namespace ngs { namespace GameCenter {

// FIXME:グローバル変数を止める
static bool authenticated = false;

void authenticateLocalPlayer(std::function<void()> start_callback,
                             std::function<void()> finish_callback) noexcept {
  authenticated = false;
  
  GKLocalPlayer* player = [GKLocalPlayer localPlayer];
  player.authenticateHandler = ^(UIViewController* viewController, NSError* error) {
    if (viewController != nil) {
      // 認証ダイアログ表示
      start_callback();
      UIViewController* app_vc = ci::app::getWindow()->getNativeViewController();
      [app_vc presentViewController:viewController animated:YES completion:nil];
    }
    else if ([GKLocalPlayer localPlayer].isAuthenticated) {
      authenticated = true;
      finish_callback();
      
      NSLOG(@"認証成功");
    }
    else if (error != nil) {
      finish_callback();
      
      NSLOG(@"認証失敗");
    }
  };
}

bool isAuthenticated() noexcept {
  return authenticated;
}


// FIXME:グローバル変数を止める
static std::function<void()> leaderboard_finish;

void showBoard(std::function<void()> start_callback,
               std::function<void()> finish_callback) noexcept {
  GKLeaderboardViewController* leaderboardController = [[[GKLeaderboardViewController alloc] init] autorelease];

  if (leaderboardController != nil) {
    leaderboard_finish = finish_callback;
    start_callback();
    
    auto app_vc = (UIViewController<GKLeaderboardViewControllerDelegate>*)ci::app::getWindow()->getNativeViewController();
    leaderboardController.leaderboardDelegate = app_vc;
    [app_vc presentViewController:leaderboardController animated:YES completion:nil];
  }  
}


// スコア送信
static void sendScore(NSArray* scores) {
  [GKScore reportScores:scores withCompletionHandler:^(NSError* error) {
      if (error != nil) {
        NSLOG(@"Sending score error:%@", [error localizedDescription]);
      }
      else {
        NSLOG(@"Sending score OK!");
      }
    }];
}

static NSString* createString(const std::string& text) {
  NSString* str = [[[NSString alloc] initWithCString:text.c_str() encoding:NSUTF8StringEncoding] autorelease];
  return str;
}


void submitStageScore(const int stage,
                      const int score, const double clear_time) noexcept {
  std::ostringstream str;
  str << "BRICKTRIP.STAGE"
      << std::setw(2) << std::setfill('0') << stage;

  std::string hiscore_id(str.str() + ".HISCORE");
  GKScore* hiscore_reporter = [[[GKScore alloc] initWithCategory:createString(hiscore_id)] autorelease];
  hiscore_reporter.value = score;

  DOUT << "submit:" << hiscore_id << " " << score << std::endl;
  
  std::string besttime_id(str.str() + ".BESTTIME");
  GKScore* besttime_reporter = [[[GKScore alloc] initWithCategory:createString(besttime_id)] autorelease];
  besttime_reporter.value = int64_t(clear_time * 100.0);

  DOUT << "submit:" << besttime_id << " " << clear_time << std::endl;

  NSArray* score_array = @[hiscore_reporter, besttime_reporter];
  sendScore(score_array);
}

void submitScore(const int score) noexcept {
  if (score == 0) return;
  
  GKScore* score_reporter = [[[GKScore alloc] initWithCategory:createString("BRICKTRIP.HISCORE")] autorelease];
  score_reporter.value = score;

  DOUT << "submit:" << score << std::endl;
  
  NSArray* score_array = @[score_reporter];
  sendScore(score_array);
}

} }


// TIPS:空のクラス定義
//      実際の定義はcinderのAppCocoaTouch.mm
@interface WindowImplCocoaTouch
@end

// 既存のクラスにクラスメソッドを追加する
@interface WindowImplCocoaTouch(GameCenter)

- (void)leaderboardViewControllerDidFinish:(GKLeaderboardViewController*)viewController;

@end

@implementation WindowImplCocoaTouch(GameCenter)

- (void)leaderboardViewControllerDidFinish:(GKLeaderboardViewController*)viewController {
  [viewController dismissViewControllerAnimated:YES completion:nil];
  ngs::GameCenter::leaderboard_finish();
  NSLOG(@"leaderboardViewControllerDidFinish");
}

@end
