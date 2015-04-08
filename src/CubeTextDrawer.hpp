#pragma once

//
// 立方体文字列表示
//

#include "cinder/Font.h"
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


#if 0

void drawFontRect(ci::gl::TextureRef texture, const ci::Rectf &rect, const float z, const float base_line) {
  texture->enableAndBind();
  glEnableClientState( GL_VERTEX_ARRAY );
  glEnableClientState( GL_TEXTURE_COORD_ARRAY );

  GLfloat verts[3 * 4];
  glVertexPointer( 3, GL_FLOAT, 0, verts );

  GLfloat texCoords[2 * 4];
  glTexCoordPointer( 2, GL_FLOAT, 0, texCoords );

#if defined(CINDER_MSW)
  // baseLineを足してやる必要がある
  float v_ofs = (base_line - 1) / texture->getHeight();
#else
  float v_ofs = 0.0f;
#endif
    
  verts[0*3+0] = rect.getX2(); texCoords[0*2+0] = texture->getMaxU();
  verts[0*3+1] = rect.getY1(); texCoords[0*2+1] = 0 + v_ofs;
  verts[0*3+2] = z;
  verts[1*3+0] = rect.getX1(); texCoords[1*2+0] = 0;
  verts[1*3+1] = rect.getY1(); texCoords[1*2+1] = 0 + v_ofs;
  verts[1*3+2] = z;
  verts[2*3+0] = rect.getX2(); texCoords[2*2+0] = texture->getMaxU();
  verts[2*3+1] = rect.getY2(); texCoords[2*2+1] = texture->getMaxV() + v_ofs;
  verts[2*3+2] = z;
  verts[3*3+0] = rect.getX1(); texCoords[3*2+0] = 0;
  verts[3*3+1] = rect.getY2(); texCoords[3*2+1] = texture->getMaxV() + v_ofs;
  verts[3*3+2] = z;

  glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

  glDisableClientState( GL_VERTEX_ARRAY );
  glDisableClientState( GL_TEXTURE_COORD_ARRAY );	
  texture->disable();
}

#endif


void drawCubeAndText(TextTexture& texture_info,
                     Model& cube, Model& text,
                     const ci::Color& color, const ci::Color& text_color) {
  ci::gl::enable(GL_LIGHTING);
  ci::gl::enableDepthRead();
  ci::gl::enableDepthWrite();

  ci::gl::color(color);

  ci::gl::draw(cube.mesh());

  ci::gl::disable(GL_LIGHTING);
  ci::gl::enableDepthRead(false);
  ci::gl::enableDepthWrite(false);

  ci::gl::color(text_color);

  texture_info.first->enableAndBind();
  ci::gl::draw(text.mesh());
  texture_info.first->disable();
}


void draw(const CubeText& cube_text,
          TextureFont& font,
          Model& model_cube, Model& model_text,
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
    auto texture_info = font.getTextureFromString(t.chara);

    if (t.disp) {
      ci::gl::pushModelView();

      // 立方体の左下を(0, 0)として位置を決める
      ci::gl::translate(text_pos + ci::Vec3f(chara_size / 2, chara_size / 2, -chara_size / 2));
      if (!rotation.empty()) {
        ci::gl::rotate(ci::Quatf(ci::Vec3f(1, 0, 0), std::fmod<float>(rotation[i], M_PI * 2.0f)));
        // ci::gl::rotate(ci::Quatf(ci::Vec3f(1, 0, 0), rotation[i]));
      }
      ci::gl::scale(cube_size);
      
      drawCubeAndText(texture_info, model_cube, model_text, base_color, text_color);
      
      ci::gl::popModelView();
    }

    text_pos.x += chara_size + chara_spacing;
  }
}

}
}
