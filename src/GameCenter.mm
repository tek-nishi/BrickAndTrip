//
// GameCenter操作
//

#import <GameKit/GameKit.h>
#import "GameCenter.h"

namespace ngs { namespace GameCenter {

void authenticateLocalPlayer() noexcept {
  GKLocalPlayer* player = [GKLocalPlayer localPlayer];

  player.authenticateHandler = ^(UIViewController* viewController, NSError* error) {
    if (viewController != nil) {
      UIViewController* app_vc = ci::app::getWindow()->getNativeViewController();
      [app_vc presentViewController:viewController animated:YES completion:nil];
    }
    else if ([GKLocalPlayer localPlayer].isAuthenticated) {
      NSLog(@"認証成功");
    }
    else if (error != nil) {
      NSLog(@"認証失敗");
    }
  };
}

void showBord() noexcept {
  GKLeaderboardViewController* leaderboardController = [[[GKLeaderboardViewController alloc] init] autorelease];

  if (leaderboardController != nil) {
    id<GKLeaderboardViewControllerDelegate> app_vc = ci::app::getWindow()->getNativeViewController();
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
        NSLog(@"Sending score error:%@", [error localizedDescription]);
      }
      else {
        NSLog(@"Sending score OK!");
      }
    }];
}

} }
