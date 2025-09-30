#pragma once

#include <GL/gl.h>
#include "glcorearb.h"

//
// OpenGL backend context
//

struct R_GL_Backend {
  Arena *arena; 
  
  GLuint quad_vbo;
  GLuint quad_vao; 
  
  GLuint pipeline;
  GLuint quad_vs;
  GLuint quad_fs; 
  
  GLuint fallback_texture; 
};

global R_GL_Backend *r_gl_backend;

function void r_gl_backend_init(void);
#define r_gl_fallback_texture() r_gl_backend->fallback_texture
