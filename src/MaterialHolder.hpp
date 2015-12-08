#pragma once

//
// Materialを名前で管理
//

#include "Material.hpp"
#include <boost/noncopyable.hpp>
#include <map>


namespace ngs {

class MaterialHolder : private boost::noncopyable {
  std::map<std::string, Material> materials_;
  

public:
  void add(const std::string& name, const ci::JsonTree& params) noexcept {
    materials_.emplace(std::piecewise_construct,
                       std::forward_as_tuple(name),
                       std::forward_as_tuple(params));
  }

  Material& get(const std::string& name) {
    auto it = materials_.find(name);
    if (it == std::end(materials_)) {
      // 見つからなかったら例外
      throw;
    }

    return it->second;
  }
  
  
private:


  
};

}
