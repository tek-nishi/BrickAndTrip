#pragma once

//
// Modelを名前で管理
//

#include "Model.hpp"
#include <boost/noncopyable.hpp>
#include <map>


namespace ngs {

class ModelHolder : private boost::noncopyable {
  std::map<std::string, Model> models_;
  

public:
  ModelHolder() = default;


  void add(const std::string& name, const std::string& path) {
    models_.emplace(std::piecewise_construct,
                    std::forward_as_tuple(name),
                    std::forward_as_tuple(path));
  }

  const Model& get(const std::string& name) {
    auto it = models_.find(name);
    if (it == std::end(models_)) {
      // 見つからなかったら例外
      throw;
    }

    return it->second;
  }

  
private:


  
};

}
