#pragma once

//
// Initialization and configuration
//

function void r_backend_init(void);
function void r_backend_equip_window(OS_Handle window);
function void r_backend_set_viewport(RectU32 rect);
function void r_backend_set_clear_color(V4F32 color);

//
// Per-frame operations
//

function void r_backend_begin_frame(void);
function void r_backend_end_frame(void);
function void r_backend_submit_batch(R_Batch *batch);

//
// Resource management
//

// TODO: Deprecate these two in favor of a single in-place function for
// sake of backend compatibility. 
// An alternative would be to provide a single texture creation API, but add a 
// usage enum (immutable, cpu-writable, etc.), and allow textures created as 
// cpu-writable to be updated later with the texture update API.
function R_Texture *r_backend_texture_create_impl(U32 width, U32 height, 
                                                  R_TextureFormat fmt, U32 pixel_align, B32 gen_mips);
function void r_backend_texture_update(R_Texture *texture, void *data, 
                                       U32 x, U32 y, U32 width, U32 height, R_TextureFormat fmt);

function void r_backend_texture_delete(R_Texture **texture);

// NOTE: Temporary test
function R_Texture *r_backend_texture_create_in_place(void *data, 
                                                      U32 width, U32 height, R_TextureFormat fmt, U32 pixel_align, B32 gen_mips);
