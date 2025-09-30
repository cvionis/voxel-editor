//
// Helpers
//

function R_Key
r_key_from_texture(R_Texture *texture)
{
  R_Key key = {0};
  key.v = IntFromPtr(texture);
  return key;
}

function void
r_batch_push_quad(Arena *arena, R_Batch *batch, R_Quad *quad)
{
  // First chunk
  if (batch->chunk_last == 0) {
    R_QuadChunk *chunk = ArenaPushStruct(arena, R_QuadChunk);
    SLLQueuePush(batch->chunk_first, batch->chunk_last, chunk);
    batch->chunk_count += 1;
  }
  
  // If out of space in the newest chunk, add another chunk 
  if (batch->chunk_last->quad_count >= R_CHUNK_QUADS_MAX) {
    R_QuadChunk *chunk = ArenaPushStruct(arena, R_QuadChunk);
    SLLQueuePush(batch->chunk_first, batch->chunk_last, chunk);
    batch->chunk_count += 1;
  }
  
  // First quad in chunk
  if (batch->chunk_last->quads == 0) {
    batch->chunk_last->quads = ArenaPushArray(arena, R_Quad, R_CHUNK_QUADS_MAX);
  }
  R_Quad *pos = batch->chunk_last->quads + batch->chunk_last->quad_count;
  MemoryCopyStruct(pos, quad);
  batch->chunk_last->quad_count += 1;
  
  batch->quad_count_total += 1; 
}

//
// Rendering subsystem resources
//

function R_Texture *
r_texture_load(String8 path, R_TextureFormat fmt)
{
  R_Texture *texture = 0;
  
  S32 width, height;
  S32 num_channels;
  
  stbi_set_flip_vertically_on_load(1);
  U8 *data = stbi_load((char *)path.data, &width, &height, &num_channels, 0);
  if (data) {
#if 0
    texture = r_backend_texture_create_impl(width, height, fmt, 4, 0);
    if (texture) {
      r_backend_texture_update(texture, data, 0, 0, width, height, fmt);
    }
#endif
    texture = r_backend_texture_create_in_place(data, width, height, fmt, 4, 0);
  }
  
  return texture; 
}

function R_Font * 
r_font_ttf_bake(Arena *arena, F_FontCache *font_cache)
{
  R_Font *result = 0;
  
  // Calculate size of font atlas
  
  U32 atlas_width  = 0; 
  U32 atlas_height = 0;
  
  F_FontSize *sizes = font_cache->sizes;
  U32 sizes_count = font_cache->sizes_count;
  
  V2U32 *dim_largest = sizes[sizes_count-1].dim;
  for (S32 chr = 32; chr < F_GLYPH_MAX; chr += 1) {
    U32 width = dim_largest[chr].x;
    atlas_width += width;
  }
  
  for (U32 size_idx = 0; size_idx < sizes_count; size_idx += 1) {
    atlas_height += sizes[size_idx].max_glyph_height;
  }
  
  // Create a texture for the atlas
  
#if 0
  TempArena temp = TempArenaBegin(arena);
  U8 *atlas_data = ArenaPushArray(temp.arena, U8, atlas_width*atlas_height);
  
  for (U32 size_idx = 0; size_idx < sizes_count; size_idx += 1) {
    F_FontSize *curr_size = &sizes[size_idx];
    
    for (S32 chr = 32; chr < F_GLYPH_MAX; chr += 1) {
      V2U32 dim = curr_size->dim[chr];
      V2U32 off = curr_size->off[chr];
      U8 *bmp   = curr_size->bmp[chr];
      
      MemoryCopy(atlas_data+off.x+off.y*atlas_width, bmp, dim.x*dim.y);
    }
  }
  
  R_TextureFormat fmt = R_TextureFormat_R8;
  R_Texture *texture = r_backend_texture_create_in_place(atlas_data, 
                                                         atlas_width, atlas_height, fmt, 1, 1);
  
  TempArenaEnd(temp);
  
  if (texture) {
    result = ArenaPushStruct(arena, R_Font);
    result->texture = texture;
    result->cache = font_cache;
    result->atlas_size = v2u32(atlas_width, atlas_height);
  }
#endif
  // NOTE: Temporary
  
  R_TextureFormat fmt = R_TextureFormat_R8;
  R_Texture *texture = r_backend_texture_create_impl(atlas_width, atlas_height, 
                                                     fmt, 1, 1);
  
  if (texture) {
    result = ArenaPushStruct(arena, R_Font);
    
    for (U32 size_idx = 0; size_idx < sizes_count; size_idx += 1) {
      F_FontSize *curr_size = &sizes[size_idx];
      
      for (S32 chr = 32; chr < F_GLYPH_MAX; chr += 1) {
        V2U32 dim = curr_size->dim[chr];
        V2U32 off = curr_size->off[chr];
        U8 *bmp   = curr_size->bmp[chr];
        
        r_backend_texture_update(texture, (void *)bmp, off.x, off.y, 
                                 dim.x, dim.y, fmt);
      }
    }
    
    result->texture = texture;
    result->cache = font_cache;
    result->atlas_size = v2u32(atlas_width, atlas_height);
  }
  
  
  return result; 
}

