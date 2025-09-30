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

struct R_D3D11_Backend {
  Arena *arena; 
  
  ID3D11Device *device;
  ID3D11DeviceContext *context;
  IDXGISwapChain1 *swap_chain;
  
  ID3D11Buffer *quad_vbuff;
  ID3D11InputLayout *quad_layout;
  ID3D11VertexShader *quad_vs;
  ID3D11PixelShader *quad_ps;
  
  ID3D11SamplerState *sampler;
  ID3D11ShaderResourceView *fallback_texture_view; 
  
  ID3D11BlendState *blend_state;
  ID3D11RasterizerState *rasterizer_state;
  ID3D11DepthStencilState *depth_state;
  
  ID3D11RenderTargetView *rt_view;
  ID3D11DepthStencilView *ds_view; 
  
  // NOTE: Temp 
  V2U32 client_size_curr; 
  V2U32 client_size_prev; 
  V4F32 clear_color;
};

global R_D3D11_Backend *r_d3d11_backend; 
