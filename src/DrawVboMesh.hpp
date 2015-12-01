#pragma once

//
// OpenGL ES で定義されていないVboMeshの描画
//

#if defined( CINDER_GLES )

#include <cinder/gl/Vbo.h>


namespace cinder { namespace gl {

void draw(const VboMesh& vbo) noexcept;

} }

#endif
