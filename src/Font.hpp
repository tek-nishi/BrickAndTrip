#pragma once

//
// FreeType2によるFontレンダリング
//
// VSの場合、文字リテラルがcp932になる->正しく処理できない
// なので、コード内に文字列を埋め込む場合は以下の指定が必要
// #pragma execution_character_set("utf-8")
//
// この場合、path指定にascii以外が入ると正しく処理されなくなる:D
//

#include <ft2build.h>
#include FT_FREETYPE_H
#include "cinder/Surface.h"
#include "cinder/ip/Fill.h"
#include "cinder/ip/Resize.h"
#include <boost/noncopyable.hpp>
#include "Utility.hpp"


// VS:FreeType2のライブラリのリンク指定
#if defined(CINDER_MSW)
#ifdef DEBUG
#pragma comment (lib, "freetype_d.lib")
#else
#pragma comment (lib, "freetype.lib")
#endif
#endif


namespace ngs {

class FontCreator : private boost::noncopyable {
  FT_Library library_;


public:
  FontCreator() {
    DOUT << "FontCreator()" << std::endl;
    
    auto error = FT_Init_FreeType(&library_);
    if (error) {
      DOUT << "error FT_Init_FreeType" << std::endl;
      throw;
    }
  }

  ~FontCreator() {
    DOUT << "~FontCreator()" << std::endl;

    FT_Done_FreeType(library_);
  }


  FT_Library handle() { return library_; }
  
};


class Font : private boost::noncopyable {
  FT_Face face_;

  int size_;
    
  int ascender_;
  int descender_;
  int font_width_;
  int font_height_;
  int font_edge_;


public:
  Font(const std::string& path, FontCreator& creator) {
    DOUT << "Font()" << std::endl;
    
    auto error = FT_New_Face(creator.handle(),
                             ci::app::getAssetPath(path).string().c_str(),
                             0,
                             &face_);
    if (error) {
      DOUT << "error FT_New_Face:" << path << std::endl;
      throw;
    }
  }

  ~Font() {
    DOUT << "~Font()" << std::endl;

    FT_Done_Face(face_);
  }

  void setSize(const int size) {
    size_ = size;

    // FT_Set_Pixel_Sizesは横サイズ基準でサイズを決めるので、
    // 縦横比から逆算する
    float font_aspect = face_->max_advance_width / float(face_->max_advance_height);
    // TIPS:まれに横がはみ出す状況があるのに対処(ちょっとだけ小さくする)
    int height = (size - 1) * font_aspect;
    FT_Set_Pixel_Sizes(face_, 0, height);

    ascender_    = face_->size->metrics.ascender >> 6;
    descender_   = face_->size->metrics.descender >> 6;
    font_width_  = face_->size->metrics.max_advance >> 6;
    font_height_ = ascender_ - descender_;
    font_edge_   = (face_->size->metrics.height >> 6) - font_height_;

    DOUT << "ascender:" << ascender_ << std::endl
         << "descender:" << descender_ << std::endl
         << "font_width:" << font_width_ << std::endl
         << "font_height:" << font_height_ << std::endl
         << "metrics.height:" << (face_->size->metrics.height >> 6) << std::endl
         << "font_edge:" << font_edge_ << std::endl
         << "size:" << size_ << std::endl;
  }

    
  // 一文字レンダリング
  ci::Surface8u rendering(const char32_t chara) {
    FT_Load_Char(face_, chara, FT_LOAD_RENDER);
    FT_GlyphSlot slot = face_->glyph;
    auto bitmap = slot->bitmap;
      
    ci::Surface8u surface(size_, size_, true);
    ci::ip::fill(&surface, ci::ColorAT<uint8_t>(255, 255, 255, 0));

    // font_heightがsurfaceサイズより大きい場合はascenderを調整
    int ascender = ascender_;
    if ((ascender_ - slot->bitmap_top - font_edge_ + int(bitmap.rows)) > size_) {
      ascender = size_ + slot->bitmap_top + font_edge_ - bitmap.rows;
    }

    // アルファチャンネルにのみ書き込む
    for (u_int iy = 0; iy < bitmap.rows; ++iy) {
      for (u_int ix = 0; ix < bitmap.width; ++ix) {
        surface.setPixel(ci::Vec2i(ix + slot->bitmap_left, iy + ascender - slot->bitmap_top - font_edge_),
                         ci::ColorA8u(255, 255, 255, bitmap.buffer[iy * bitmap.width + ix]));
      }
    }

    return surface;
  }

  // 文字列をレンダリング
  ci::Surface8u rendering(const std::string& text) {
    size_t chara_num = strlen(text);

    ci::Surface8u surface(size_ * chara_num, size_, true);
    ci::ip::fill(&surface, ci::ColorAT<uint8_t>(255, 255, 255, 0));

    FT_GlyphSlot slot = face_->glyph;
    int pen_x = 0;
    for (size_t ic = 0; ic < chara_num; ++ic) {
      char32_t chara = getCharactor(text, ic);
      FT_Load_Char(face_, chara, FT_LOAD_RENDER);
      auto bitmap = slot->bitmap;

      // font_heightがsurfaceサイズより大きい場合はascenderを調整
      int ascender = ascender_;
      if ((ascender_ - slot->bitmap_top - font_edge_ + int(bitmap.rows)) > size_) {
        ascender = size_ + slot->bitmap_top + font_edge_ - bitmap.rows;
      }
        
      // アルファチャンネルにのみ書き込む
      for (u_int iy = 0; iy < bitmap.rows; ++iy) {
        for (u_int ix = 0; ix < bitmap.width; ++ix) {
          surface.setPixel(ci::Vec2i(ix + slot->bitmap_left + pen_x, iy + ascender - slot->bitmap_top - font_edge_),
                           ci::ColorA8u(255, 255, 255, bitmap.buffer[iy * bitmap.width + ix]));
        }
      }
        
      pen_x += slot->advance.x >> 6;
    }

    // FIXME:リサイズはCinder任せ
    return ci::ip::resizeCopy(surface, ci::Area(0, 0, pen_x, size_), ci::Vec2i(size_, size_));
  }
  
};


}
