#pragma once

//
// Stage Data読み書き
//

#include "Params.hpp"


namespace ngs { namespace StageData {

ci::JsonTree load(const std::string& path) noexcept {
#if defined (OBFUSCATION_STAGES)
  auto file_path = replaceFilenameExt(path, "data");
  return ci::JsonTree(TextCodec::load(Asset::fullPath(file_path)));
#else
  return ci::JsonTree(Asset::load(path));
#endif
}

#ifdef DEBUG

void convert(const ci::JsonTree& params) noexcept {
#if !defined (CINDER_COCOA_TOUCH)
  const auto& stages = Json::getArray<std::string>(params["game.stage_path"]);
  for (const auto& path : stages) {
    ci::JsonTree data(Asset::load(path));

    auto write_path = replaceFilenameExt(path, "data");
    Params::write(write_path, data);
  }
  
  {
    ci::JsonTree data(Asset::load("startline.json"));
    Params::write("startline.data", data);
  }
  {
    ci::JsonTree data(Asset::load("finishline.json"));
    Params::write("finishline.data", data);
  }
#endif
}

#endif

} }
