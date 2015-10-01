//
// OpenGL ES で定義されていないVboMeshの描画
//

#if defined( CINDER_GLES )

#include "DrawVboMesh.hpp"


namespace cinder {
namespace gl {

void drawRange( const VboMesh &vbo, const GLsizei indexCount) noexcept {
	vbo.enableClientStates();
	vbo.bindAllData();
	
	glDrawElements( vbo.getPrimitiveType(), indexCount, GL_UNSIGNED_SHORT, 0 );
	
	gl::VboMesh::unbindBuffers();
	vbo.disableClientStates();
}

void drawArrays( const VboMesh &vbo, const GLsizei count ) noexcept {
	vbo.enableClientStates();
	vbo.bindAllData();
  
	glDrawArrays( vbo.getPrimitiveType(), 0, count );

	gl::VboMesh::unbindBuffers();
	vbo.disableClientStates();
}

void draw(const VboMesh &vbo) noexcept {
	if( vbo.getNumIndices() > 0 )
		drawRange( vbo, (GLsizei)vbo.getNumIndices() );
	else
		drawArrays( vbo, (GLsizei)vbo.getNumVertices() );
}

}
}

#endif
