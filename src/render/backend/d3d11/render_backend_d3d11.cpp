#include "shaders/quad_hlsl.h"

//
// Internal D3D11 backend helpers
//

function R_Texture *
r_d3d11_texture_from_resource_view(ID3D11ShaderResourceView *view)
{
  R_Texture *texture = (R_Texture *)view;
  return texture;
}

function ID3D11ShaderResourceView *
r_d3d11_resource_view_from_texture(R_Texture *texture)
{
  ID3D11ShaderResourceView *view = (ID3D11ShaderResourceView *)texture;
  return view;
}

function DXGI_FORMAT
r_d3d11_texture_format(R_TextureFormat fmt)
{
  DXGI_FORMAT result = 0;
  
  switch (fmt) {
    case R_TextureFormat_R8: {
      result = DXGI_FORMAT_R8_UNORM;
    }break;
    // TODO: D3D11 doesn't support 24-bit formats...
    // Probably best to not use them at all for consistency across backends.
    case R_TextureFormat_RGB8: 
    case R_TextureFormat_RGBA8: { 
      result = DXGI_FORMAT_R8G8B8A8_UNORM;
    }break;
  }
  
  return result; 
}

function void 
r_backend_init(void)
{
  //
  // Allocate the backend context 
  //
  
  Arena *arena = arena_alloc_default();
  r_d3d11_backend = ArenaPushStruct(arena, R_D3D11_Backend);
  r_d3d11_backend->arena = arena;
  
  HRESULT hr; 
  
  //
  // Create D3D11 a device and device context
  //
  
  ID3D11Device *device;
  ID3D11DeviceContext *context;
  
  UINT flags = 0;
#if BUILD_DEBUG
  flags |= D3D11_CREATE_DEVICE_DEBUG; 
#endif
  D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0 };
  
  hr = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, flags, 
                         feature_levels, 1, D3D11_SDK_VERSION, &device, 0, &context);
  Assert(SUCCEEDED(hr));
  
