function VOX_MapResult 
vox_map(V3F32 pos, F32 voxel_scale)
{
	VOX_MapResult result = {0};
  result.idx = -1;
  
	// Treat center of chunk as origin
	pos.x += (VOX_SLICE_SIZE * voxel_scale) / 2.f;
	pos.z += (VOX_SLICE_SIZE * voxel_scale) / 2.f;
	pos.y += (VOX_SLICE_SIZE * voxel_scale) / 2.f;
  
	V3S32 chunk_coord = {0};
  chunk_coord.x = (S32)(pos.x / voxel_scale - 0.5f);
  chunk_coord.y = (S32)(pos.y / voxel_scale - 0.5f);
  chunk_coord.z = (S32)(pos.z / voxel_scale - 0.5f);
  
  B32 coord_valid = 
  (chunk_coord.x >= 0 && chunk_coord.x < VOX_SLICE_SIZE) && 
  (chunk_coord.y >= 0 && chunk_coord.y < VOX_SLICE_SIZE) && 
  (chunk_coord.z >= 0 && chunk_coord.z < VOX_SLICE_SIZE);
  
  if (coord_valid) {
    S32 idx = 
      chunk_coord.x + 
      chunk_coord.y*VOX_SLICE_SIZE +
      chunk_coord.z*VOX_SLICE_SIZE*VOX_SLICE_SIZE;
    
    result.idx = idx;
  }
  
  return result;
}

function VOX_RaycastResult 
vox_raycast(VOX_Chunk *chunk, F32 voxel_scale, V3F32 ro, V3F32 rd)
{
  VOX_RaycastResult result = {0};
  
  V3F32 stp = v3f32((F32)Sign(rd.x), (F32)Sign(rd.y), (F32)Sign(rd.z));
  V3F32 pos = v3f32(floorf32(ro.x), floorf32(ro.y), floorf32(ro.z));
  
  // Initial distance to next voxel boundary, in the appropriate direction
  V3F32 dist_to_vox = v3f32_sub(pos, ro);
  dist_to_vox = v3f32_mul(dist_to_vox, stp);
  
  // Convert stp to a value between 0 and 1 so we can use it to scale the ray
  // direction vector below.
  V3F32 stp_positive = v3f32_add(v3f32_scale(stp, 0.5f), v3f32(0.5f,0.5f,0.5f));
  
  // Distance to next voxel boundary expressed as a parametric 
  // t-value along the current pixel's ray.
  V3F32 t_max = {0};
	t_max.x = (rd.x > 0.f) ? (pos.x + 1.f - ro.x) / rd.x : (ro.x - pos.x) / -rd.x;
	t_max.y = (rd.y > 0.f) ? (pos.y + 1.f - ro.y) / rd.y : (ro.y - pos.y) / -rd.y;
	t_max.z = (rd.z > 0.f) ? (pos.z + 1.f - ro.z) / rd.z : (ro.z - pos.z) / -rd.z;
	
  // Distance needed to move by one voxel in each axis along the 
  // current pixel's ray. 
  V3F32 t_delta = {0};
  t_delta.x = absf32(1.f / rd.x);
  t_delta.y = absf32(1.f / rd.y);
  t_delta.z = absf32(1.f / rd.z);
  
  F32 steps = 0.f;
  F32 t = 0.f;
  B32 hit = 0;
  
  V3F32 normal = {0};
  
  for (F32 idx = 0.f; idx < 256.f; idx += 1.f) {
    VOX_MapResult map = vox_map(pos, voxel_scale);
    if (map.idx >= 0) {
      result.idx = map.idx;
      result.pos = pos;
      result.normal = normal;
      
      VOX_Voxel *v = vox_get_voxel(chunk, result.idx);
      if (v->opacity > 0) {
        hit = 1;
        break;
      }
    }
    
    // Advance to next voxel boundary in the dimension in which
    // the distance to the next voxel boundary is smallest
    if (t_max.x < t_max.y && t_max.x < t_max.z ) { 
      t = t_max.x; t_max.x += t_delta.x; pos.x += stp.x; 
      normal = v3f32(-stp.x, 0, 0);
    }
    else if(t_max.y < t_max.z ) { 
      t = t_max.y; t_max.y += t_delta.y; pos.y += stp.y; 
      normal = v3f32(0, -stp.y, 0);
    }
    else { 
      t = t_max.z; t_max.z += t_delta.z; pos.z += stp.z; 
      normal = v3f32(0, 0, -stp.z);
    }     
    
    steps += 1.f;
  }
  
  {
    V3F32 p = v3f32_add(result.pos, result.normal);
    VOX_MapResult map = vox_map(p, voxel_scale);
    result.prev_idx = map.idx;
  }
  
  result.hit = hit;
  result.steps = steps;
  
  return result;
}
