#pragma once

//
// 立方体文字列表示
//

#include "cinder/Text.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Surface.h"
#include "cinder/Tween.h"
#include "CubeText.hpp"
#include "TextureFont.hpp"
#include "Model.hpp"
#include <map>


namespace ngs {
namespace CubeTextDrawer {

void drawCubeAndText(ci::gl::TextureRef texture,
                     const Model& cube, const Model& text,
                     const ci::Color& color, const ci::Color& text_color,
                     const ci::Vec3f& scale, const ci::Vec3f& offset) {
  ci::gl::color(color);
  ci::gl::draw(cube.mesh());

  ci::gl::color(text_color);

  ci::gl::translate(offset);
  ci::gl::scale(scale);
  
  ci::gl::enable(GL_BLEND);
  texture->enableAndBind();
  ci::gl::draw(text.mesh());

  texture->disable();
  ci::gl::disable(GL_BLEND);
}


void draw(const CubeText& cube_text,
          TextureFont& font,
          const Model& model_cube, const Model& model_text,
          const ci::Vec3f& pos,
          const ci::Vec3f& scale,
          const ci::Color& text_color,
          const ci::Color& base_color,
          const std::vector<ci::Anim<float> >& rotation = std::vector<ci::Anim<float> >()) {
  auto text_pos = pos;

  const float chara_size = cube_text.size();
  const float chara_spacing = cube_text.spacing();
  auto cube_size = ci::Vec3f(chara_size, chara_size, chara_size) * scale;
  
  const auto& text = cube_text.text();

  for (size_t i = 0; i < text.size(); ++i) {
    const auto& t = text[i];
    auto texture = font.getTextureFromString(t);

    ci::gl::pushModelView();

    // 立方体の左下を(0, 0)として位置を決める
    ci::gl::translate(text_pos + ci::Vec3f(chara_size / 2, chara_size / 2, -chara_size / 2));
    if (!rotation.empty()) {
      float t = (i & 1) ? -chara_size / 2 : chara_size / 2;

      ci::gl::translate(0, t, 0);

      // TIPS:gl::rotate(Quarf)は、内部でglRotatefを使っている
      //      この計算が正しく求まらない状況があるため、Quarf->Matrix
      //      にしている。これだと問題ない
      ci::Quatf rot(ci::Vec3f(1, 0, 0), rotation[i]);
      glMultMatrixf(rot.toMatrix44());

      ci::gl::translate(0, -t, 0);
    }
    ci::gl::scale(cube_size);
      
    drawCubeAndText(texture, model_cube, model_text,
                    base_color, text_color,
                    font.scale(), font.offset());
      
    ci::gl::popModelView();

    text_pos.x += chara_size + chara_spacing;
  }
}

}
}
