// 
// 評価ダイアログ
//

#include "Defines.hpp"
#include "Rating.h"


namespace ngs { namespace Rating {

enum {
  RATE_FREQENCY = 10
};

// FIXME:グローバル変数を止める
static std::function<void()> alert_finish;


// UIAlertViewでの表示(iOS8以前)
static void popupUIAlertView(std::function<void()> finish_callback) noexcept {
  alert_finish = finish_callback;
  
  auto app_vc = ci::app::getWindow()->getNativeViewController();
  
  UIAlertView* alert = [[[UIAlertView alloc]
                            initWithTitle: NSLocalizedString(@"rate_title", nil)
                            message: NSLocalizedString(@"rate_text", nil)
                            delegate: app_vc
                            cancelButtonTitle: NSLocalizedString(@"rate_now", nil)
                            otherButtonTitles: NSLocalizedString(@"rate_cancel", nil), NSLocalizedString(@"rate_later", nil),
                            nil]
                           autorelease];
  [alert show];
}

// UIAlertViewでの表示(iOS8以降)
static void popupUIAlertController(std::function<void()> finish_callback) noexcept {
  UIAlertController *alertController = [UIAlertController
                                           alertControllerWithTitle: NSLocalizedString(@"rate_title", nil)
                                           message: NSLocalizedString(@"rate_text", nil)
                                           preferredStyle: UIAlertControllerStyleAlert];

  // addActionした順に左から右にボタンが配置されます
  [alertController addAction:[UIAlertAction
                                 actionWithTitle: NSLocalizedString(@"rate_cancel", nil)
                                 style: UIAlertActionStyleDefault
                                 handler: ^(UIAlertAction* action) {
        // otherボタンが押された時の処理
        [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"rate_exec"];
        NSLOG(@"Rating:No Thanks");
        finish_callback();
      }]];
  
  [alertController addAction:[UIAlertAction
                                 actionWithTitle: NSLocalizedString(@"rate_later", nil)
                                 style: UIAlertActionStyleDefault
                                 handler: ^(UIAlertAction* action) {
        NSLOG(@"Rating:Remind me later");
        finish_callback();
      }]];
  
  [alertController addAction:[UIAlertAction
                                 actionWithTitle: NSLocalizedString(@"rate_now", nil)
                                 style: UIAlertActionStyleCancel
                                 handler: ^(UIAlertAction* action) {
        NSString* url = NSLocalizedString(@"rate_url", nil);
        [[UIApplication sharedApplication] openURL:[NSURL URLWithString:url]];
    
        [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"rate_exec"];
        NSLOG(@"Rating:OK");
        finish_callback();
      }]];

  auto app_vc = ci::app::getWindow()->getNativeViewController();
  [app_vc presentViewController:alertController animated:YES completion:nil];
}


// ダイアログ表示
void popup(std::function<void()> start_callback,
           std::function<void()> finish_callback) noexcept {
  // ダイアログは時々表示するようにしている
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];

  // 実行済みなら処理しない
  if ([defaults boolForKey:@"rate_exec"]) return;

  NSInteger rate_num = [defaults integerForKey:@"rate_num"];
  [defaults setInteger:rate_num + 1 forKey:@"rate_num"];

  NSLOG(@"rating:rate_num:%ld", long(rate_num));
  if ((rate_num % RATE_FREQENCY) != (RATE_FREQENCY - 1)) return;

  start_callback();

  if (NSClassFromString(@"UIAlertController")) {
    // iOS8以降で追加されたUIAlertControllerがあれば、それを使う
    popupUIAlertController(finish_callback);
  }
  else {
    popupUIAlertView(finish_callback);
  }
}

// 初期化
void reset() noexcept {
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  [defaults setBool:NO forKey:@"rate_exec"];
}

} }


// TIPS:空のクラス定義
//      実際の定義はcinderのAppCocoaTouch.mm
@interface WindowImplCocoaTouch
@end

// 既存のクラスにクラスメソッドを追加する
@interface WindowImplCocoaTouch(Rating)

-(void)alertView:(UIAlertView*)alertView clickedButtonAtIndex:(NSInteger)buttonIndex;
  
@end

@implementation WindowImplCocoaTouch(Rating)

-(void)alertView:(UIAlertView*)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  
  switch (buttonIndex) {
  case 0:
    // Rate now
    {
      NSString* url = NSLocalizedString(@"rate_url", nil);
      [[UIApplication sharedApplication] openURL:[NSURL URLWithString:url]];
    
      [defaults setBool:YES forKey:@"rate_exec"];
      NSLOG(@"Rating:OK");
    }
    break;

  case 1:
    // Cancel
    {
      [defaults setBool:YES forKey:@"rate_exec"];
      NSLOG(@"Rating:No Thanks");
    }
    break;

  case 2:
    // Rate later
    {
      NSLOG(@"Rating:Remind me later");
    }
    break;
  }

  ngs::Rating::alert_finish();
}

@end
