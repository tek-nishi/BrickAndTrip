#pragma once

//
// テクスチャ化フォント
// キャッシュ管理付き
//

#include <map>
#include <boost/noncopyable.hpp>
#include "cinder/gl/Texture.h"
#include "Font.hpp"


namespace ngs {

class TextureFont : private boost::noncopyable {
  Font font_;

  ci::Vec3f scale_;
  ci::Vec3f offset_;
  
  std::map<std::string, ci::gl::TextureRef> texture_cache_;


public:
  explicit TextureFont(const std::string& path, FontCreator& creator,
                       const int size,
                       const ci::Vec3f& scale, const ci::Vec3f& offset) :
    font_(path, creator),
    scale_(scale),
    offset_(offset)
  {
    font_.setSize(size);
  }

  
  // 文字列から生成したテクスチャを取得
  ci::gl::TextureRef getTextureFromString(const std::string& str) noexcept {
    // キャッシュ済みなら、それを返却
    auto cached = isCachedTexture(str);
    if (cached.first) {
      return cached.second->second;
    }

    // TIPS:文字はmipmapを利用(縮小時の見栄えが良くなる)
    ci::gl::Texture::Format format;
    format.enableMipmapping();
    
    auto texture  = ci::gl::Texture::create(font_.rendering(str), format);
    texture->setMinFilter(GL_LINEAR_MIPMAP_NEAREST);
    texture->setMagFilter(GL_LINEAR);
    
    auto inserted = texture_cache_.emplace(str, std::move(texture));

    return inserted.first->second;
  }

  const ci::Vec3f& scale() const noexcept { return scale_; }
  const ci::Vec3f& offset() const noexcept { return offset_; }

  
private:
  // キャッシュされている文字テクスチャを返却
  std::pair<bool, std::map<std::string, ci::gl::TextureRef>::iterator> isCachedTexture(const std::string& str) noexcept {
    auto it = texture_cache_.find(str);
    return std::make_pair(it != std::end(texture_cache_), it);
  }
  
};

}
