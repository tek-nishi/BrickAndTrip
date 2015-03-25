#pragma once

//
// Eventで送る汎用パラメーター
//

#include <map>
#include <string>
#include "boost/any.hpp"


namespace ngs {

using EventParam = std::map<std::string, boost::any>;

}
