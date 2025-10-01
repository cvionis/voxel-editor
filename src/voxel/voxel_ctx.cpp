function VOX_Context 
vox_ctx_make(OS_Handle window)
{
  VOX_Context ctx = {0};
  
  VOX_Renderer *r = vox_render_alloc();
  vox_render_init(r, window, S8("../src/voxel/shaders/fullscreen.hlsl"));
  ctx.renderer = r;
  
  return ctx;
}

function void
vox_update_uniforms(VOX_Context *ctx)
{
  static F32 time = 0;
  time += 0.0167f;
  
  VOX_Input *input = &ctx->input;
  VOX_UniformData *uniforms = &ctx->uniforms;
  VOX_Renderer *r = ctx->renderer;
  
  B32 r_mouse = vox_mouse_down(input, VOX_MouseButton_Right);
  B32 l_mouse = vox_mouse_pressed(input, VOX_MouseButton_Left);
  V2S32 mouse_s32 = vox_mouse_position(input);
  
  // Client size, time, zoom
  {
    uniforms->client_size = v2f32((F32)r->client_size.x, (F32)r->client_size.y);
    uniforms->time = time;
    uniforms->zoom += vox_key_pressed(input, VOX_Key_Equal)*0.1f;
    uniforms->zoom -= vox_key_pressed(input, VOX_Key_Minus)*0.1f;
  }
  
  // View angle (@Todo)
  {
    static V2F32 view_pos = v2f32(0,0);
    static V2F32 last_view_mouse = v2f32(0,0);
    V2F32 curr_view_mouse = v2f32((F32)mouse_s32.x, (F32)mouse_s32.y);
    
    if (view_pos.x == 0 && view_pos.y == 0) {
      view_pos = v2f32((F32)mouse_s32.x, (F32)mouse_s32.y);
    }
    
    V2F32 view_delta = v2f32_sub(curr_view_mouse, last_view_mouse);
    if (r_mouse) {
      view_pos = v2f32_add(view_pos, view_delta);
      r_mouse = 0;
    }
    uniforms->view = view_pos;
    last_view_mouse = curr_view_mouse;
  }
  
  // Mouse data
  {
    V2F32 pos = v2f32((F32)mouse_s32.x, (F32)mouse_s32.y);
    uniforms->mouse = v4f32(pos.x, pos.y, (F32)r_mouse, (F32)l_mouse);
  }
}

function void
vox_update_edit_state(VOX_Context *ctx)
{
  VOX_EditState *edit = &ctx->edit;
  VOX_Input *input = &ctx->input;
  
  VOX_UniformData *uniforms = &ctx->uniforms;
  VOX_Chunk *chunk = &ctx->chunk;
  
  S32 selected_voxel_idx = -1;
  S32 nearest_empty_voxel_idx = -1;
  
  B32 l_mouse_pressed = (B32)uniforms->mouse.w;
  V2F32 client_size = uniforms->client_size;
  
  if (l_mouse_pressed) {
    F32 mx = (uniforms->mouse.x - client_size.x*0.5f) / client_size.y;
    F32 my = (uniforms->mouse.y - client_size.y*0.5f) / client_size.y;
    
    F32 view_angle = 10.f*uniforms->view.x / client_size.x;
    F32 orbit_radius = 100.f;
    F32 ray_y = 120.f*uniforms->view.y / client_size.y;
    F32 fov = 3.14159f/1.2f;
    V3F32 up = v3f32(0,1,0);;
    
    V3F32 rb = v3f32(0,0,0);
    V3F32 ro = v3f32_add(rb, v3f32(orbit_radius*cosf32(view_angle), -ray_y, orbit_radius*sinf32(view_angle)));
    
    V3F32 cw = v3f32_normalize(v3f32_sub(rb, ro));
    V3F32 cu = v3f32_normalize(v3f32_cross(cw, up));
    V3F32 cv = v3f32_cross(cu, cw);
    
    V3F32 dx = v3f32_scale(cu, mx);
    V3F32 dy = v3f32_scale(cv, my);
    V3F32 dz = v3f32_scale(cw, fov);
    
    V3F32 rd = v3f32_normalize(v3f32_add(v3f32_add(dx, dy), dz));
    
    F32 voxel_scale = uniforms->zoom;
    VOX_RaycastResult raycast = vox_raycast(chunk, voxel_scale, ro, rd);
    if (raycast.hit) {
      selected_voxel_idx = raycast.idx;
      nearest_empty_voxel_idx = raycast.prev_idx;
    }
  }
  
  edit->selected_voxel_idx = selected_voxel_idx;
  edit->nearest_empty_voxel_idx = nearest_empty_voxel_idx;
  
  if (0) {}
  else if (vox_key_pressed(input, VOX_Key_F1)) {
    edit->mode = VOX_EditMode_Delete;
  }
  else if (vox_key_pressed(input, VOX_Key_F2)) {
    edit->mode = VOX_EditMode_Add;
  }
}

function void
vox_update_chunk(VOX_Context *ctx)
{
  VOX_EditState *edit = &ctx->edit;
  VOX_Chunk *chunk = &ctx->chunk;
  
  S32 selected_voxel_idx = edit->selected_voxel_idx;
  S32 nearest_empty_voxel_idx = edit->nearest_empty_voxel_idx;
  
  switch (edit->mode) {
    case VOX_EditMode_Delete: {
      if (selected_voxel_idx >= 0) {
        VOX_Voxel *v = vox_get_voxel(chunk, selected_voxel_idx);
        v->opacity = 0;
      }
    }break;
    case VOX_EditMode_Add: {
      if (nearest_empty_voxel_idx >= 0) {
        VOX_Voxel *v = vox_get_voxel(chunk, nearest_empty_voxel_idx);
        v->opacity = 255;
      }
    }break;
  }
  
  edit->selected_voxel_idx = -1;
}

