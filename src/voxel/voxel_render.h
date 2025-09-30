#pragma once

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_3.h>
#include <d3dcompiler.h>
#include <dxgidebug.h>

#pragma comment(lib, "gdi32")
#pragma comment(lib, "user32")
#pragma comment(lib, "dxguid")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

struct VOX_Vertex {
  V2F32 position;
  V2F32 texcoord;
};

struct VOX_UniformData {
  // @Todo: fov, view angle instead of view, orbit radius/distance, view height (ray_y)
  V2F32 client_size;
  F32 pad0[2];
  V2F32 view; 
  F32 pad1[2];
  V4F32 mouse; // x-position, y-position, left btn (0/1), right btn (0/1)
  F32 zoom = 1;
  F32 time;
  F32 pad3[2];
};

struct VOX_Renderer {
  Arena *arena;
  
  OS_Handle window;
  V2U32 client_size;
  String8 shader_path;
  U32 shader_compile_flags;
  
  ID3D11Device *device;
  ID3D11DeviceContext *context;
  IDXGISwapChain1 *swapchain;
  
  ID3D11Buffer *vertex_buffer;
  ID3D11InputLayout *input_layout;
  
  ID3D11Buffer *constant_buffer;
  
  // @Note: Starting with one chunk for now
  ID3D11Texture3D *chunk_texture;
  ID3D11ShaderResourceView *chunk_texture_view; 
  ID3D11SamplerState *chunk_texture_sampler;
  
  ID3D11VertexShader *vertex_shader;
  ID3D11PixelShader *pixel_shader;
  
  ID3D11BlendState *blend_state;
  ID3D11RasterizerState *rasterizer_state;
  ID3D11DepthStencilState *depth_state;
  
  ID3D11RenderTargetView *rt_view;
  ID3D11DepthStencilView *ds_view; 
};

function VOX_Renderer *vox_render_alloc(void);
function void vox_render_release(VOX_Renderer *r);

function void vox_render_init(VOX_Renderer *r, String8 shader_path, OS_Handle window);

#if 0
function void vox_render_reload_shader(VOX_Renderer *r);

function void vox_render_begin_frame(VOX_Renderer *r); 
function void vox_render_draw(VOX_Renderer *r, VOX_UniformData *uniforms);
function void vox_render_end_frame(VOX_Renderer *r);
#endif