function R_Font * 
r_font_ttf_parse_and_bake(Arena *arena, String8 path)
{
  R_Font *result = 0;
  
  F32 default_sizes[] = { 10.f, 11.f, 13.f, 14.f, 16.f, 20.f, 24.f, 32.f };
  
  F_FontCache *font_cache = f_parse_and_cache_font_ttf(arena, path, default_sizes, 
                                                       ArrayCount(default_sizes));
  if (font_cache) {
    result = r_font_ttf_bake(arena, font_cache);
  }
  
  return result; 
}

//
// Rendering context and drawing API
//

function R_Context *
r_context_alloc(void)
{
  Arena *arena = arena_alloc_default(); 
  R_Context *ctx = ArenaPushStruct(arena, R_Context);
  ctx->arena = arena; 
  ctx->frame_arena = arena_alloc_default(); 
  return ctx; 
}

function void 
r_context_release(R_Context *ctx)
{
  arena_release(ctx->frame_arena);
  arena_release(ctx->arena);
}

function void 
r_begin_frame(R_Context *ctx)
{
  arena_clear(ctx->frame_arena);
  ctx->batch_table = ArenaPushArray(ctx->frame_arena, R_BatchSlot, R_BATCH_TABLE_SLOTS);
  r_backend_begin_frame();
}

function void 
r_flush(R_Context *ctx)
{
  for (R_Batch *batch = ctx->batch_list_first; batch != 0; batch = batch->next) {
    r_backend_submit_batch(batch);
  }
}

function void 
r_end_frame(R_Context *ctx)
{
  (void *)ctx;
  r_backend_end_frame();
}

function void 
r_quad(R_Context *ctx, R_Quad *quad, R_Texture *texture)
{
  R_Key key = r_key_from_texture(texture);
  U64 idx = key.v % R_BATCH_TABLE_SLOTS; 
  R_BatchSlot *slot = &ctx->batch_table[idx];
  
  R_Batch *batch = slot->batch;  
  if (batch == 0) {
    batch = ArenaPushStruct(ctx->frame_arena, R_Batch);
    batch->texture = texture;
    SLLQueuePush(ctx->batch_list_first, ctx->batch_list_last, batch);
    slot->batch = batch; 
    ctx->batch_count += 1;
  }
  r_batch_push_quad(ctx->frame_arena, batch, quad);
}

