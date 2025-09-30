#pragma once

struct VOX_MapResult {
  S32 idx;
};

struct VOX_RaycastResult {
  B32 hit;
  S32 prev_idx;
  V3F32 pos;
  V3F32 normal;
  S32 idx;
  F32 steps;
};

function VOX_MapResult vox_map(V3F32 pos, F32 voxel_scale);
function VOX_RaycastResult vox_raycast(VOX_Chunk *chunk, F32 voxel_scale, V3F32 ro, V3F32 rd);