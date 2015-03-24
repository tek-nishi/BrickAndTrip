#pragma once

//
// 文字列をgl::Textureにレンダリング
//


#include <string>
#include <utility>
#include "cinder/Cinder.h"
#include "cinder/Font.h"
#include "cinder/gl/Texture.h"


namespace ngs {

using TextTexture = std::pair<ci::gl::TextureRef, float>;

TextTexture createTextureFromString(const std::string& str, const ci::ColorA& color, ci::Font& font);

}
