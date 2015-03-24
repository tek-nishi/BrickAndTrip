#pragma once

//
// TextureFontを格納、名前で取得
//

#include "TextureFont.hpp"
#include <map>


namespace ngs {

class FontHolder {
  std::map<std::string, TextureFont> fonts_;

  std::string default_font_name_;


public:
  FontHolder() = default;

  
  TextureFont& getFont(const std::string& name) {
    auto it = fonts_.find(name);
    if (it == std::end(fonts_)) {
      // 見つからなかった場合は標準名のFontを探す
      it = fonts_.find(default_font_name_);
      assert(it != std::end(fonts_));
    }

    return it->second;
  }

  void addFont(const std::string& name, const std::string& path, const float size) {
    fonts_.emplace(name, TextureFont(path, size));
  }

  void setDefaultFont(const std::string& name) {
    // TIPS:名前が見つからない場合は例外で止まる
    fonts_.at(name);
    default_font_name_ = name;
  }


private:
  

  
};

}
