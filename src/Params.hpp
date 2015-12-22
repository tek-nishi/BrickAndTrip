#pragma once

//
// 各種パラメーター
//   ci::JsonTreeで保持すればよいのだが
//   難読化のために必要な関数を用意

#include "TextCodec.hpp"
#include "JsonUtil.hpp"
#include <sstream>


namespace ngs { namespace Params {

ci::JsonTree load(const std::string& path) noexcept {
#if defined (OBFUSCATION_PARAMS)
  auto file_path = replaceFilenameExt(path, "data");
  return ci::JsonTree(TextCodec::load(Asset::fullPath(file_path)));
#else
  return ci::JsonTree(Asset::load(path));
#endif
}

#ifdef DEBUG

void write(const std::string& path, const ci::JsonTree& params) noexcept {
#if !defined (CINDER_COCOA_TOUCH)
  TextCodec::write(Asset::fullPath(path), params.serialize());
#endif
}

void convert(const std::string& path) noexcept {
#if !defined (CINDER_COCOA_TOUCH)
  ci::JsonTree params(Asset::load(path));

  auto write_path = replaceFilenameExt(path, "data");
  write(write_path, params);
#endif
}

#endif

} }
