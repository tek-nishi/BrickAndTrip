#pragma once

//
// テクスチャ化フォント
// キャッシュ管理付き
//

#include <map>
#include "TextRender.hpp"


namespace ngs {

class TextureFont {
  ci::Font font_;
  std::map<std::string, TextTexture> texture_cache_;


public:
 TextureFont() = default;
  
  explicit TextureFont(const std::string& path, const float size) :
    font_(ci::app::loadAsset(path), size)
  {}

  
  // 文字列から生成したテクスチャを取得
  TextTexture getTextureFromString(const std::string& str) {
    // キャッシュ済みなら、それを返却
    auto cached = isCachedTexture(str);
    if (cached.first) {
      return cached.second->second;
    }

    auto texture  = createTextureFromString(str, ci::ColorA::white(), font_);
    auto inserted = texture_cache_.emplace(str, std::move(texture));

    return inserted.first->second;
  }

  
private:
  // キャッシュされている文字テクスチャを返却
  std::pair<bool, std::map<std::string, TextTexture>::iterator> isCachedTexture(const std::string& str) {
    auto it = texture_cache_.find(str);
    return std::make_pair(it != std::end(texture_cache_), it);
  }
  
};

}
