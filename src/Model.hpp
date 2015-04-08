#pragma once

//
// .objの表示
//

#include <boost/noncopyable.hpp>
#include "cinder/TriMesh.h"
#include "cinder/ObjLoader.h"


namespace ngs {

class Model : private boost::noncopyable {
  ci::TriMesh mesh_;


public:
  Model(const std::string& path) {
    ci::ObjLoader loader(ci::app::loadAsset(path));
    loader.load(&mesh_);
  }
  

  const ci::TriMesh& mesh() const { return mesh_; }
  

private:

  
  
};

}
