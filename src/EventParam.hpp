#pragma once

//
// Eventで送る汎用パラメーター
//

#include <map>
#include <string>
#include "boost/any.hpp"


namespace ngs {

using EventParam = std::map<std::string, boost::any>;

bool hasKey(const EventParam& params, const std::string& key) noexcept {
  auto it = params.find(key);
  return it != std::end(params);
}

}
