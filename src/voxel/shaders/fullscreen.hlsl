struct VS_INPUT {
	float2 pos : POSITION;
	float2 texcoord : TEXCOORD;
};

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float2 texcoord : TEXCOORD;
};

struct Voxel {
	float opacity;
	int palette_idx;
	uint id;
};

struct MapResult {
	float d;
	float3 color;
	uint id;
};

struct RaymarchResult {
	int hit;
	float3 pos;
	float3 color;
	float3 normal;
	float steps;
	uint id;
};

cbuffer PerFrameData : register(b0) {
	float2 client_size;
	float2 pad0;

	float2 view;
	float2 pad1;

	float4 mouse; // right click x, y; right click status (0/1); left click status (0/1)

	float zoom; // [1, float_max]
	float time;
	float2 pad3;
}

SamplerState chunk_sampler : register(s0);
Texture3D<float4> chunk_texture : register(t0);

static const float chunk_slice_size = 32;

static const float3 palette[4] = {
	float3(0.2, 0.2, 0.2),
	float3(0.24, 0.38, 0.1),
	float3(0.1, 0.23, 0.14),
	float3(0.123, 0.22, 0.24),
};

static const float steps_max = 256.0;

PS_INPUT 
vs_main(VS_INPUT input)
{
	PS_INPUT output;
	output.pos = float4(input.pos.x, input.pos.y, 0.0, 1.0);
	output.texcoord = input.texcoord;
	return output;
}

Voxel
get_voxel(float3 pos) 
{
	float3 chunk_origin = float3(0,0,0);
	float voxel_size = zoom;
	int3 chunk_coord = int3((pos - chunk_origin) / voxel_size);
	
	//float4 s = chunk_texture.Load(int4(chunk_coord.xyz, 0));
	float4 s = chunk_texture.Load(int4(pos.xyz/voxel_size, 0));

	Voxel result;
	result.opacity = s.x;
	result.palette_idx = (int)(s.y * 255.0 + 0.5);	

	uint id0 = (uint)(s.z * 255.0);
	uint id1 = (uint)(s.w * 255.0);

	result.id = (id0 << 8) | id1;

	return result;
}	

// https://www.shadertoy.com/view/XtBfzz
float 
filtered_grid(float2 p, float2 dx, float2 dy)
{
	const float N = 12.0;
	// filter kernel
  float2 w = max(abs(dx), abs(dy)) + 0.01;

	// analytic (box) filtering
  float2 a = p + 0.5*w;                        
  float2 b = p - 0.5*w;           
  float2 i = (floor(a)+min(frac(a)*N,1.0)-floor(b)-min(frac(b)*N,1.0))/(N*w);
  // pattern
  return (1.0-i.x)*(1.0-i.y);
}

float3 
apply_fog(float3 col, float3 fog_col, float t)
{
  float b = 0.05;
  float fog = 1.0 - exp(-t*b);
  return lerp(col, fog_col, fog);
}

float 
map1(float3 pos)
{
	float d = -pos.y;
	return d;
}

MapResult 
map(float3 pos)
{
	MapResult result;

	// Origin in center not corner
	pos.x += (chunk_slice_size * zoom) / 2;
	pos.z += (chunk_slice_size * zoom) / 2;
	pos.y += (chunk_slice_size * zoom) / 2;

	Voxel v = get_voxel(pos);

	result.d = v.opacity;
	result.color = palette[v.palette_idx];
	result.id = v.id;

	return result;
}