#if BUILD_DEBUG
  // Enable debug break on API errors for debug builds
  ID3D11InfoQueue *info;
  ID3D11Device_QueryInterface(device, &IID_ID3D11InfoQueue, (void**)&info);
  ID3D11InfoQueue_SetBreakOnSeverity(info, D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
  ID3D11InfoQueue_SetBreakOnSeverity(info, D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
  ID3D11InfoQueue_Release(info);
  
  // Enable debug break on DXGI errors as well
  IDXGIInfoQueue *dxgi_info;
  hr = DXGIGetDebugInterface1(0, &IID_IDXGIInfoQueue, (void**)&dxgi_info);
  Assert(SUCCEEDED(hr));
  IDXGIInfoQueue_SetBreakOnSeverity(dxgi_info, DXGI_DEBUG_ALL, 
                                    DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
  IDXGIInfoQueue_SetBreakOnSeverity(dxgi_info, DXGI_DEBUG_ALL, 
                                    DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
  IDXGIInfoQueue_Release(dxgi_info);
#endif
  
  r_d3d11_backend->device = device;
  r_d3d11_backend->context = context;
  
  //
  // Set up resources for rendering instanced textured quads
  //
  
  // Instance buffer
  U32 max_quads = 1024; // NOTE: Temp
  ID3D11Buffer *vbuffer;
  {
    D3D11_BUFFER_DESC desc = {
      .Usage               = D3D11_USAGE_DYNAMIC,
      .ByteWidth           = sizeof(R_Quad) * max_quads, 
      .BindFlags           = D3D11_BIND_VERTEX_BUFFER,
      .CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE,  
      .MiscFlags           = 0,
      .StructureByteStride = sizeof(R_Quad),
    };
    
    ID3D11Device_CreateBuffer(device, &desc, 0, &vbuffer);
  }
  
  r_d3d11_backend->quad_vbuff = vbuffer;
  
  // Quad shaders and input layout
  ID3D11InputLayout *layout;
  ID3D11VertexShader *vshader;
  ID3D11PixelShader *pshader;
  {
    D3D11_INPUT_ELEMENT_DESC desc[] = {
      { "RECT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, OffsetOf(R_Quad, rect), 
        D3D11_INPUT_PER_INSTANCE_DATA, 1 },
      { "UV_RECT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, OffsetOf(R_Quad, uv_rect), 
        D3D11_INPUT_PER_INSTANCE_DATA, 1 },
      { "CL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, (UINT)OffsetOf(R_Quad, colors[0]), 
        D3D11_INPUT_PER_INSTANCE_DATA, 1 },
      { "CR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, (UINT)OffsetOf(R_Quad, colors[1]), 
        D3D11_INPUT_PER_INSTANCE_DATA, 1 },
      { "CB", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, (UINT)OffsetOf(R_Quad, colors[2]), 
        D3D11_INPUT_PER_INSTANCE_DATA, 1 },
      { "CT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, (UINT)OffsetOf(R_Quad, colors[3]), 
        D3D11_INPUT_PER_INSTANCE_DATA, 1 },
      { "THETA", 0, DXGI_FORMAT_R32_FLOAT, 0, OffsetOf(R_Quad, theta), 
        D3D11_INPUT_PER_INSTANCE_DATA, 1 },
      { "RADIUS", 0, DXGI_FORMAT_R32_FLOAT, 0, OffsetOf(R_Quad, radius), 
        D3D11_INPUT_PER_INSTANCE_DATA, 1 },
      // TODO: This is going to need to be an unsigned integer.
      { "SAMPLE_MODE", 0, DXGI_FORMAT_R32_UINT, 0, OffsetOf(R_Quad, sample_mode), 
        D3D11_INPUT_PER_INSTANCE_DATA, 1 },
      { "BORDER_THICKNESS", 0, DXGI_FORMAT_R32_FLOAT, 0, OffsetOf(R_Quad, border_thickness), 
        D3D11_INPUT_PER_INSTANCE_DATA, 1 },
      { "CORNER_SOFTNESS", 0, DXGI_FORMAT_R32_FLOAT, 0, OffsetOf(R_Quad, corner_softness), 
        D3D11_INPUT_PER_INSTANCE_DATA, 1 },
      { "CLIP_RECT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, OffsetOf(R_Quad, clip_rect), 
        D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    };
    
    U32 flags = 
      D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | 
      D3DCOMPILE_ENABLE_STRICTNESS | 
      D3DCOMPILE_WARNINGS_ARE_ERRORS;
#ifdef BUILD_DEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
    ID3DBlob* error;
    ID3DBlob* vblob;
    ID3DBlob* pblob;
    
    hr = D3DCompile(quad_hlsl, sizeof(quad_hlsl), 0, 0, 0, "vs_main", "vs_5_0", 
                    flags, 0, &vblob, &error);
    {
      if (error) {
        char *error_msg = ID3D10Blob_GetBufferPointer(error);
        (void *)error_msg;
      }
      Assert(SUCCEEDED(hr));
    }
    hr = D3DCompile(quad_hlsl, sizeof(quad_hlsl), 0, 0, 0, "ps_main", "ps_5_0", 
                    flags, 0, &pblob, &error);
    {
      if (error) {
        char *error_msg = ID3D10Blob_GetBufferPointer(error);
        (void *)error_msg;
      }
      Assert(SUCCEEDED(hr));
    }
    
    ID3D11Device_CreateVertexShader(device, ID3D10Blob_GetBufferPointer(vblob), 
                                    ID3D10Blob_GetBufferSize(vblob), 0, &vshader);
    ID3D11Device_CreatePixelShader(device, ID3D10Blob_GetBufferPointer(pblob), 
                                   ID3D10Blob_GetBufferSize(pblob), 0, &pshader);
    
    ID3D11Device_CreateInputLayout(device, desc, ARRAYSIZE(desc), 
                                   ID3D10Blob_GetBufferPointer(vblob), ID3D10Blob_GetBufferSize(vblob), &layout);
    
    ID3D10Blob_Release(pblob);
    ID3D10Blob_Release(vblob);
    
    r_d3d11_backend->quad_layout = layout;
    r_d3d11_backend->quad_vs = vshader;
    r_d3d11_backend->quad_ps = pshader;
  }
  
  //
  // Create a sampler for quads
  //
  
  ID3D11SamplerState *sampler;
  {
    D3D11_SAMPLER_DESC desc = {
      .Filter        = D3D11_FILTER_MIN_MAG_MIP_POINT,
      .AddressU      = D3D11_TEXTURE_ADDRESS_WRAP,
      .AddressV      = D3D11_TEXTURE_ADDRESS_WRAP,
      .AddressW      = D3D11_TEXTURE_ADDRESS_WRAP,
      .MipLODBias    = 0,
      .MaxAnisotropy = 1,
      .MinLOD        = 0,
      .MaxLOD        = D3D11_FLOAT32_MAX,
    };
    ID3D11Device_CreateSamplerState(device, &desc, &sampler);
  }
  r_d3d11_backend->sampler = sampler;
  
  //
  // Create a fallback texture for quads
  //
  
  ID3D11ShaderResourceView *fallback_texture_view;
  {
    U8 fallback_data[] = {
      0xFF,0xFF,0xFF,0xFF,
      0xFF,0xFF,0xFF,0xFF,
      0xFF,0xFF,0xFF,0xFF,
      0xFF,0xFF,0xFF,0xFF,
    };
    
    U32 fallback_width  = 2;
    U32 fallback_height = 2;
    
    D3D11_TEXTURE2D_DESC desc = {
      .Width      = fallback_width,
      .Height     = fallback_height,
      .MipLevels  = 1,
      .ArraySize  = 1,
      .Format     = DXGI_FORMAT_R8G8B8A8_UNORM,
      .SampleDesc = { 1, 0 },
      .Usage      = D3D11_USAGE_IMMUTABLE,
      .BindFlags  = D3D11_BIND_SHADER_RESOURCE,
    };
    
    D3D11_SUBRESOURCE_DATA data = {
      .pSysMem = fallback_data,
      .SysMemPitch = fallback_width * sizeof(U8),
    };
    
    ID3D11Texture2D* texture;
    ID3D11Device_CreateTexture2D(device, &desc, &data, &texture);
    ID3D11Device_CreateShaderResourceView(device, 
                                          (ID3D11Resource*)texture, 0, &fallback_texture_view);
    ID3D11Texture2D_Release(texture);
  }
  r_d3d11_backend->fallback_texture_view = fallback_texture_view;
  
  //
  // Set up pipeline state
  //
  
  ID3D11BlendState *blend_state;
  {
    D3D11_BLEND_DESC desc = {
      .RenderTarget[0] = {
        .BlendEnable           = TRUE,
        .SrcBlend              = D3D11_BLEND_SRC_ALPHA,
        .DestBlend             = D3D11_BLEND_INV_SRC_ALPHA,
        .BlendOp               = D3D11_BLEND_OP_ADD,
        .SrcBlendAlpha         = D3D11_BLEND_SRC_ALPHA,
        .DestBlendAlpha        = D3D11_BLEND_INV_SRC_ALPHA,
        .BlendOpAlpha          = D3D11_BLEND_OP_ADD,
        .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
      },
    };
    ID3D11Device_CreateBlendState(device, &desc, &blend_state);
  }
  
  ID3D11RasterizerState *rasterizer_state;
  {
    D3D11_RASTERIZER_DESC desc = {
      .FillMode        = D3D11_FILL_SOLID,
      .CullMode        = D3D11_CULL_NONE,
      .DepthClipEnable = TRUE,
    };
    ID3D11Device_CreateRasterizerState(device, &desc, &rasterizer_state);
  }
  
  ID3D11DepthStencilState *depth_state;
  {
    D3D11_DEPTH_STENCIL_DESC desc = {
      .DepthEnable      = FALSE,
      .DepthWriteMask   = D3D11_DEPTH_WRITE_MASK_ALL,
      .DepthFunc        = D3D11_COMPARISON_LESS,
      .StencilEnable    = FALSE,
      .StencilReadMask  = D3D11_DEFAULT_STENCIL_READ_MASK,
      .StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK,
    };
    ID3D11Device_CreateDepthStencilState(device, &desc, &depth_state);
  }
  
  r_d3d11_backend->blend_state = blend_state;
  r_d3d11_backend->rasterizer_state = rasterizer_state;
  r_d3d11_backend->depth_state = depth_state;
}

function void 
r_backend_equip_window(OS_Handle window)
{
  HRESULT hr; 
  ID3D11Device *device = r_d3d11_backend->device; 
  
  //
  // Create a DXGI swap chain for the provided window
  //
  
  IDXGISwapChain1 *swap_chain;
  {
    IDXGIDevice *dxgi_device;
    hr = ID3D11Device_QueryInterface(device, &IID_IDXGIDevice, (void**)&dxgi_device);
    Assert(SUCCEEDED(hr));
    
    // Get a DXGI adapter interface from the DXGI device
    IDXGIAdapter *dxgi_adapter;
    hr = IDXGIDevice_GetAdapter(dxgi_device, &dxgi_adapter);
    Assert(SUCCEEDED(hr));
    
    // Get a DXGI factory interface from the DXGI adapter
    IDXGIFactory2 *factory;
    hr = IDXGIAdapter_GetParent(dxgi_adapter, &IID_IDXGIFactory2, (void**)&factory);
    Assert(SUCCEEDED(hr));
    
    DXGI_SWAP_CHAIN_DESC1 desc = {
      // Use width and height of hwnd
      //.Width = 0,
      //.Height = 0,
      
      // NOTE: Use DXGI_FORMAT_R8G8B8A8_UNORM_SRGB for storing sRGB
      .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
      
      // FLIP presentation model does not allow MSAA framebuffer
      // if you want MSAA then you'll need to render offscreen and manually
      // resolve to non-MSAA framebuffer - Martins
      .SampleDesc = { 1, 0 },
      
      .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
      .BufferCount = 2,
      
      // we don't want any automatic scaling of window content
      // this is supported only on FLIP presentation model - Martins
      .Scaling = DXGI_SCALING_NONE,
      
      // use more efficient FLIP presentation model
      // Windows 10 allows to use DXGI_SWAP_EFFECT_FLIP_DISCARD
      // for Windows 8 compatibility use DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
      // for Windows 7 compatibility use DXGI_SWAP_EFFECT_DISCARD - Martins
      .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
    };
    
    HWND hwnd = os_win32_window_from_handle(window)->hwnd;
    
    hr = IDXGIFactory2_CreateSwapChainForHwnd(factory, (IUnknown*)device, hwnd, 
                                              &desc, 0, 0, &swap_chain);
    Assert(SUCCEEDED(hr));
    
    // Stop Alt+Enter from changing monitor resolution to match the window size.
    IDXGIFactory_MakeWindowAssociation(factory, hwnd, DXGI_MWA_NO_ALT_ENTER);
    
    IDXGIFactory2_Release(factory);
    IDXGIAdapter_Release(dxgi_adapter);
    IDXGIDevice_Release(dxgi_device);
  }
  
  r_d3d11_backend->swap_chain = swap_chain;
}

function void 
r_backend_set_viewport(RectU32 rect)
{
  V2U32 size = rect_u32_dim(rect); 
  r_d3d11_backend->client_size_curr = size;
}

function void 
r_backend_set_clear_color(V4F32 color)
{
  r_d3d11_backend->clear_color = color;
}

function void
r_backend_begin_frame(void)
{
  ID3D11Device *device = r_d3d11_backend->device;
  ID3D11DeviceContext *context = r_d3d11_backend->context;
  IDXGISwapChain1 *swap_chain = r_d3d11_backend->swap_chain;
  
  ID3D11RenderTargetView *rt_view = r_d3d11_backend->rt_view;
  ID3D11DepthStencilView *ds_view = r_d3d11_backend->ds_view;
  
  V2U32 client_curr = r_d3d11_backend->client_size_curr;
  V2U32 client_prev = r_d3d11_backend->client_size_prev;
  V4F32 clear_color = r_d3d11_backend->clear_color;
  
  //
  // Resize the swap chain if necessary 
  //
  
  if (!rt_view || client_curr.x != client_prev.x || client_curr.y != client_prev.y) {
    if (rt_view) {
      // Release old swap chain buffers (required in order to resize swap chain)
      ID3D11DeviceContext_ClearState(context);
      ID3D11RenderTargetView_Release(rt_view);
      ID3D11DepthStencilView_Release(ds_view);
      r_d3d11_backend->rt_view = 0;
    }
    
    // Resize the swap buffer, only if the new client size is nonzero
    if (client_curr.x != 0 && client_curr.y != 0) {
      IDXGISwapChain1_ResizeBuffers(swap_chain, 0, 
                                    client_curr.x, client_curr.y, DXGI_FORMAT_UNKNOWN, 0);
      
      // Create a new render target view for the resized swap buffer
      ID3D11Texture2D *backbuffer;
      IDXGISwapChain1_GetBuffer(swap_chain, 0, &IID_ID3D11Texture2D, &backbuffer);
      ID3D11Device_CreateRenderTargetView(device, 
                                          (ID3D11Resource*)backbuffer, 0, &r_d3d11_backend->rt_view);
      ID3D11Texture2D_Release(backbuffer);
      
      // Create a depth target view for the resized swap buffer
      D3D11_TEXTURE2D_DESC depth_desc = {
        .Width      = client_curr.x,
        .Height     = client_curr.y,
        .MipLevels  = 1,
        .ArraySize  = 1,
        .Format     = DXGI_FORMAT_D32_FLOAT, 
        .SampleDesc = { 1, 0 },
        .Usage      = D3D11_USAGE_DEFAULT,
        .BindFlags  = D3D11_BIND_DEPTH_STENCIL,
      };
      
      ID3D11Texture2D *depth;
      ID3D11Device_CreateTexture2D(device, &depth_desc, 0, &depth);
      ID3D11Device_CreateDepthStencilView(device, (ID3D11Resource*)depth, 
                                          0, &r_d3d11_backend->ds_view);
      ID3D11Texture2D_Release(depth);
    }
    r_d3d11_backend->client_size_prev = r_d3d11_backend->client_size_curr;
  }
  
  //
  // Configure the frame's pipeline state
  //
  if (rt_view) {
    // Clear the screen
    ID3D11DeviceContext_ClearRenderTargetView(context, r_d3d11_backend->rt_view, 
                                              &clear_color.e[0]);
    ID3D11DeviceContext_ClearDepthStencilView(context, r_d3d11_backend->ds_view, 
                                              D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
  }
}

function void
r_backend_end_frame(void)
{
  IDXGISwapChain1 *swap_chain = r_d3d11_backend->swap_chain;
  IDXGISwapChain1_Present(swap_chain, 0, 0);
}

function void 
r_backend_submit_batch(R_Batch *batch)
{
  ID3D11DeviceContext *context = r_d3d11_backend->context;
  
  ID3D11SamplerState *sampler = r_d3d11_backend->sampler;
  ID3D11ShaderResourceView *texture_view = r_d3d11_backend->fallback_texture_view;
  R_Texture *texture = batch->texture;
  if (texture != 0) {
    texture_view = r_d3d11_resource_view_from_texture(texture);
  }
  
  U32 quad_count_total = batch->quad_count_total; 
  U32 vertices_per_quad = 6; 
  
  D3D11_MAPPED_SUBRESOURCE mapped;
  ID3D11DeviceContext_Map(context, (ID3D11Resource *)r_d3d11_backend->quad_vbuff, 
                          0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
  
  U64 pos = 0;
  for (R_QuadChunk *chunk = batch->chunk_first; chunk != 0; chunk = chunk->next) {
    U64 size = chunk->quad_count * sizeof(R_Quad);
    MemoryCopy((void *)((U8 *)mapped.pData + pos), chunk->quads, size);
    pos += size; 
  }
  
  ID3D11DeviceContext_Unmap(context, (ID3D11Resource *)r_d3d11_backend->quad_vbuff, 0);
  
  // TODO: Why do I have to put this here and not in begin_frame()?
  // TODO: This doesn't need to be done for each batch... only setting of shader
  // resources (texture view) does.
  
  // Input Assembler
  ID3D11DeviceContext_IASetInputLayout(context, r_d3d11_backend->quad_layout);
  ID3D11DeviceContext_IASetPrimitiveTopology(context, 
                                             D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  
  UINT stride = sizeof(R_Quad); // NOTE: Temp
  UINT offset = 0;
  ID3D11DeviceContext_IASetVertexBuffers(context, 0, 1, 
                                         &r_d3d11_backend->quad_vbuff, &stride, &offset);
  
  // Vertex shader
  ID3D11DeviceContext_VSSetShader(context, r_d3d11_backend->quad_vs, 0, 0);
  
  // Rasterizer Stage
  D3D11_VIEWPORT viewport = {
    .TopLeftX = 0,
    .TopLeftY = 0,
    .Width    = (FLOAT)r_d3d11_backend->client_size_curr.x,
    .Height   = (FLOAT)r_d3d11_backend->client_size_curr.y,
    .MinDepth = 0,
    .MaxDepth = 1,
  };
  ID3D11DeviceContext_RSSetViewports(context, 1, &viewport);
  
  // Pixel shader
  ID3D11DeviceContext_PSSetSamplers(context, 0, 1, &sampler);
  ID3D11DeviceContext_PSSetShaderResources(context, 0, 1, &texture_view);
  ID3D11DeviceContext_PSSetShader(context, r_d3d11_backend->quad_ps, 0, 0);
  
  ID3D11DepthStencilState *depth_state = r_d3d11_backend->depth_state;
  ID3D11BlendState *blend_state = r_d3d11_backend->blend_state;
  
  // Output Merger
  ID3D11DeviceContext_OMSetBlendState(context, blend_state, 0, ~0U);
  ID3D11DeviceContext_OMSetDepthStencilState(context, depth_state, 0);
  ID3D11DeviceContext_OMSetRenderTargets(context, 1, 
                                         &r_d3d11_backend->rt_view, r_d3d11_backend->ds_view);
  
  // Draw quad instances
  ID3D11DeviceContext_DrawInstanced(context, 
                                    vertices_per_quad, quad_count_total, 0, 0);
}

// NOTE: Temporary test
function R_Texture *
r_backend_texture_create_in_place(void *data, U32 width, U32 height, 
                                  R_TextureFormat fmt, U32 pixel_align, B32 gen_mips)
{
  (void)pixel_align;
  (void)gen_mips;
  
  R_Texture *texture = 0; 
  
  DXGI_FORMAT dxgi_fmt = r_d3d11_texture_format(fmt);  
  ID3D11ShaderResourceView *d3d11_texture_view = 0;
  HRESULT hr;
  
  D3D11_TEXTURE2D_DESC desc = {
    .Width      = width,
    .Height     = height,
    .MipLevels  = 1,
    .ArraySize  = 1,
    .Format     = dxgi_fmt,
    .SampleDesc = { 1, 0 },
    .Usage      = D3D11_USAGE_DEFAULT,
    .BindFlags  = D3D11_BIND_SHADER_RESOURCE,
  };
  
  D3D11_SUBRESOURCE_DATA subrsrc = {
    .pSysMem = data,
    .SysMemPitch = width * sizeof(U8), // TODO: sizeof(U8) -> pixel_align
  };
  
  ID3D11Device *device = r_d3d11_backend->device;
  ID3D11Texture2D* d3d11_texture;
  hr = ID3D11Device_CreateTexture2D(device, &desc, &subrsrc, &d3d11_texture);
  if (SUCCEEDED(hr)) {
    hr = ID3D11Device_CreateShaderResourceView(device, 
                                               (ID3D11Resource*)d3d11_texture, 0, &d3d11_texture_view);
    if (SUCCEEDED(hr)) {
      texture = r_d3d11_texture_from_resource_view(d3d11_texture_view);
    }
    ID3D11Texture2D_Release(d3d11_texture);
  }
  
  return texture;
}

// TODO: Probably should just merge texture_create and texture_update and require
// data to be provided at creation so that we can make the texture immutable.
// TODO: Use the gen_mips and pixel_align parameters!
// TODO: pixel_align isn't used for D3D11 implementation...
function R_Texture *
r_backend_texture_create_impl(U32 width, U32 height, R_TextureFormat fmt, 
                              U32 pixel_align, B32 gen_mips)
{
  (void)pixel_align;
  (void)gen_mips;
  
  R_Texture *texture = 0; 
  
  DXGI_FORMAT dxgi_fmt = r_d3d11_texture_format(fmt);  
  ID3D11ShaderResourceView *d3d11_texture_view = 0;
  HRESULT hr;
  
  D3D11_TEXTURE2D_DESC desc = {
    .Width      = width,
    .Height     = height,
    .MipLevels  = 1,
    .ArraySize  = 1,
    .Format     = dxgi_fmt,
    .SampleDesc = { 1, 0 },
    .Usage      = D3D11_USAGE_DYNAMIC,
    .BindFlags  = D3D11_BIND_SHADER_RESOURCE,
    .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
  };
  
  ID3D11Device *device = r_d3d11_backend->device;
  ID3D11Texture2D* d3d11_texture;
  hr = ID3D11Device_CreateTexture2D(device, &desc, 0, &d3d11_texture);
  if (SUCCEEDED(hr)) {
    hr = ID3D11Device_CreateShaderResourceView(device, 
                                               (ID3D11Resource*)d3d11_texture, 0, &d3d11_texture_view);
    if (SUCCEEDED(hr)) {
      texture = r_d3d11_texture_from_resource_view(d3d11_texture_view);
    }
    ID3D11Texture2D_Release(d3d11_texture);
  }
  
  return texture ;
}

function void 
r_backend_texture_update(R_Texture *texture, void *data, U32 x, U32 y, 
                         U32 width, U32 height, R_TextureFormat fmt)
{
  HRESULT hr; 
  
  ID3D11ShaderResourceView *d3d11_resource_view = 
    r_d3d11_resource_view_from_texture(texture);
  
  // Get the texture from the resource view
  ID3D11Resource *d3d11_resource = 0;
  ID3D11ShaderResourceView_GetResource(d3d11_resource_view, &d3d11_resource);
  
  ID3D11Texture2D *d3d11_texture = 0;
  hr = ID3D11Resource_QueryInterface(d3d11_resource, &IID_ID3D11Texture2D,
                                     (void **)&d3d11_texture);
  ID3D11Resource_Release(d3d11_resource);
  
  // Update the texture
  if (SUCCEEDED(hr)) {
    ID3D11DeviceContext *context = r_d3d11_backend->context;
    D3D11_MAPPED_SUBRESOURCE mapped;
    
    ID3D11DeviceContext_Map(context, (ID3D11Resource*)d3d11_texture, 
                            0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    {
      U32 pixel_size = (fmt == R_TextureFormat_RGBA8) ? 4 : 1;
      U32 pitch = mapped.RowPitch;
      U8 *dest = (U8 *)mapped.pData + x*pixel_size + y*pitch;
      MemoryCopy((void *)dest, data, width * height * pixel_size); 
    }
    ID3D11DeviceContext_Unmap(context, (ID3D11Resource *)d3d11_texture, 0);
  }
}

function void 
r_backend_texture_delete(R_Texture **texture)
{
  if (texture) {
    ID3D11ShaderResourceView *view = r_d3d11_resource_view_from_texture(*texture);
    if (view) {
      ID3D11Resource *d3d11_resource = 0;
      ID3D11ShaderResourceView_GetResource(view, &d3d11_resource);
      
      ID3D11Texture2D *d3d11_texture = 0;
      ID3D11Resource_QueryInterface(d3d11_resource, &IID_ID3D11Texture2D,
                                    (void **)&d3d11_texture);
      ID3D11Texture2D_Release(d3d11_texture);
      *texture = 0;
      
      ID3D11Resource_Release(d3d11_resource);
      ID3D11ShaderResourceView_Release(view); 
    }
  }
}
