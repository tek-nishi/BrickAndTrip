
#import <UIKit/UIKit.h>
#include <string>
#include "FileUtil.hpp"


namespace ngs {

ci::fs::path getDocumentPath() {
  NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
  return ci::fs::path([[paths objectAtIndex:0] UTF8String]);
}

}
