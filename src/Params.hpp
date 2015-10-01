#pragma once

//
// 各種パラメーター
//   ci::JsonTreeで保持すればよいのだが
//   難読化のために必要な関数を用意

#include "TextCodec.hpp"
#include "JsonUtil.hpp"
#include <sstream>


namespace ngs {
namespace Params {

ci::JsonTree load(const std::string& path) noexcept {
#if defined (OBFUSCATION_PARAMS)
#if defined (DEBUG) && defined (CINDER_MAC)
  // DEBUG時、OSXはプロジェクトの場所からfileを読み込む
  boost::filesystem::path full_path(std::string(PREPRO_TO_STR(SRCROOT) "../assets/") + path);
#else
  auto full_path = ci::app::getAssetPath(path);
#endif
  auto load_path = replaceFilenameExt(full_path.string(), "data");
  DOUT << "Params path:" << load_path << std::endl;

  return ci::JsonTree(TextCodec::load(load_path));
#else
  return Json::readFromFile(path);
#endif
}

#ifdef DEBUG

void write(const std::string& path, const ci::JsonTree& params) noexcept {
#if defined (CINDER_MAC)
  boost::filesystem::path full_path(std::string(PREPRO_TO_STR(SRCROOT) "../assets/") + path);
#else
  auto full_path = ci::app::getAssetPath(path);
#endif
  TextCodec::write(full_path.string(), params.serialize());
}

void convert(const std::string& path) noexcept {
#if !defined (CINDER_COCOA_TOUCH)
  auto params = Json::readFromFile(path);

  auto write_path = replaceFilenameExt(path, "data");
  write(write_path, params);
#endif
}

#endif

}
}
