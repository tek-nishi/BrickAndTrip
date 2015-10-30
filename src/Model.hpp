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

  std::vector<int> group_face_;


public:
  Model(const std::string& path,
        const bool has_normals = true, const bool has_uv = true,
        const bool has_indics = true) {
    ci::ObjLoader loader(ci::app::loadAsset(path));

    ci::TriMesh mesh;
    loader.load(&mesh);

    //glDrawArrays向けのデータ
    if (!has_indics) {
      ci::TriMesh mesh_new;
      
      const auto& vertices = mesh.getVertices();
      const auto& normals  = mesh.getNormals();
      const auto& coords   = mesh.getTexCoords();
      const auto& indices  = mesh.getIndices();

      // FIXME:Cheetah3Dの出力する.objは
      // 法線もUVも書き出すので手抜き
      // bool has_normals = !normals.empty();
      // bool has_coords  = !coords.empty();

      for (const auto index : indices) {
        mesh_new.appendVertex(vertices[index]);
        mesh_new.appendNormal(normals[index]);
        mesh_new.appendTexCoord(coords[index]);
      }
      
      std::swap(mesh, mesh_new);
    }
    
    if (!has_normals) {
      mesh.getNormals().clear();
    }
    if (!has_uv) {
      mesh.getTexCoords().clear();
    }

    const auto& groups = loader.getGroups();
    
    DOUT << "model:" << path
         << " v:" << mesh.getVertices().size()
         << " n:" << mesh.getNormals().size()
         << " t:" << mesh.getTexCoords().size()
         << " i:" << mesh.getIndices().size()
         << " g:" << groups.size()
         << std::endl;

    for (const auto& g : groups) {
      group_face_.push_back(int(g.mFaces.size()));
      DOUT << "face:"
           <<  g.mFaces.size()
           << std::endl;
    }
    
    mesh_ = ci::gl::VboMesh::create(mesh);
  }
  

  const ci::gl::VboMesh& mesh() const noexcept { return *mesh_; }
  int getGroupFaces(const size_t index) const noexcept { return group_face_[index]; }
  

private:

  
  
};

}