RaymarchResult
raymarch(float3 ro, float3 rd)
{
	// Step direction (+1 or -1 for each)
	float3 stp = sign(rd);

	float3 pos = floor(ro);

  // Initial distance to next voxel boundary, in the appropriate direction
  float3 dist_to_vox = pos - ro;
  dist_to_vox *= stp;
    
  // Convert stp to a value between 0 and 1 so we can use it to scale the ray
  // direction vector below.
  float3 stp_positive = (stp * 0.5) + 0.5;
   
  // Distance to next voxel boundary expressed as a parametric 
  // t-value along the current pixel's ray.
	float3 t_max;
	t_max.x = (rd.x > 0.0) ? (pos.x + 1.0 - ro.x) / rd.x : (ro.x - pos.x) / -rd.x;
	t_max.y = (rd.y > 0.0) ? (pos.y + 1.0 - ro.y) / rd.y : (ro.y - pos.y) / -rd.y;
	t_max.z = (rd.z > 0.0) ? (pos.z + 1.0 - ro.z) / rd.z : (ro.z - pos.z) / -rd.z;
	
  // Distance needed to move by one voxel in each axis along the 
  // current pixel's ray. 
  float3 t_delta = abs(float3(1.0,1.0,1.0) / rd);
    
  float steps = 0.0;
	float3 normal = float3(0,0,0);
	
	RaymarchResult res;
	
	// DDA loop
  float t = 0.0;
  int hit = 0;
	MapResult map_res;
	map_res.color = float3(0,0,0);
	map_res.id = -1;

  for (float idx = 0.0; idx < steps_max; idx += 1.0) {
		map_res = map(pos);
    if (map_res.d > 0.0) {
      hit = 1;
      break;
    }

    // Advance to next voxel boundary in the dimension in which
    // the distance to the next voxel boundary is smallest
    if (t_max.x < t_max.y && t_max.x < t_max.z ) { 
      t = t_max.x; t_max.x += t_delta.x; pos.x += stp.x; 
			normal = float3(-stp.x,0,0);
    }
    else if(t_max.y < t_max.z ) { 
      t = t_max.y; t_max.y += t_delta.y; pos.y += stp.y; 
			normal = float3(0, -stp.y, 0);
    }
    else { 
      t = t_max.z; t_max.z += t_delta.z; pos.z += stp.z; 
			normal = float3(0, 0, -stp.z);
    }     

    steps += 1.0;
  }
	
	res.hit = hit;
	res.color = map_res.color;
	res.pos = pos;	
	res.normal = normal;
	res.steps = steps;
	res.id = map_res.id;

	return res;
}

float4
ps_main(PS_INPUT input) : SV_TARGET
{
	float2 frag_coord = float2(input.pos.x, input.pos.y);

	float2 p = (frag_coord - client_size*0.5) / client_size.y;

	float view_angle = 10.*view.x / client_size.x;
	float ray_y = 120.*view.y / client_size.y;
	float orbit_radius = 100.;

	float3 rb = float3(0.0, 0.0, 0.0);
	float3 ro = rb + float3(
		orbit_radius*cos(view_angle), 
		-ray_y, 
		orbit_radius*sin(view_angle)
	);

	float fov = 3.14159/1.2;
  float3 up = float3(0.0,1.0,0.0);

  float3 cw = normalize(rb - ro);
  float3 cu = normalize(cross(cw, up));
  float3 cv = cross(cu, cw);
    
  float3 rd = normalize(p.x*cu + p.y*cv + cw*fov);
	
	float3 color = lerp(float3(0.22,0.22,0.12), float3(0.23,0.32,0.24), 2.0-dot(p,p));
	float3 normal = float3(0,0,0);

	float d;
	int hit = 0;
	float t = 0;
	float3 pos = float3(0,0,0);		

	for (float stp = 0.0; stp < 256; stp += 1.0) {
	  pos = ro + t*rd;
		d = map1(pos);
		if (d < 0.001) { 
			hit = 1;
			break;
		}
		t += d;
	}
	if (hit) {
		float3 bg_color = color;
		normal = float3(0.,-1,0.);

		float3 grid_color = float3(0.5,0.5,0.5);
        
    float grid_cell_freq = 1.;
		float2 uv = pos.xz*grid_cell_freq;
    float2 ddx_uv = ddx(uv); 
    float2 ddy_uv = ddy(uv); 
        
    float grid_f = (1.0-filtered_grid(uv, ddx_uv, ddy_uv));
    color = lerp(color, grid_color, grid_f);
		float tmp = 0.2;
    color = apply_fog(color, bg_color, tmp*t);
	}

	RaymarchResult res = raymarch(ro, rd);
	if (res.hit) {
		color = res.color;
		normal = res.normal;

 	 float3 key_dir = float3(-0.5, -0.6, -0.5);
	  float3 key_col = float3(1.64, 1.27, 0.99);
	  float  key = clamp(dot(normal, key_dir), 0.0, 1.0);

		float3 sky_col = float3(0.16,0.20,0.28);
		float sky = clamp(0.5 - 0.5*normal.y, 0.0, 1.0);

		float3 ind_col = float3(0.40,0.28,0.20);
		float ind = clamp(dot(normal, normalize(key_dir*float3(-1.0,0.0,-1.0))), 0.0, 1.0);

		float3 lin = key_col*key + sky_col*sky + ind_col*ind;
		color = color*lin;

		float gamma = 1.0/2.2;
		color = pow(color, float3(gamma,gamma,gamma));
	}

	return float4(color.xyz, 1.0);
}
