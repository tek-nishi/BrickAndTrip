﻿#pragma once

//
// TextureFontを格納、名前で取得
//

#include "TextureFont.hpp"
#include <map>
#include <boost/noncopyable.hpp>


namespace ngs {

class FontHolder : private boost::noncopyable {
  FontCreator font_creator_;

  std::map<std::string, TextureFont> fonts_;

  std::string default_font_name_;


public:
  TextureFont& getFont(const std::string& name) noexcept {
    auto it = fonts_.find(name);
    if (it == std::end(fonts_)) {
      // 見つからなかった場合は標準名のFontを探す
      it = fonts_.find(default_font_name_);
      assert(it != std::end(fonts_));
    }

    return it->second;
  }

  
  TextureFont& addFont(const std::string& name, const std::string& path,
                       const int size,
                       const ci::Vec3f& scale, const ci::Vec3f& offset,
                       const bool mipmap = true) noexcept {
    auto result = fonts_.emplace(std::piecewise_construct,
                                 std::forward_as_tuple(name),
                                 std::forward_as_tuple(path, font_creator_, size, scale, offset, mipmap));

    assert(result.second);
    
    return result.first->second;
  }

  void setDefaultFont(const std::string& name) noexcept {
    // TIPS:名前が見つからない場合は例外で止まる
    fonts_.at(name);
    default_font_name_ = name;
  }
  

private:
  

  
};

}
