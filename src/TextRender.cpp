//
// 文字列をgl::Textureにレンダリング
//

#include "TextRender.hpp"
#include "cinder/Text.h"
#include "cinder/Surface.h"
#include "cinder/ip/Fill.h"
#if defined(CINDER_COCOA_TOUCH)
#include <CoreText/CoreText.h>
#endif


namespace ngs {

#if defined(CINDER_COCOA_TOUCH)
// 文字列を２のべき乗のサイズ領域にレンダリング
// SOURCE:ci::renderStringPow2
static ci::Surface renderStringPow2(const std::string &str, const ci::Font &font,
                                    const ci::ColorA &color, ci::Vec2i* actualSize, float *baselineOffset) {
  const auto glyphs = font.getGlyphs(str);

  // advancesには次の文字を描画するための移動量が入る
  std::vector<CGSize> advances(glyphs.size());
  ::CTFontGetAdvancesForGlyphs(font.getCtFontRef(), kCTFontDefaultOrientation,
                               &glyphs[0], &advances[0], glyphs.size());

  // 描画用の位置情報を生成
  ::CGPoint current_pos = ::CGPointMake(0, 0);
  std::vector<::CGPoint> render_positions;
  for (size_t i = 0; i < advances.size(); ++i) {
    render_positions.push_back(current_pos);
    current_pos.x += advances[i].width;
  }
  
  ci::Vec2i pixelSize, pow2PixelSize;
  pixelSize = ci::Vec2i(ci::math<float>::ceil(current_pos.x), ci::math<float>::ceil(font.getAscent() + font.getDescent()));

	pow2PixelSize = ci::Vec2i( ci::nextPowerOf2( pixelSize.x ), ci::nextPowerOf2( pixelSize.y ) );
  ci::Surface result( pow2PixelSize.x, pow2PixelSize.y, true );
	::CGContextRef cgContext = ci::cocoa::createCgBitmapContext( result );
	ci::ip::fill( &result, ci::ColorA( 0, 0, 0, 0 ) );

  // 上下ひっくり返って描画された文字が、上端にくっつくように調整
  ::CGContextTranslateCTM(cgContext, 0, pow2PixelSize.y - pixelSize.y);
  
  ::CGContextSetFont( cgContext, font.getCgFontRef() );
	::CGContextSetFontSize( cgContext, font.getSize() );
  
	::CGContextSetTextDrawingMode( cgContext, kCGTextFill );
	::CGContextSetRGBFillColor( cgContext, color.r, color.g, color.b, color.a );
	::CGContextSetTextPosition( cgContext, 0, font.getDescent() + 1 );

  ::CGContextShowGlyphsAtPositions(cgContext, &glyphs[0], &render_positions[0], glyphs.size());
  
	if( baselineOffset )
		*baselineOffset = font.getAscent() - pixelSize.y;
	if( actualSize )
		*actualSize = pixelSize;
	
	::CGContextRelease( cgContext );
	return result;
}
#endif
  
// 文字列をレンダリング、Textureとして返却
// SOURCE: ci::gl::drawString()
TextTexture createTextureFromString(const std::string& str, const ci::ColorA& color, ci::Font& font) {
    
  float baselineOffset;
#if defined(CINDER_COCOA_TOUCH)
  // TIPS:textureのサイズは2のべき乗になっていなければならない
  ci::Vec2i actualSize;
  ci::Surface8u pow2Surface(ngs::renderStringPow2(str, font, color, &actualSize, &baselineOffset));
  auto tex = std::make_shared<ci::gl::Texture>(pow2Surface);

  tex->setCleanTexCoords(actualSize.x / (float)pow2Surface.getWidth(), actualSize.y / (float)pow2Surface.getHeight());
  baselineOffset += pow2Surface.getHeight();
#else
  auto tex = std::make_shared<ci::gl::Texture>(ci::renderString(str, font, color, &baselineOffset));
#endif

  tex->setFlipped(false);
  
  return std::make_pair(std::move(tex), baselineOffset);
}

}
