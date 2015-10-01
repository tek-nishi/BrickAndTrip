//
// ローカライズ済み文字列を取り出す
//


#import "Localize.h" 


namespace ngs {

// 投稿用の文字列(ローカライズ済み)を取得
std::string localizedString(const std::string& key) noexcept {
  NSString* text;

  NSString* key_text = [NSString stringWithUTF8String:key.c_str()];
  
  text = NSLocalizedString(key_text, nil);
  return [text UTF8String];
}

}
