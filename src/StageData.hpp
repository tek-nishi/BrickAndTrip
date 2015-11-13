#pragma once

//
// Stage Data読み書き
//

#include "Params.hpp"


namespace ngs { namespace StageData {

ci::JsonTree load(const std::string& path) noexcept {
#if defined (OBFUSCATION_STAGES)
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

void convert(const ci::JsonTree& params) noexcept {
#if !defined (CINDER_COCOA_TOUCH)
  const auto& stages = Json::getArray<std::string>(params["game.stage_path"]);
  for (const auto& path : stages) {
    auto data = Json::readFromFile(path);

    auto write_path = replaceFilenameExt(path, "data");
    Params::write(write_path, data);
  }
  
  {
    auto data = Json::readFromFile("startline.json");
    Params::write("startline.data", data);
  }
  {
    auto data = Json::readFromFile("finishline.json");
    Params::write("finishline.data", data);
  }
#endif
}

} }
