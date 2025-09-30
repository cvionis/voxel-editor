function VOX_Renderer *
vox_render_alloc(void)
{
  VOX_Renderer *r = 0;
  
  Arena *arena = arena_alloc_default();
  r = ArenaPushStruct(arena, VOX_Renderer);
  r->arena = arena;
  
  return r;
}

function void
vox_render_release(VOX_Renderer *r)
{
  // @Todo: Release D3D11 objects
  if (r) {
    arena_release(r->arena);
  }
}

function void
vox_render_init(VOX_Renderer *r, OS_Handle window, String8 shader_path)
{
  HRESULT hr; 
  
  // Create D3D11 device and device context
  
  {
    ID3D11Device *device;
    ID3D11DeviceContext *context;
    
    UINT flags = 0;
#if BUILD_DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG; 
#endif
    D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0 };
    
    hr = D3D11CreateDevice(0, 
                           D3D_DRIVER_TYPE_HARDWARE, 0, flags, 
                           feature_levels, 1, D3D11_SDK_VERSION, 
                           &device, 0, &context);
    Assert(SUCCEEDED(hr));
    
    r->device = device;
    r->context = context;
  }
  
  // Enable debug breaks in debug builds
  
#if BUILD_DEBUG
  {
    // Enable debug break on API errors for debug builds
    ID3D11InfoQueue *info;
    r->device->QueryInterface(IID_ID3D11InfoQueue, (void**)&info);
    info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
    info->Release();
    
    // Enable debug break on DXGI errors as well
    IDXGIInfoQueue *dxgi_info;
    hr = DXGIGetDebugInterface1(0, IID_IDXGIInfoQueue, (void**)&dxgi_info);
    Assert(SUCCEEDED(hr));
    dxgi_info->SetBreakOnSeverity(DXGI_DEBUG_ALL, 
                                  DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    dxgi_info->SetBreakOnSeverity(DXGI_DEBUG_ALL, 
                                  DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
    dxgi_info->Release();
  }
#endif
  
  // Create a DXGI swap chain for the provided window
  
  IDXGISwapChain1 *swapchain;
  {
    IDXGIDevice *dxgi_device;
    hr = r->device->QueryInterface(IID_IDXGIDevice, (void**)&dxgi_device);
    Assert(SUCCEEDED(hr));
    
    // Get a DXGI adapter interface from the DXGI device
    IDXGIAdapter *dxgi_adapter;
    hr = dxgi_device->GetAdapter(&dxgi_adapter);
    Assert(SUCCEEDED(hr));
    
    // Get a DXGI factory interface from the DXGI adapter
    IDXGIFactory2 *dxgi_factory;
    hr = dxgi_adapter->GetParent(IID_IDXGIFactory2, (void**)&dxgi_factory);
    Assert(SUCCEEDED(hr));
    
    DXGI_SWAP_CHAIN_DESC1 desc = {0};
    {
      // Use width and height of window
      desc.Width = 0;
      desc.Height = 0;
      
      // Note: Use DXGI_FORMAT_R8G8B8A8_UNORM_SRGB for storing sRGB
      desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
      
      // FLIP presentation model does not allow MSAA framebuffer
      // if you want MSAA then you'll need to render offscreen and manually
      // resolve to non-MSAA framebuffer - Martins
      desc.SampleDesc = { 1, 0 };
      
      desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      desc.BufferCount = 2;
      
      // we don't want any automatic scaling of window content
      // this is supported only on FLIP presentation model - Martins
      desc.Scaling = DXGI_SCALING_NONE;
      
      // use more efficient FLIP presentation model
      // Windows 10 allows to use DXGI_SWAP_EFFECT_FLIP_DISCARD
      // for Windows 8 compatibility use DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
      // for Windows 7 compatibility use DXGI_SWAP_EFFECT_DISCARD - Martins
      desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    }
    
    HWND hwnd = os_win32_window_from_handle(window)->hwnd;
    hr = dxgi_factory->CreateSwapChainForHwnd((IUnknown *)r->device, hwnd, &desc, 
                                              0, 0, &swapchain);
    Assert(SUCCEEDED(hr));
    
    // Stop Alt+Enter from changing monitor resolution to match the window size.
    dxgi_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
    
    dxgi_factory->Release();
    dxgi_adapter->Release();
    dxgi_device->Release();
    
    r->swapchain = swapchain;
  }
  
  // Set up a vertex buffer for rendering a full-screen quad
  
  {
    VOX_Vertex vertices[6] = {
      // Triangle 1
      { { -1.0f, -1.0f }, { 0.0f, 1.0f } }, // bottom-left
      { { -1.0f,  1.0f }, { 0.0f, 0.0f } }, // top-left
      { {  1.0f,  1.0f }, { 1.0f, 0.0f } }, // top-right
      // Triangle 2
      { { -1.0f, -1.0f }, { 0.0f, 1.0f } }, // bottom-left
      { {  1.0f,  1.0f }, { 1.0f, 0.0f } }, // top-right
      { {  1.0f, -1.0f }, { 1.0f, 1.0f } }, // bottom-right
    };
    
    D3D11_BUFFER_DESC desc = {0};
    desc.ByteWidth = sizeof(vertices);
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    
    D3D11_SUBRESOURCE_DATA initial_data = {0};
    initial_data.pSysMem = vertices;
    
    ID3D11Buffer *vertex_buffer;
    r->device->CreateBuffer(&desc, &initial_data, &vertex_buffer);
    r->vertex_buffer = vertex_buffer;
  }
  
  // Set up an input layout for the vertex inputs, create a vertex and pixel shader
  // for rendering a full-screen quad
  
  {
    D3D11_INPUT_ELEMENT_DESC desc[] = {
      { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(VOX_Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0},
      { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(VOX_Vertex, texcoord), 
        D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    
    TempArena scratch = arena_scratch_begin(0,0);
    
    String8 source = os_file_read(scratch.arena, shader_path);
    Assert(source.count > 0);
    
    U32 compile_flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;
#if BUILD_DEBUG
    compile_flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    compile_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
    r->shader_compile_flags = compile_flags;
    
    ID3DBlob *error;
    
    ID3DBlob *vblob;
    hr = D3DCompile(source.data, source.count, 0, 0, 0, "vs_main", "vs_5_0", 
                    compile_flags, 0, &vblob, &error);
    if (FAILED(hr)) {
      char *msg = (char *)error->GetBufferPointer();
      OutputDebugStringA(msg);
      Assert(!"vox_render_init(): failed to compile vertex shader");
    }
    
    ID3DBlob *pblob;
    hr = D3DCompile(source.data, source.count, 0, 0, 0, "ps_main", "ps_5_0", 
                    compile_flags, 0, &pblob, &error);
    if (FAILED(hr)) {
      char *msg = (char *)error->GetBufferPointer();
      OutputDebugStringA(msg);
      Assert(!"vox_render_init(): failed to compile pixel shader");
    }
    
    ID3D11VertexShader *vertex_shader;
    ID3D11PixelShader *pixel_shader;
    ID3D11InputLayout *input_layout;
    
    r->device->CreateVertexShader(vblob->GetBufferPointer(), vblob->GetBufferSize(), 0, &vertex_shader);
    r->device->CreatePixelShader(pblob->GetBufferPointer(), pblob->GetBufferSize(), 0, &pixel_shader);
    r->device->CreateInputLayout(desc, ARRAYSIZE(desc), vblob->GetBufferPointer(), vblob->GetBufferSize(), &input_layout);
    
    pblob->Release();
    vblob->Release();
    arena_scratch_end(scratch);
    
    r->pixel_shader = pixel_shader;
    r->vertex_shader = vertex_shader;
    r->input_layout = input_layout;
  }
  
  // Create a constant buffer to hold per-frame data
  
  {
    ID3D11Buffer *buff;
    
    D3D11_BUFFER_DESC desc = {0};
    desc.ByteWidth = sizeof(VOX_UniformData);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    r->device->CreateBuffer(&desc, 0, &buff);
    r->constant_buffer = buff;
  }
  
  // Create a 3D texture for storing chunk data
  
  {
    ID3D11ShaderResourceView *texture_view;
    U32 width = VOX_SLICE_SIZE;
    U32 height = VOX_SLICE_SIZE;
    U32 depth = VOX_SLICE_SIZE;
    
    D3D11_TEXTURE3D_DESC desc = {0};
    desc.Width          = width;
    desc.Height         = height;
    desc.Depth          = depth;
    desc.MipLevels      = 1; // @Todo: What should I use here?
    desc.Format         = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage          = D3D11_USAGE_DEFAULT;
    desc.BindFlags      = D3D11_BIND_SHADER_RESOURCE;
    
    ID3D11Texture3D *texture;
    r->device->CreateTexture3D(&desc, 0, &texture);
    r->device->CreateShaderResourceView((ID3D11Resource *)texture, 0, &texture_view);
    
    r->chunk_texture = texture;
    r->chunk_texture_view = texture_view;
  }
  
  // Create a sampler for the chunk texture(s)
  
  {
    ID3D11SamplerState *sampler;
    
    D3D11_SAMPLER_DESC desc = {0};
    desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.MipLODBias = 0;
    desc.MaxAnisotropy = 1;
    desc.MinLOD = 0;
    desc.MaxLOD = D3D11_FLOAT32_MAX;
    
    r->device->CreateSamplerState(&desc, &sampler);
    r->chunk_texture_sampler = sampler;
  }
  
  // Set up blend state
  
  {
    ID3D11BlendState *blend_state;
    
    // Use alpha blending
    D3D11_BLEND_DESC desc = {0};
    desc.RenderTarget[0] = {0};
    desc.RenderTarget[0].BlendEnable = TRUE;
    desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    
    r->device->CreateBlendState(&desc, &blend_state);
    r->blend_state = blend_state;
  }
  
  // Set up rasterizer state
  
  {
    ID3D11RasterizerState *rasterizer_state;
    
    // Disable culling
    D3D11_RASTERIZER_DESC desc = {0};
    desc.FillMode = D3D11_FILL_SOLID;
    desc.CullMode = D3D11_CULL_NONE;
    desc.DepthClipEnable = TRUE;
    
    r->device->CreateRasterizerState(&desc, &rasterizer_state);
    r->rasterizer_state = rasterizer_state;
  }
  
  // Set up depth state
  
  {
    ID3D11DepthStencilState *depth_state;
    
    // Disable depth and stencil testing
    D3D11_DEPTH_STENCIL_DESC desc = {0};
    desc.DepthEnable = FALSE;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc = D3D11_COMPARISON_LESS;
    desc.StencilEnable = FALSE;
    desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
    desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
    
    r->device->CreateDepthStencilState(&desc, &depth_state);
    r->depth_state = depth_state;
  }
  
  r->window = window;
  r->shader_path = shader_path;
}