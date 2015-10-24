//
// GameCenter操作
//

#import <GameKit/GameKit.h>
#import "GameCenter.h"
#include <functional>


// リリース時 NSLog 一網打尽マクロ
#ifdef DEBUG
#define NSLOG(...) NSLog(__VA_ARGS__)
#else
#define NSLOG(...) 
#endif


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
      finish_callback();
      authenticated = true;
      
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

void submitScore() noexcept {
  GKScore* scoreReporter = [[GKScore alloc] initWithCategory:@"設定したIDを記入"];

  // 送信する値
  NSInteger scoreR = 0;

  scoreReporter.value = scoreR;
  [scoreReporter reportScoreWithCompletionHandler:^(NSError *error) {
      if (error != nil) {
        NSLOG(@"Sending score error:%@", [error localizedDescription]);
      }
      else {
        NSLOG(@"Sending score OK!");
      }
    }];
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
