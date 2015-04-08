#pragma once

//
// .obj保持
//

#include <boost/noncopyable.hpp>
#include "cinder/TriMesh.h"
#include "cinder/ObjLoader.h"
#include "cinder/gl/Vbo.h"


namespace ngs {

class Model : private boost::noncopyable {
  ci::gl::VboMeshRef mesh_;


public:
  Model(const std::string& path) {
    ci::ObjLoader loader(ci::app::loadAsset(path));

    ci::TriMesh mesh;
    loader.load(&mesh);

    mesh_ = ci::gl::VboMesh::create(mesh);
  }
  

  const ci::gl::VboMesh& mesh() const { return *mesh_; }
  

private:

  
  
};

}
