//
// File関連の雑多な処理(iOS)
//

#import <UIKit/UIKit.h>
#include <string>
#include "FileUtil.hpp"


namespace ngs {

ci::fs::path getDocumentPath() noexcept {
  NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
  return ci::fs::path([[paths objectAtIndex:0] UTF8String]);
}

}
