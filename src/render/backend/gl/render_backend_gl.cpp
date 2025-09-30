#include "shaders/quad_vs_glsl.h"
#include "shaders/quad_fs_glsl.h"

//
// Internal OpenGL backend helpers
//

struct R_GL_TextureFormat {
  GLenum internal;
  GLenum base;
};

function GLuint
r_gl_handle_from_texture(R_Texture *texture)
{
  GLuint gl_texture = (GLuint)IntFromPtr(texture);
  return gl_texture;
}

function R_GL_TextureFormat
r_gl_texture_format(R_TextureFormat fmt)
{
  R_GL_TextureFormat result = {0};
  
  switch (fmt) {
    case R_TextureFormat_R8: {
      result.internal = GL_R8;
      result.base = GL_RED;
    }break;
    case R_TextureFormat_RGB8: { 
      result.internal = GL_RGB8;
      result.base = GL_RGB;
    }break;
    case R_TextureFormat_RGBA8: { 
      result.internal = GL_RGBA8; 
      result.base = GL_RGBA;
    }break;
  }
  
  return result; 
}

//
// OpenGL backend initialization
//

function void 
r_gl_backend_init(void)
{
  // Allocate memory for OpenGL backend state
  Arena *arena = arena_alloc_default();
  r_gl_backend = ArenaPushStruct(arena, R_GL_Backend);
  r_gl_backend->arena = arena; 
  
  // Allocate texture array and create fallback texture
  {
    GLuint texture; 
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    local U8 fallback_data[] = {
      0xFF,0xFF,0xFF,0xFF,
      0xFF,0xFF,0xFF,0xFF,
      0xFF,0xFF,0xFF,0xFF,
      0xFF,0xFF,0xFF,0xFF,
    };
    // TODO: Make RGBA8
    glTextureStorage2D(texture, 1, GL_RGB8, 2, 2);
    glTextureSubImage2D(texture, 0, 0, 0, 2, 2, GL_RGB, 
                        GL_UNSIGNED_BYTE, fallback_data);
    
    r_gl_backend->fallback_texture = texture; 
  }
  
  // Compile shaders used for rendering textured quads
  GLuint vshader = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &quad_vs_glsl);
  GLuint fshader = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &quad_fs_glsl);
  
  GLint linked;
  glGetProgramiv(vshader, GL_LINK_STATUS, &linked);
  if (!linked) {
    char message[1024];
    glGetProgramInfoLog(vshader, sizeof(message), 0, message);
  }
  Assert(linked);
  
  glGetProgramiv(fshader, GL_LINK_STATUS, &linked);
  if (!linked) {
    char message[1024];
    glGetProgramInfoLog(fshader, sizeof(message), 0, message);
  }
  Assert(linked);
  
  // Create a shader program pipeline for the OpenGL backend
  GLuint pipeline;
  glGenProgramPipelines(1, &pipeline);
  glUseProgramStages(pipeline, GL_VERTEX_SHADER_BIT, vshader);
  glUseProgramStages(pipeline, GL_FRAGMENT_SHADER_BIT, fshader);
  
  // Create vertex array for rendering textured quads
  GLuint vbo;
  glCreateBuffers(1, &vbo);
  
  U32 buff_idx = 0;
  
  GLuint vao;
  glCreateVertexArrays(1, &vao);
  glVertexArrayVertexBuffer(vao, buff_idx, vbo, 0, sizeof(R_Quad)); 
  
  S32 a_rect = 0; // Rect 
  glVertexArrayAttribFormat(vao, a_rect, 4, GL_FLOAT, GL_FALSE, 
                            (GLuint)OffsetOf(R_Quad, rect));
  glVertexArrayAttribBinding(vao, a_rect, buff_idx);
  glEnableVertexArrayAttrib(vao, a_rect);
  glVertexArrayBindingDivisor(vao, a_rect, 1);
  
  S32 a_uv_rect = 1; // UV coordinates
  glVertexArrayAttribFormat(vao, a_uv_rect, 4, GL_FLOAT, GL_FALSE, 
                            (GLuint)OffsetOf(R_Quad, uv_rect));
  glVertexArrayAttribBinding(vao, a_uv_rect, buff_idx);
  glEnableVertexArrayAttrib(vao, a_uv_rect);
  glVertexArrayBindingDivisor(vao, a_uv_rect, 1);
  
  S32 a_c0 = 2; // Left color
  glVertexArrayAttribFormat(vao, a_c0, 4, GL_FLOAT, GL_FALSE, 
                            (GLuint)OffsetOf(R_Quad, colors[0]));
  glVertexArrayAttribBinding(vao, a_c0, buff_idx);
  glEnableVertexArrayAttrib(vao, a_c0);
  glVertexArrayBindingDivisor(vao, a_c0, 1);
  
  S32 a_c1 = 3; // Right color
  glVertexArrayAttribFormat(vao, a_c1, 4, GL_FLOAT, GL_FALSE, 
                            (GLuint)OffsetOf(R_Quad, colors[1]));
  glVertexArrayAttribBinding(vao, a_c1, buff_idx);
  glEnableVertexArrayAttrib(vao, a_c1);
  glVertexArrayBindingDivisor(vao, a_c1, 1);
  
  S32 a_c2 = 4; // Bottom color
  glVertexArrayAttribFormat(vao, a_c2, 4, GL_FLOAT, GL_FALSE, 
                            (GLuint)OffsetOf(R_Quad, colors[2]));
  glVertexArrayAttribBinding(vao, a_c2, buff_idx);
  glEnableVertexArrayAttrib(vao, a_c2);
  glVertexArrayBindingDivisor(vao, a_c2, 1);
  
  S32 a_c3 = 5; // Top Color
  glVertexArrayAttribFormat(vao, a_c3, 4, GL_FLOAT, GL_FALSE, 
                            (GLuint)OffsetOf(R_Quad, colors[3]));
  glVertexArrayAttribBinding(vao, a_c3, buff_idx);
  glEnableVertexArrayAttrib(vao, a_c3);
  glVertexArrayBindingDivisor(vao, a_c3, 1);
  
  S32 a_theta = 6; // Theta (radians)
  glVertexArrayAttribFormat(vao, a_theta, 1, GL_FLOAT, GL_FALSE, 
                            (GLuint)OffsetOf(R_Quad, theta));
  glVertexArrayAttribBinding(vao, a_theta, buff_idx);
  glEnableVertexArrayAttrib(vao, a_theta);
  glVertexArrayBindingDivisor(vao, a_theta, 1);
  
  S32 a_radius = 7; // Border radius
  glVertexArrayAttribFormat(vao, a_radius, 1, GL_FLOAT, GL_FALSE, 
                            (GLuint)OffsetOf(R_Quad, radius));
  glVertexArrayAttribBinding(vao, a_radius, buff_idx);
  glEnableVertexArrayAttrib(vao, a_radius);
  glVertexArrayBindingDivisor(vao, a_radius, 1);
  
  S32 a_sample_mode = 8; // Sampling mode (0: sample rgb color; 
  // 1: sample alpha from red channel)
  glVertexArrayAttribFormat(vao, a_sample_mode, 1, GL_INT, GL_FALSE, 
                            (GLuint)OffsetOf(R_Quad, sample_mode));
  glVertexArrayAttribBinding(vao, a_sample_mode, buff_idx);
  glEnableVertexArrayAttrib(vao, a_sample_mode);
  glVertexArrayBindingDivisor(vao, a_sample_mode, 1);
  
  S32 a_border_thickness = 9; // Border thickness
  glVertexArrayAttribFormat(vao, a_border_thickness, 1, GL_FLOAT, GL_FALSE, 
                            (GLuint)OffsetOf(R_Quad, border_thickness));
  glVertexArrayAttribBinding(vao, a_border_thickness, buff_idx);
  glEnableVertexArrayAttrib(vao, a_border_thickness);
  glVertexArrayBindingDivisor(vao, a_border_thickness, 1);
  
  S32 a_corner_softness = 10; // Corner softness
  glVertexArrayAttribFormat(vao, a_corner_softness, 1, GL_FLOAT, GL_FALSE, 
                            (GLuint)OffsetOf(R_Quad, corner_softness));
  glVertexArrayAttribBinding(vao, a_corner_softness, buff_idx);
  glEnableVertexArrayAttrib(vao, a_corner_softness);
  glVertexArrayBindingDivisor(vao, a_corner_softness, 1);
  
  S32 a_clip_rect = 11; // Clipping rectangle
  glVertexArrayAttribFormat(vao, a_clip_rect, 4, GL_FLOAT, GL_FALSE, 
                            (GLuint)OffsetOf(R_Quad, clip_rect));
  glVertexArrayAttribBinding(vao, a_clip_rect, buff_idx);
  glEnableVertexArrayAttrib(vao, a_clip_rect);
  glVertexArrayBindingDivisor(vao, a_clip_rect, 1);
  
  r_gl_backend->pipeline = pipeline;
  r_gl_backend->quad_vs  = vshader;
  r_gl_backend->quad_fs  = fshader;
  r_gl_backend->quad_vbo = vbo;
  r_gl_backend->quad_vao = vao;
  
  // Set persistent per-frame OpenGL state
  glClearColor(0.05f, 0.05f, 0.05f, 1);
  glEnable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

