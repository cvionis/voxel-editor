#pragma once

//
// Tweakable constants
//

#define R_BATCH_TABLE_SLOTS 64
#define R_CHUNK_QUADS_MAX 128

//
// Core rendering types
//

typedef void R_Texture;

struct R_Key {
  U64 v; 
};

enum R_SampleMode {
  R_SampleMode_Color,
  R_SampleMode_Alpha,
  R_SampleMode_COUNT,
};

enum R_TextureFormat {
  R_TextureFormat_Null,
  R_TextureFormat_R8,
  R_TextureFormat_RGB8, // TODO: Deprecate; not supported by all backends
  R_TextureFormat_RGBA8,
  R_TextureFormat_COUNT,
};

struct R_Font {
  R_Texture *texture;  
  F_FontCache *cache;
  V2U32 atlas_size;
};

// NOTE: A quad is either hollow or filled. border_thickness > 0: hollow with 
// border, else filled. To draw an outlined quad, draw a filled quad with a
// hollow, outlined one above it.
// TODO: Optimize layout.
struct R_Quad {
  RectF32 rect; 
  RectF32 uv_rect; 
  V4F32 colors[4]; 
  F32 theta; 
  F32 radius;
  R_SampleMode sample_mode; 
  F32 border_thickness; 
  F32 corner_softness;
  RectF32 clip_rect;
};

struct R_QuadChunk {
  R_QuadChunk *next; 
  R_Quad *quads;
  U32 quad_count; 
};

struct R_Batch {
  R_Batch *next; 
  
  R_QuadChunk *chunk_first;
  R_QuadChunk *chunk_last; 
  
  // Batch-unique data
  U32 chunk_count; 
  U32 quad_count_total; 
  R_Texture *texture; // NOTE: Texture doubles as batch's table key.
};

struct R_BatchSlot {
  R_Batch *batch;
};

struct R_Context {
  Arena *arena;
  Arena *frame_arena; 
  
  R_BatchSlot *batch_table;
  R_Batch *batch_list_first;  
  R_Batch *batch_list_last;  
  U32 batch_count; 
};


// TODO: This is stupid. Stop prefixing with backend and just make their implementations
// graphics-api specific...

//
// Rendering subsystem initialization and configuration
//

#define r_init()                 r_backend_init()
#define r_equip_window(window)   r_backend_equip_window(window)
#define r_set_viewport(rect)     r_backend_set_viewport(rect)
#define r_set_clear_color(color) r_backend_set_clear_color(color)

//
// Resources
//

#define r_texture_create(width, height, fmt) \
r_backend_texture_create_impl(width, height, fmt, 4, 0)

#define r_texture_update(texture, data, x, y, width, height, fmt) \
r_backend_texture_update(texture, data, x, y, width, height, fmt)

#define r_texture_delete(texture) r_backend_texture_delete(texture)

function R_Texture *r_texture_load(String8 path, R_TextureFormat fmt);

function R_Font *r_font_ttf_bake(Arena *arena, F_FontCache *font_cache);
function R_Font *r_font_ttf_parse_and_bake(Arena *arena, String8 path);

//
// Rendering context and drawing API
//

function R_Context *r_context_alloc(void);
function void r_context_release(R_Context *ctx);
function void r_begin_frame(R_Context *ctx);
function void r_flush(R_Context *ctx); 
function void r_end_frame(R_Context *ctx);

function void r_quad(R_Context *ctx, R_Quad *quad, R_Texture *texture);
function void r_segment(R_Context *ctx, V2F32 p0, V2F32 p1, F32 radius, V4F32 color);
function void r_circle(R_Context *ctx, V2F32 pos, F32 radius, F32 border_thickness, 
                       V4F32 color);
function void r_text(R_Context *ctx, String8 text, R_Font *font, F32 pt, V2F32 pos, 
                     V4F32 color, RectF32 *clip); 