function void 
r_segment(R_Context *ctx, V2F32 p0, V2F32 p1, F32 radius, V4F32 color)
{
  V2F32 v = v2f32_sub(p1, p0); 
  F32 len = v2f32_length(v);
  
  V2F32 center = v2f32_scale(v2f32_add(p0,p1), 0.5f);
  F32 xdelta = len * 0.5f + radius;
  F32 ydelta = radius;
  
  F32 theta = PI_F32/2.f - v2f32_angle(v); 
  
  R_Quad quad = {0};
  {
    quad.colors[0] = color;
    quad.colors[1] = color;
    quad.colors[2] = color;
    quad.colors[3] = color;
    quad.theta = theta;
    quad.corner_softness = 1;
    
    quad.rect = rect_f32(
                         center.x - xdelta,
                         center.y - ydelta, 
                         center.x + xdelta,
                         center.y + ydelta
                         );
    
    r_quad(ctx, &quad, 0);
  }
}

function void 
r_circle(R_Context *ctx, V2F32 pos, F32 radius, F32 border_thickness, V4F32 color)
{
  R_Quad quad = {0};
  {
    quad.colors[0] = color;
    quad.colors[1] = color;
    quad.colors[2] = color;
    quad.colors[3] = color;
    quad.radius = radius;
    quad.border_thickness = border_thickness;
    quad.corner_softness = 1;
    
    quad.rect = rect_f32(
                         pos.x, 
                         pos.y, 
                         pos.x + 2*radius,
                         pos.y + 2*radius
                         );
    
    r_quad(ctx, &quad, 0);
  }
}

function void 
r_text(R_Context *ctx, String8 text, R_Font *font, F32 pt, V2F32 pos, 
       V4F32 color, RectF32 *clip)
{
  if (font) {
    V2U32 atlas_size = font->atlas_size;
    
    F_FontCache *cache = font->cache;
    F_FontSize *sizes = cache->sizes;
    
    // Search for the desired font size in the font cache
    F_FontSize *curr_size = 0;
    U32 size_max_glyph_height = 0;
    
    for (U32 size_idx = 0; size_idx < cache->sizes_count; size_idx += 1) {
      F_FontSize *size = &sizes[size_idx];
      if (size->size_pt == pt) {
        curr_size = size;
        size_max_glyph_height = size->max_glyph_height;
        break;
      }
    }
    
    // Construct a quad for each glyph in the text string
    
    if (curr_size) {
      F32 curr_x = pos.x;
      F32 curr_y = pos.y;
      
      F32 iaw = 1.f/(F32)atlas_size.x;
      F32 iah = 1.f/(F32)atlas_size.y;
      
      RectF32 clip_rect = {0};
      if (clip != 0) {
        clip_rect = *clip;
      }
      
      for (U64 idx = 0; idx < text.count; idx += 1) {
        U8 chr = text.data[idx];
        if (chr < 32 || chr > F_GLYPH_MAX) {
          chr = '?'; 
        }
        
        V2S32 *adv = &curr_size->adv[chr];
        V2U32 *dim = &curr_size->dim[chr];
        V2S32 *spc = &curr_size->spc[chr];
        V2U32 *off = &curr_size->off[chr];
        
        R_Quad quad = {0};
        
        // NOTE: Adding the max glyph height of the font size 
        // being used is a hacky but convenient method I found 
        // to be able to render text using a top-left origin 
        // while still maintaining bottom-left relative glyph
        // spacings. 
        // It's not a particularly nice solution, but it's the
        // only one I could think of for now.
        F32 x = curr_x + (F32)spc->x;
        F32 y = curr_y + (F32)spc->y + size_max_glyph_height;
        
        quad.rect = {
          x, 
          y,
          x + (F32)dim->x, 
          y - (F32)dim->y,
        };
        
        F32 uv_x = (F32)off->x*iaw;
        F32 uv_y = (F32)off->y*iah;
        
        quad.uv_rect = {
          uv_x, 
          uv_y,
          uv_x + (F32)dim->x*iaw,
          uv_y + (F32)dim->y*iah,
        };
        
        quad.colors[0] = color;
        quad.colors[1] = color;
        quad.colors[2] = color;
        quad.colors[3] = color;
        
        quad.sample_mode = R_SampleMode_Alpha;
        quad.clip_rect = clip_rect;
        
        r_quad(ctx, &quad, font->texture);
        
        curr_x += (F32)adv->x;
      }
    }
  }
}