//
// OpenGL implementation of the rendering backend API
//

function R_Texture *
r_backend_texture_create_in_place(void *data, U32 width, U32 height, 
                                  R_TextureFormat fmt, U32 pixel_align, B32 gen_mips)
{
  R_Texture *result = 0;
  
  R_GL_TextureFormat gl_fmt = r_gl_texture_format(fmt);
  
  GLuint gl_texture;
  glCreateTextures(GL_TEXTURE_2D, 1, &gl_texture);
  
  glTextureParameteri(gl_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTextureParameteri(gl_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glPixelStorei(GL_UNPACK_ALIGNMENT, pixel_align);
  // TODO: Should provide a way to change which min/mag hardware filters are used
  // separately. 
  if (gen_mips) {
    glGenerateTextureMipmap(gl_texture); 
    glTextureParameteri(gl_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
    glTextureParameteri(gl_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }
  else {
    glTextureParameteri(gl_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
    glTextureParameteri(gl_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
  }
  
  glTextureStorage2D(gl_texture, 1, gl_fmt.internal, width, height);
  if (glGetError() && gl_texture != 0) {
    glDeleteTextures(1, &gl_texture);
    gl_texture = 0;
  }
  else {
    glTextureSubImage2D(gl_texture, 0, 0, 0, width, height, gl_fmt.base, 
                        GL_UNSIGNED_BYTE, data);
  }
  
  result = (R_Texture *)PtrFromInt(gl_texture);
  return result; 
}

function R_Texture *
r_backend_texture_create_impl(U32 width, U32 height, R_TextureFormat fmt, 
                              U32 pixel_align, B32 gen_mips)
{
  R_Texture *result = 0;
  
  R_GL_TextureFormat gl_fmt = r_gl_texture_format(fmt);
  
  GLuint gl_texture;
  glCreateTextures(GL_TEXTURE_2D, 1, &gl_texture);
  
  glTextureParameteri(gl_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTextureParameteri(gl_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glPixelStorei(GL_UNPACK_ALIGNMENT, pixel_align);
  // TODO: Should provide a way to change which min/mag hardware filters are used
  // separately. 
  if (gen_mips) {
    glGenerateTextureMipmap(gl_texture); 
    glTextureParameteri(gl_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
    glTextureParameteri(gl_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }
  else {
    glTextureParameteri(gl_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
    glTextureParameteri(gl_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
  }
  
  glTextureStorage2D(gl_texture, 1, gl_fmt.internal, width, height);
  if (glGetError() && gl_texture != 0) {
    glDeleteTextures(1, &gl_texture);
    gl_texture = r_gl_fallback_texture();
  }
  
  result = (R_Texture *)PtrFromInt(gl_texture);
  return result; 
}

function void 
r_backend_texture_update(R_Texture *texture, void *data, U32 x, U32 y, U32 width, 
                         U32 height, R_TextureFormat fmt)
{
  if (texture) {
    R_GL_TextureFormat gl_fmt = r_gl_texture_format(fmt);
    GLuint gl_texture = r_gl_handle_from_texture(texture);
    
    glTextureSubImage2D(gl_texture, 0, x, y, width, height, gl_fmt.base, 
                        GL_UNSIGNED_BYTE, data);
  }
}

function void 
r_backend_texture_delete(R_Texture **texture)
{
  if (texture) {
    GLuint gl_texture = r_gl_handle_from_texture(*texture);
    if (gl_texture != 0) {
      glDeleteTextures(1, &gl_texture);
      *texture = 0;
    }
  }
}

function void 
r_backend_set_viewport(RectU32 rect)
{
  glViewport(rect.x0, rect.y0, rect.x1, rect.y1);
  
  V2U32 res_u32 = rect_u32_dim(rect); 
  V2F32 res = v2f32((F32)res_u32.x,(F32)res_u32.y);
  GLuint quad_vs = r_gl_backend->quad_vs; 
  glProgramUniform2fv(quad_vs, 0, 1, &res.e[0]);
}

function void 
r_backend_set_clear_color(V4F32 color)
{
  glClearColor(color.r,color.g,color.b,color.a);
}

function void 
r_backend_begin_frame(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

function void 
r_backend_submit_batch(R_Batch *batch)
{
  GLuint texture = r_gl_handle_from_texture(batch->texture);
  // TODO: No longer necessary since you're defaulting to fallback texture
  if (!glIsTexture(texture)) {
    texture = r_gl_fallback_texture();
  }
  // TODO: This shouldn't be done every batch. It only needs to be done
  // before all the batches are submitted! Put it in r_backend_begin_frame().
  // Also, backend_end_frame should revert these state changes!
  GLuint vbo = r_gl_backend->quad_vbo;
  GLuint vao = r_gl_backend->quad_vao;
  GLuint pipeline = r_gl_backend->pipeline;
  
  glBindProgramPipeline(pipeline);
  glBindTextureUnit(0, texture);
  glBindVertexArray(vao);
  
  // Allocate an adequately-sized buffer on the GPU 
  // TODO: Stop allocating buffer every frame!!!
  U32 quad_count_total = batch->quad_count_total; 
  U64 data_size = sizeof(R_Quad) * quad_count_total;
  glNamedBufferData(vbo, data_size, 0, GL_STREAM_DRAW);
  
  // Populate the vertex buffer with batch's quads 
  U64 pos = 0;
  for (R_QuadChunk *chunk = batch->chunk_first; chunk != 0; chunk = chunk->next) {
    U64 size = chunk->quad_count * sizeof(R_Quad);
    glNamedBufferSubData(vbo, pos, size, chunk->quads);
    pos += size; 
  }
  
  glDrawArraysInstanced(GL_TRIANGLES, 0, 6, quad_count_total);
  glBindVertexArray(0);
}
