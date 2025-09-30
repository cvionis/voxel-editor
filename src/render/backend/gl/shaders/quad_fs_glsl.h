char *quad_fs_glsl = 
"#version 450 core\n"
// Interpolated vertex shader outputs
"in vec4       v_uv_rect;\n"
"in vec4       v_c0;\n" // Colors: left, right
"in vec4       v_c1;\n"
"in vec4       v_c2;\n" // Colors: bottom, top
"in vec4       v_c3;\n"
"in float      v_radius;\n"
"in flat float v_sample_mode;\n"
"in float      v_border_thickness;\n"
"in float      v_corner_softness;\n"
"in vec4       v_clip_rect;\n"
"in vec2       v_rect_center;\n"
"in vec2       v_rect_half_dim;\n"
"in vec2       v_pos;\n"
"in float      v_theta;\n"
// Output color
"out vec4 frag_color;\n"
// Constants and uniforms
"uniform sampler2D u_tex;\n"
"#define R_SAMPLE_MODE_COLOR 0.0\n"
"#define R_SAMPLE_MODE_ALPHA 1.0\n"

"float sdf_rounded_rect(vec2 pos, vec2 rect_center, vec2 rect_half_size, float r)\n"
"{\n"
"  vec2 d2 = abs(rect_center - pos) - rect_half_size + vec2(r,r);\n"
"  float dist = min(max(d2.x,d2.y),0.0) + length(max(d2,0.0)) - r;\n"
"  return dist;\n"
"}\n"

"void main()\n"
"{\n"
// Discard fragments outside the clipping rectangle
"  vec4 cl = v_clip_rect;\n"
"  if (cl != vec4(0.0)) {\n"
"    if ((gl_FragCoord.x <  cl.x) ||\n"
"        (gl_FragCoord.y <  cl.y) ||\n"
"        (gl_FragCoord.x >= cl.z) ||\n"
"        (gl_FragCoord.y >= cl.w)) {\n"
"      discard;\n"
"    }\n"
"  }\n"
// Rotation (opposite of rect's rotation)
// TODO: You could just give the fs the rotated position, right?
"  vec2 q = gl_FragCoord.xy - v_rect_center;\n"
"  float c = cos(-v_theta);\n"
"  float s = sin(-v_theta);\n"
"  q = mat2(c,s,-s,c) * q;\n"
// Get this fragment's position within current quad ([-1, 1] in x, y)
"  float uv_xtu = (q.x + v_rect_half_dim.x)/(2.*v_rect_half_dim.x);\n"
"  float uv_ytu = (q.y + v_rect_half_dim.y)/(2.*v_rect_half_dim.y);\n"
// Get UV coords for this fragment from the current quad's UV rect ([0, 1] in x, y)
"  vec4 color = vec4(1.0);\n"
"  if (v_uv_rect.z > 0.0) {\n"
"    ivec2 tex_size = textureSize(u_tex, 0);\n"
"    vec2 htex = vec2(0.5/tex_size.x, 0.5/tex_size.y);\n"
"    float uv_xu = v_uv_rect.x + (v_uv_rect.z - v_uv_rect.x) * uv_xtu;\n"
"    float uv_yu = v_uv_rect.y + (v_uv_rect.w - v_uv_rect.y) * uv_ytu;\n"
"    float uv_x = clamp(uv_xu, v_uv_rect.x + htex.x, v_uv_rect.z - htex.x);\n"
"    float uv_y = clamp(uv_yu, v_uv_rect.y + htex.y, v_uv_rect.w - htex.y);\n"
"    vec2 uv = vec2(uv_x, uv_y);\n"
// Sample from the texture according to the quad's sample mode
"    if (v_sample_mode == R_SAMPLE_MODE_COLOR) {\n"
"      color = texture(u_tex, uv);\n"
"    }\n"
"    else if (v_sample_mode == R_SAMPLE_MODE_ALPHA) {\n"
"      float s = texture(u_tex, uv).r;\n"
"      color.a *= s;\n"
"    }\n"
"  }\n"
// Calculate color gradient interpolation factors 
"  float color_v_t = clamp(uv_ytu, 0.0, 1.0);\n"
"  float color_h_t = clamp(uv_xtu, 0.0, 1.0);\n"
// Shrink the rectangle's half-size used for distance calculations with the edge 
// softness. If we don't do this, the underlying primitive will cut off the falloff 
// too early (https://www.rfleury.com/p/ui-part-6-rendering).
"  float softness = v_corner_softness;\n"
"  float soft_n = softness*2.0-1.0;\n"
"  vec2 softness_padding = vec2(max(0, soft_n), max(0, soft_n));\n"
// Compute filled/hollow rounded rectangle SDF factors
"  float d0 = sdf_rounded_rect(v_pos, v_rect_center,\n"
"    v_rect_half_dim-softness_padding, v_radius);\n"
"  float d1 = sdf_rounded_rect(v_pos, v_rect_center, v_rect_half_dim-\n"
"    v_border_thickness-softness_padding, v_radius-v_border_thickness);\n"
"  float sdf_factor = 1.0 - smoothstep(0.0, 2.0*softness, d0);\n"
"  float border_factor = smoothstep(0.0, 2.0*softness, d1);\n"
// Don't consider SDF factor for alpha sampling mode
"  sdf_factor = mix(sdf_factor, 1.0, v_sample_mode);\n"
// Compute final fragment color 
"  vec4 gradient_color = mix(v_c0, v_c1, color_h_t) * mix(v_c2, v_c3, color_v_t);\n"
"  vec4 fill_color = mix(color, vec4(border_factor), v_border_thickness);\n"
"  frag_color = fill_color * gradient_color * sdf_factor;\n"
"}\n";

