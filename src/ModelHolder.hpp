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


  void add(const std::string& name, const std::string& path,
           const bool has_normals = true, const bool has_uvs = true,
           const bool has_indices = true) {
    models_.emplace(std::piecewise_construct,
                    std::forward_as_tuple(name),
                    std::forward_as_tuple(path, has_normals, has_uvs, has_indices));
  }

  
  const Model& get(const std::string& name) const {
    return models_.at(name);
  }

  
private:


  
};

}