function void 
vox_reload_shader(VOX_Context *ctx)
{
  VOX_Renderer *r = ctx->renderer;
  U32 compile_flags = r->shader_compile_flags; 
  
  TempArena scratch = arena_scratch_begin(0,0);
  {
    String8 source = os_file_read(scratch.arena, r->shader_path);
    if (source.count > 0) {
      ID3DBlob *error;
      ID3DBlob *pblob;
      D3DCompile(source.data, source.count, 0, 0, 0, "ps_main", "ps_5_0", 
                 compile_flags, 0, &pblob, &error);
      r->device->CreatePixelShader(pblob->GetBufferPointer(), pblob->GetBufferSize(), 0, &r->pixel_shader);
      pblob->Release();
    }
  }
  arena_scratch_end(scratch);
}

function void
vox_render(VOX_Context *ctx)
{
  VOX_Renderer *r = ctx->renderer;
  VOX_UniformData *uniforms = &ctx->uniforms;
  
  static U32 last_width = 0;
  static U32 last_height = 0;
  
  RectU32 client = os_window_client_rect(r->window);
  U32 client_width = client.x1 - client.x0;
  U32 client_height = client.y1 - client.y0;
  r->client_size = v2u32(client_width, client_height);
  
  B32 should_resize_swapchain = (!r->rt_view) | (client_width != last_width) | (client_height != last_height);
  if (should_resize_swapchain) {
    if (r->rt_view) { 
      r->context->ClearState();
      r->rt_view->Release();
      r->ds_view->Release();
      r->rt_view = 0;
    }
    
    if (client_width && client_height) {
      r->swapchain->ResizeBuffers(0, client_width, client_height, DXGI_FORMAT_UNKNOWN, 0);
      
      // Create a new render target view adjusted for the new client size
      ID3D11Texture2D *backbuffer;
      r->swapchain->GetBuffer(0, IID_ID3D11Texture2D, (void **)&backbuffer);
      r->device->CreateRenderTargetView((ID3D11Resource *)backbuffer, 0, &r->rt_view);
      backbuffer->Release();
      
      // Create a new depth stencil view adjusted for the new client size
      D3D11_TEXTURE2D_DESC depth_desc = {0};
      depth_desc.Width = client_width;
      depth_desc.Height = client_height;
      depth_desc.MipLevels = 1;
      depth_desc.ArraySize = 1;
      depth_desc.Format = DXGI_FORMAT_D32_FLOAT; 
      depth_desc.SampleDesc = { 1, 0 };
      depth_desc.Usage = D3D11_USAGE_DEFAULT;
      depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
      
      ID3D11Texture2D *depth;
      r->device->CreateTexture2D(&depth_desc, 0, &depth);
      r->device->CreateDepthStencilView((ID3D11Resource *)depth, 0, &r->ds_view);
      depth->Release();
      
      last_width = client_width;
      last_height = client_height;
    }
  }
  
  if (r->rt_view) {
    // Clear screen
    F32 clear_color[] = { 0.392f, 0.584f, 0.929f, 1.f };
    r->context->ClearRenderTargetView(r->rt_view, clear_color);
    r->context->ClearDepthStencilView(r->ds_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
    
    D3D11_VIEWPORT viewport = {0};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = (F32)client_width;
    viewport.Height = (F32)client_height;
    viewport.MinDepth = 0;
    viewport.MaxDepth = 1;
    
    // Upload updated uniforms
    {
      D3D11_MAPPED_SUBRESOURCE mapped;
      r->context->Map((ID3D11Resource *)r->constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
      MemoryCopy(mapped.pData, uniforms, sizeof(VOX_UniformData));
      r->context->Unmap((ID3D11Resource *)r->constant_buffer, 0);
    }
    
    // Upload updated chunk data to 3D texture
    {
      S32 row_pitch = sizeof(VOX_Voxel) * VOX_SLICE_SIZE;
      S32 depth_pitch = row_pitch * VOX_SLICE_SIZE;
      
      VOX_Voxel *data = ctx->chunk.voxels;
      r->context->UpdateSubresource(r->chunk_texture, 0, 0, (void *)data, row_pitch, depth_pitch);
    }
    
    // Input Assembler
    r->context->IASetInputLayout(r->input_layout);
    r->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    U32 stride = sizeof(VOX_Vertex);
    U32 offset = 0;
    r->context->IASetVertexBuffers(0, 1, &r->vertex_buffer, &stride, &offset);
    
    // Vertex Shader
    r->context->VSSetShader(r->vertex_shader, 0, 0);
    
    // Rasterizer Stage
    r->context->RSSetViewports(1, &viewport);
    r->context->RSSetState(r->rasterizer_state);
    
    // Pixel Shader
    r->context->PSSetConstantBuffers(0, 1, &r->constant_buffer);
    r->context->PSSetSamplers(0, 1, &r->chunk_texture_sampler);
    r->context->PSSetShaderResources(0, 1, &r->chunk_texture_view);
    r->context->PSSetShader(r->pixel_shader, 0, 0);
    
    // Output Merger
    r->context->OMSetBlendState(r->blend_state, 0, ~0U);
    r->context->OMSetDepthStencilState(r->depth_state, 0);
    r->context->OMSetRenderTargets(1, &r->rt_view, r->ds_view);
    
    // Draw 4 vertices
    r->context->Draw(6, 0);
  }
  
  B32 vsync = 1;
  HRESULT hr = r->swapchain->Present(vsync, 0);
  if (hr == DXGI_STATUS_OCCLUDED) {
    // Window is minimized, cannot vsync - instead sleep a bit
    if (vsync) {
      Sleep(10);
    }
  }
}