//
// SNS投稿(iOS専用)
//

#import <Social/Social.h>
#include "cinder/app/Window.h"
#import "Social.h"
#include "Defines.hpp"
#include <string>
#include <cassert>
#include <functional>


namespace ngs {
namespace Social {


// enumから文字列に変換
static NSString* const typeFromEnum(const Type type) {
  switch (type) {
  case TWITTER:
    return SLServiceTypeTwitter;

  case FACEBOOK:
    return SLServiceTypeFacebook;

  default:
    assert(0 && "invalid Type");
    return nil;
  }
}


// ツイート可能か返す
bool canPost(const Type type) {
  bool has_class = NSClassFromString(@"SLComposeViewController") ? true : false;
  
	return has_class && [SLComposeViewController isAvailableForServiceType:typeFromEnum(type)];
}

void post(const Type type,
          const std::string& text, UIImage* image,
          std::function<void()> complete_callback) {
  if (!canPost(type)) {
    complete_callback();
    return;
  }

  SLComposeViewController* viewController = [SLComposeViewController composeViewControllerForServiceType:typeFromEnum(type)];

  void (^completion) (SLComposeViewControllerResult result) = ^(SLComposeViewControllerResult result) {
    switch (result) {
    case SLComposeViewControllerResultCancelled:
    case SLComposeViewControllerResultDone:
      // TIPS:自分でdismissしないといけない(iOS6)
      [viewController dismissViewControllerAnimated:YES completion:nil];

      complete_callback();
    }
  };
  [viewController setCompletionHandler:completion];

	NSString* str = [[[NSString alloc] initWithCString:text.c_str() encoding:NSUTF8StringEncoding] autorelease];
  [viewController setInitialText:str];

  if (image) {
    [viewController addImage:image];
  }
  // [viewController addURL:[NSURL URLWithString:@"https://itunes.apple.com/us/app/uirou-lite/id697740169?mt=8"]];

  UIViewController* app_vc = ci::app::getWindow()->getNativeViewController();
  [app_vc presentViewController:viewController animated:YES completion:nil];
}


}
}
