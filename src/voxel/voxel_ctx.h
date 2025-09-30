#pragma once

struct VOX_Context {
  VOX_Renderer *renderer;
  VOX_UniformData uniforms;
  VOX_Input input;
  VOX_EditState edit;
  VOX_Chunk chunk;
};

function VOX_Context vox_ctx_make(OS_Handle window);
function void vox_ctx_release(VOX_Context *ctx);

function void vox_update_uniforms(VOX_Context *ctx);
function void vox_update_edit_state(VOX_Context *ctx);
function void vox_update_chunk(VOX_Context *ctx);
function void vox_reload_shader(VOX_Context *ctx);
function void vox_render(VOX_Context *ctx);