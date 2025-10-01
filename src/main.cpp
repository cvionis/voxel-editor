#include "base/base_inc.h"
#include "os/os_inc.h"
#include "font/font_inc.h"
#include "voxel/voxel_inc.h"

#include "base/base_inc.cpp"
#include "os/os_inc.cpp"
#include "font/font_inc.cpp"
#include "voxel/voxel_inc.cpp"

struct AppState {
  B32 quit;
  Arena *arena;
  OS_Handle window;
  VOX_Context vox_ctx;
};

void 
entry_point(void) 
{
  os_init();
  os_gfx_init();
  
  AppState app = {0};
  app.window = os_window_open(S8("VoxelEdit"), 1280, 720);
  app.arena = arena_alloc_default();
  app.vox_ctx = vox_ctx_make(app.window);
  
  {
    VOX_Context *ctx = &app.vox_ctx;
    VOX_Chunk *chunk = &ctx->chunk;
    for (S32 voxel_idx = 0; voxel_idx < VOX_CHUNK_SIZE; voxel_idx += 1) {
      VOX_Voxel *v = &chunk->voxels[voxel_idx];
      v->opacity = 155;
      v->color = 2;
      
      v->id0 = (U8)((voxel_idx >> 8) & 255);
      v->id1 = (U8)(voxel_idx & 255);
#if 0
      if (voxel_idx >= VOX_CHUNK_SIZE/8) {
        v->color = 0;
      }
      if (voxel_idx >= VOX_CHUNK_SIZE/4) {
        v->color = 3;
      }
      if (voxel_idx >= VOX_CHUNK_SIZE/2) {
        v->color = 1;
      }
#endif
    }
  }
  
  while (!app.quit) {
    OS_Handle window = app.window;
    VOX_Context *ctx = &app.vox_ctx;
    
    OS_EventList *events = os_get_events();
    for (OS_Event *e = events->first; e != 0; e = e->next) {
      if (e->kind == OS_EventKind_WindowClose) {
        app.quit = 1;
      }
    }
    
    vox_get_input(ctx, events);
    vox_update_uniforms(ctx);
    vox_update_edit_state(ctx);
    vox_update_chunk(ctx);
    
    vox_reload_shader(ctx);
    vox_render(ctx);
  }
  
  arena_release(app.arena);
  os_window_close(app.window);
}