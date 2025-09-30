//
// Undo system
//

function ED_UndoContext 
ed_undo_make(void)
{
  ED_UndoContext undo = {0};
  
  Arena *arena = arena_alloc_default();
  undo.arena = arena;
  
  return undo;
}

function void
ed_undo_release(ED_UndoContext *undo)
{
  arena_release(undo->arena);
}

function void 
ed_undo_clear(ED_UndoContext *undo)
{
  arena_clear(undo->arena);
  undo->count = 0;
}

function void 
ed_undo_push(ED_UndoContext *undo, G_TileMap *map)
{
  ED_UndoNode *n = undo->free;
  SLLStackPopN(undo->free, free_next);
  if (!n) {
    n = ArenaPushStruct(undo->arena, ED_UndoNode);
  }
  
  U32 tiles_size = map->width * map->height * sizeof(G_TileToken);
  MemoryCopy(n->tokens, map->tokens, tiles_size);
  n->width = map->width;
  n->height = map->height;
  
  SLLQueuePushFrontN(undo->first, undo->last, n, stack_next);
  undo->count += 1;
}

function void
ed_undo_pop(ED_UndoContext *undo, G_TileMap *map)
{
  ED_UndoNode *n = undo->first;
  if (n) {
    if (map) {
      U32 tiles_size = map->width * map->height * sizeof(G_TileToken);
      MemoryCopy(map->tokens, n->tokens, tiles_size);
      map->width = n->width;
      map->height = n->height;
      
      SLLQueuePopN(undo->first, undo->last, stack_next);
      SLLStackPushN(undo->free, n, free_next);
      undo->count -= 1;
    }
  }
}

//
// Tilemap growing/shrinking helpers
//

function void
ed_tilemap_column_append_left(Arena *arena, G_TileMap *map)
{
  S32 width = map->width;
  S32 height = map->height;
  S32 tiles_count = width*height;
  S32 src_row_size = sizeof(G_TileToken)*width;
  
  G_TileMap *tmp = ArenaPushStruct(arena, G_TileMap);
  MemoryCopyStruct(tmp, map);
  
  S32 tiles_size = sizeof(G_TileToken)*tiles_count;
  for (S32 idx = 0; idx < tiles_size; idx += 1) {
    map->tokens[idx] = G_TileToken_Empty;
  }
  
  map->width += 1;
  
  for (S32 row = 0; row < height; row += 1) {
    S32 dst_off = row*(width+1)+1;
    S32 src_off = row*width;
    
    MemoryCopy(map->tokens+dst_off, tmp->tokens+src_off, src_row_size); 
  }
}

function void
ed_tilemap_column_append_right(Arena *arena, G_TileMap *map)
{
  S32 width = map->width;
  S32 height = map->height;
  S32 tiles_count = width*height;
  
  S32 src_row_size = sizeof(G_TileToken)*width;
  
  G_TileMap *tmp = ArenaPushStruct(arena, G_TileMap);
  MemoryCopyStruct(tmp, map);
  
  S32 tiles_size = sizeof(G_TileToken)*tiles_count;
  for (S32 idx = 0; idx < tiles_size; idx += 1) {
    map->tokens[idx] = G_TileToken_Empty;
  }
  
  map->width += 1;
  
  for (S32 row = 0; row < height; row += 1) {
    S32 dst_off = row*(width+1);
    S32 src_off = row*width;
    
    MemoryCopy(map->tokens+dst_off, tmp->tokens+src_off, src_row_size); 
  }
}

function void
ed_tilemap_column_remove_left(Arena *arena, G_TileMap *map)
{
  S32 width = map->width;
  S32 height = map->height;
  S32 tiles_count = width*height;
  
  if (tiles_count) {
    S32 new_width = width - 1;
    S32 row_size = sizeof(G_TileToken)*new_width;
    
    G_TileMap *tmp = ArenaPushStruct(arena, G_TileMap);
    MemoryCopyStruct(tmp, map);
    
    MemoryZero(map->tokens, sizeof(G_TileToken)*tiles_count);
    map->width = new_width;
    
    for (S32 row = 0; row < height; row += 1) {
      S32 dst_off = row*new_width;
      S32 src_off = row*width+1;
      
      MemoryCopy(map->tokens+dst_off, tmp->tokens+src_off, row_size); 
    }
  }
}

function void
ed_tilemap_column_remove_right(Arena *arena, G_TileMap *map)
{
  S32 width = map->width;
  S32 height = map->height;
  S32 tiles_count = width*height;
  
  if (tiles_count) {
    S32 new_width = width - 1;
    S32 row_size = sizeof(G_TileToken)*new_width;
    
    G_TileMap *tmp = ArenaPushStruct(arena, G_TileMap);
    MemoryCopyStruct(tmp, map);
    
    MemoryZero(map->tokens, sizeof(G_TileToken)*tiles_count);
    map->width = new_width;
    
    for (S32 row = 0; row < height; row += 1) {
      S32 dst_off = row*new_width;
      S32 src_off = row*width;
      
      MemoryCopy(map->tokens+dst_off, tmp->tokens+src_off, row_size); 
    }
  }
}

function void
ed_tilemap_row_append_top(Arena *arena, G_TileMap *map)
{
  S32 width = map->width;
  S32 height = map->height;
  S32 tiles_count = width*height;
  S32 src_row_size = sizeof(G_TileToken)*width;
  
  G_TileMap *tmp = ArenaPushStruct(arena, G_TileMap);
  
  MemoryCopyStruct(tmp, map);
  
  S32 tiles_size = sizeof(G_TileToken)*tiles_count;
  for (S32 idx = 0; idx < tiles_size; idx += 1) {
    map->tokens[idx] = G_TileToken_Empty;
  }
  
  map->height += 1;
  
  for (S32 row = 0; row < height; row += 1) {
    S32 src_off = row*width;
    S32 dst_off = (row+1)*width;
    
    MemoryCopy(map->tokens+dst_off, tmp->tokens+src_off, src_row_size); 
  }
}

function void
ed_tilemap_row_append_bottom(Arena *arena, G_TileMap *map)
{
  S32 width = map->width;
  S32 height = map->height;
  S32 tiles_count = width*height;
  S32 src_row_size = sizeof(G_TileToken)*width;
  
  G_TileMap *tmp = ArenaPushStruct(arena, G_TileMap);
  MemoryCopyStruct(tmp, map);
  
  
  S32 tiles_size = sizeof(G_TileToken)*tiles_count;
  for (S32 idx = 0; idx < tiles_size; idx += 1) {
    map->tokens[idx] = G_TileToken_Empty;
  }
  
  map->height += 1;
  
  for (S32 row = 0; row < height; row += 1) {
    S32 off = row*width;
    MemoryCopy(map->tokens+off, tmp->tokens+off, src_row_size); 
  }
}

function void
ed_tilemap_row_remove_top(Arena *arena, G_TileMap *map)
{
  S32 width = map->width;
  S32 height = map->height;
  S32 tiles_count = width*height;
  
  if (tiles_count) {
    S32 row_size = sizeof(G_TileToken)*width;
    
    G_TileMap *tmp = ArenaPushStruct(arena, G_TileMap);
    MemoryCopyStruct(tmp, map);
    
    MemoryZero(map->tokens, sizeof(G_TileToken)*tiles_count);
    map->height -= 1;
    
    for (S32 row = 1; row < height; row += 1) {
      S32 src_off = row*width;
      S32 dst_off = (row-1)*width;
      
      MemoryCopy(map->tokens+dst_off, tmp->tokens+src_off, row_size); 
    }
  }
}

function void
ed_tilemap_row_remove_bottom(Arena *arena, G_TileMap *map)
{
  S32 width = map->width;
  S32 height = map->height;
  S32 tiles_count = width*height;
  
  if (tiles_count) {
    S32 row_size = sizeof(G_TileToken)*width;
    S32 new_height = height - 1;
    
    G_TileMap *tmp = ArenaPushStruct(arena, G_TileMap);
    MemoryCopyStruct(tmp, map);
    
    MemoryZero(map->tokens, sizeof(G_TileToken)*tiles_count);
    map->height = new_height;
    
    for (S32 row = 0; row < new_height; row += 1) {
      S32 off = row*width;
      
      MemoryCopy(map->tokens+off, tmp->tokens+off, row_size); 
    }
  }
}

//
// Tile choice controls
//

function void
ed_choice_ctrl_update(ED_ChoiceControl *ctrl, G_Input *input)
{
  F32 scale = ctrl->scale;
  F32 base_x = ctrl->base_x;
  F32 base_y = ctrl->base_y;
  F32 pad_x = ctrl->pad_x;
  F32 pad_y = ctrl->pad_y;
  S32 column_size = ctrl->column_size;
  
  F32 mx = input->mouse.x;
  F32 my = input->mouse.y;
  
  S32 hovered_choice_idx = -1;
  
  for (S32 choice_idx = 0; choice_idx < ED_CHOICES_COUNT; choice_idx += 1) {
    ED_ChoiceEntry *choice = &ed_choice_table[choice_idx];
    
    F32 choice_x_min = base_x + pad_x*(choice_idx / column_size);
    F32 choice_y_min = base_y + (scale+pad_y)*(choice_idx % column_size);
    F32 choice_x_max = choice_x_min + scale;
    F32 choice_y_max = choice_y_min + scale;
    
    B32 mouse_in_choice = 
    (mx >= choice_x_min) && (my >= choice_y_min) && (mx < choice_x_max) && (my < choice_y_max);
    if (mouse_in_choice) {
      hovered_choice_idx = choice_idx;
    }
  }
  
  if (hovered_choice_idx >= 0 && hovered_choice_idx < ED_CHOICES_COUNT) {
    ctrl->hovered_choice_idx = hovered_choice_idx; // @Todo: Why are we doing this here
    if (g_mouse_pressed(input, G_MouseButton_Left)) {
      ctrl->selected_choice_idx = hovered_choice_idx;
    }
  }
  
  if (g_key_pressed(input, G_Key_1)) {
    ctrl->selected_choice_idx = 0;
  }
  else if (g_key_pressed(input, G_Key_2)) {
    ctrl->selected_choice_idx = 1;
  }
  else if (g_key_pressed(input, G_Key_3)) {
    ctrl->selected_choice_idx = 2;
  }
  else if (g_key_pressed(input, G_Key_4)) {
    ctrl->selected_choice_idx = 3;
  }
  else if (g_key_pressed(input, G_Key_5)) {
    ctrl->selected_choice_idx = 4;
  }
  else if (g_key_pressed(input, G_Key_6)) {
    ctrl->selected_choice_idx = 5;
  }
  else if (g_key_pressed(input, G_Key_7)) {
    ctrl->selected_choice_idx = 6;
  }
  else if (g_key_pressed(input, G_Key_8)) {
    ctrl->selected_choice_idx = 7;
  }
  else if (g_key_pressed(input, G_Key_9)) {
    ctrl->selected_choice_idx = 8;
  }
  else if (g_key_pressed(input, G_Key_E)) {
    ctrl->selected_choice_idx = 9;
  }
  else if (g_key_pressed(input, G_Key_R)) {
    ctrl->selected_choice_idx = 10;
  }
}

function void
ed_choice_ctrl_render(Arena *arena, R_Context *renderer, R_Font *font, G_AssetContext *assets, ED_ChoiceControl *ctrl)
{
  F32 scale = ctrl->scale;
  
  F32 base_x = ctrl->base_x;
  F32 base_y = ctrl->base_y;
  F32 pad_x = ctrl->pad_x;
  F32 pad_y = ctrl->pad_y;
  S32 column_size = ctrl->column_size;
  
  V4F32 text_color = v4f32(0.4f,0.4f,0.4f,1.f);
  
  F32 bg_width = 600;
  F32 bg_height = (pad_y + scale) * column_size;
  F32 bg_pad = 10;
  
  R_Quad quad = {0};
  quad.uv_rect = rect_f32(0,0,1,1);
  quad.corner_softness = 0.3f;
  quad.radius = 10.2f;
  
  {
    V4F32 bg_color = v4f32(0.2f,0.2f,0.2f,1.f);
    
    quad.rect = rect_f32(base_x-bg_pad,base_y-bg_pad,base_x+bg_width+bg_pad,base_y+bg_height+bg_pad);
    quad.colors[0] = bg_color;
    quad.colors[1] = bg_color;
    quad.colors[2] = bg_color;
    quad.colors[3] = bg_color;
    r_quad(renderer, &quad, 0);
  }
  {
    V4F32 bg_color = v4f32(0.4f,0.4f,0.4f,1.f);
    
    quad.rect = rect_f32(base_x-bg_pad,base_y-bg_pad,base_x+bg_width+bg_pad,base_y+bg_height+bg_pad);
    quad.colors[0] = bg_color;
    quad.colors[1] = bg_color;
    quad.colors[2] = bg_color;
    quad.colors[3] = bg_color;
    quad.border_thickness = 1.2f;
    r_quad(renderer, &quad, 0);
  }
  
  for (S32 choice_idx = 0; choice_idx < ED_CHOICES_COUNT; choice_idx += 1) {
    ED_ChoiceEntry *choice = &ed_choice_table[choice_idx];
    G_Sprite *sprite = g_assets_get_sprite(assets, choice->path);
    
    F32 spr_y = base_y + (scale + pad_y)*(choice_idx % column_size);
    F32 spr_x = base_x + pad_x*(choice_idx / column_size);
    
    V4F32 sprite_color = v4f32(1,1,1,1);
    if (ctrl->hovered_choice_idx == choice_idx) {
      sprite_color = v4f32(0.88f,0.88f,0.88f,1.f);
    }
    
    ed_render_sprite(renderer, sprite, spr_x, spr_y, scale, sprite_color);
    
    if (ctrl->selected_choice_idx == choice_idx) {
      V4F32 outline_color = v4f32(0.9f,0.9f,0.9f,1.f);
      F32 pad = 4;
      R_Quad quad = {0};
      quad.rect = rect_f32(spr_x-pad, spr_y-pad, spr_x+scale+pad, spr_y+scale+pad);
      quad.uv_rect = rect_f32(0,0,1,1);
      quad.colors[0] = outline_color;
      quad.colors[1] = outline_color;
      quad.colors[2] = outline_color;
      quad.colors[3] = outline_color;
      quad.corner_softness = 0.3f;
      quad.radius = 6.f;
      quad.border_thickness = 1.6f;
      r_quad(renderer, &quad, 0);
    }
    
    U32 keybind_codepoint = ed_codepoint_from_key(choice->keybind);
    String8 keybind_str = str8_pushf(arena, "%c", keybind_codepoint);
    
    V2F32 text_pos = v2f32(spr_x + scale*1.4f, spr_y + pad_y*2);
    r_text(renderer, keybind_str, font, 32, text_pos, text_color, 0);
  }
}

//
// Tilemap resizing controls
//

function void
ed_resize_ctrl_update(ED_ResizeControl *ctrl, ED_MapControl *map_ctrl, G_MapStorage *maps, ED_UndoContext *undo, G_Input *input)
{
  //
  // Compute square controls' positions
  //
  
  F32 base_x = ctrl->base_x;
  F32 base_y = ctrl->base_y;
  
  F32 square_x = base_x;
  F32 square_y = base_y;
  F32 square_size = ctrl->square_size;
  
  ED_ResizeOption *options = ctrl->options;
  
  options[ED_ResizeDirection_Left].rect = rect_f32(square_x, square_y, square_x+square_size,  square_y+square_size);
  
  square_x = base_x + 2*square_size;
  options[ED_ResizeDirection_Right].rect = rect_f32(square_x, square_y, square_x+square_size, square_y+square_size);
  
  square_x = base_x + square_size;
  square_y = base_y - square_size;
  options[ED_ResizeDirection_Top].rect = rect_f32(square_x, square_y, square_x+square_size, square_y+square_size);
  
  square_y = base_y + square_size;
  options[ED_ResizeDirection_Bottom].rect = rect_f32(square_x, square_y, square_x+square_size,  square_y+square_size);
  
  //
  // Interaction with square controls
  //
  
  F32 mx = input->mouse.x;
  F32 my = input->mouse.y;
  
  ED_ResizeDirection hovered_direction = ED_ResizeDirection_None;
  
  for (S32 option_idx = 0; option_idx < 4; option_idx += 1) {
    RectF32 rect = ctrl->options[option_idx].rect;
    
    B32 mouse_in_option = (mx >= rect.x0) && (my >= rect.y0) && (mx < rect.x1) && (my < rect.y1);
    if (mouse_in_option) {
      hovered_direction = (ED_ResizeDirection)option_idx;
    }
  }
  
  Arena *scratch = arena_get_scratch(0,0);
  
  ctrl->hovered_direction = hovered_direction;
  if (g_mouse_pressed(input, G_MouseButton_Left)) {
    G_TileMap *map = ed_map_get(maps, map_ctrl->current_map_idx);
    ed_undo_push(undo, map);
    
    switch (ctrl->mode) {
      case ED_ResizeMode_Append: {
        switch (hovered_direction) {
          case ED_ResizeDirection_Left: {
            ed_tilemap_column_append_left(scratch, map);
          }break;
          case ED_ResizeDirection_Right: {
            ed_tilemap_column_append_right(scratch, map);
          }break;
          case ED_ResizeDirection_Top: {
            ed_tilemap_row_append_top(scratch, map);
          }break;
          case ED_ResizeDirection_Bottom: {
            ed_tilemap_row_append_bottom(scratch, map);
          }break;
        }
      }break;
      case ED_ResizeMode_Remove: {
        switch (hovered_direction) {
          case ED_ResizeDirection_Left: {
            ed_tilemap_column_remove_left(scratch, map);
          }break;
          case ED_ResizeDirection_Right: {
            ed_tilemap_column_remove_right(scratch, map);
          }break;
          case ED_ResizeDirection_Top: {
            ed_tilemap_row_remove_top(scratch, map);
          }break;
          case ED_ResizeDirection_Bottom: {
            ed_tilemap_row_remove_bottom(scratch, map);
          }break;
        }
      }break;
    }
  }
  
  //
  // Resize mode selection
  //
  
  if (g_key_pressed(input, G_Key_Space)) {
    ctrl->mode = (ED_ResizeMode)!(U32)ctrl->mode;
  }
}

function void
ed_resize_ctrl_render(R_Context *renderer, ED_ResizeControl *ctrl)
{
  V4F32 square_color = v4f32(0.5f,0.58f,0.5f,1.f);
  if (ctrl->mode == ED_ResizeMode_Remove) {
    square_color = v4f32(0.58f,0.5f,0.5f,1.f);
  }
  
  F32 base_x = ctrl->base_x;
  F32 base_y = ctrl->base_y;
  F32 bg_pad = 64;
  F32 bg_width = 600;
  F32 bg_height = ctrl->square_size;
  
  R_Quad quad = {0};
  quad.uv_rect = rect_f32(0,0,1,1);
  quad.corner_softness = 0.3f;
  quad.radius = 10.2f;
  
  {
    V4F32 bg_color = v4f32(0.2f,0.2f,0.2f,1.f);
    
    quad.rect = rect_f32(base_x-bg_pad,base_y-bg_pad,base_x+bg_width+bg_pad,base_y+bg_height+bg_pad);
    quad.corner_softness = 0.3f;
    quad.radius = 10.2f;
    quad.colors[0] = bg_color;
    quad.colors[1] = bg_color;
    quad.colors[2] = bg_color;
    quad.colors[3] = bg_color;
    r_quad(renderer, &quad, 0);
  }
  {
    V4F32 bg_color = v4f32(0.4f,0.4f,0.4f,1.f);
    quad.rect = rect_f32(base_x-bg_pad,base_y-bg_pad,base_x+bg_width+bg_pad,base_y+bg_height+bg_pad);
    quad.corner_softness = 0.3f;
    quad.radius = 10.2f;
    quad.colors[0] = bg_color;
    quad.colors[1] = bg_color;
    quad.colors[2] = bg_color;
    quad.colors[3] = bg_color;
    quad.border_thickness = 1.2f;
    r_quad(renderer, &quad, 0);
  }
  
  quad.corner_softness = 0.3f;
  quad.radius = 3.2f;
  quad.border_thickness = 0;
  
  for (S32 option_idx = 0; option_idx < ED_ResizeDirection_COUNT; option_idx += 1) {
    ED_ResizeOption *option = &ctrl->options[option_idx];
    
    V4F32 col = square_color;
    if (option_idx == ctrl->hovered_direction) {
      col = v4f32_sub(col, v4f32(0.1f,0.1f,0.1f,0.f));
    }
    
    quad.colors[0] = col;
    quad.colors[1] = col;
    quad.colors[2] = col;
    quad.colors[3] = col;
    
    quad.rect = option->rect;
    r_quad(renderer, &quad, 0);
  }
}

//
// Tilemap-editing controls
//

function void
ed_map_ctrl_update(ED_MapControl *ctrl, ED_ChoiceControl *choice_ctrl, G_MapStorage *maps, ED_UndoContext *undo, G_Input *input)
{
  F32 mx = input->mouse.x;
  F32 my = input->mouse.y;
  
  F32 pmx = ctrl->prev_mouse_x;
  F32 pmy = ctrl->prev_mouse_y;
  
  F32 base_x = ctrl->base_x;
  F32 base_y = ctrl->base_y;
  F32 scale = ctrl->scale;
  
  // Map selection
  
  if (g_key_pressed(input, G_Key_Right)) {
    ctrl->current_map_idx += 1;
    ctrl->current_map_idx = Min(ctrl->current_map_idx, G_MAPS_MAX-1);
  }
  if (g_key_pressed(input, G_Key_Left)) {
    ctrl->current_map_idx -= 1;
    ctrl->current_map_idx = Max(ctrl->current_map_idx, 0);
  }
  
  // Scale adjustment
  
  static F32 scale_granularity = 10;
  if (g_key_pressed(input, G_Key_Up)) {
    ctrl->scale += scale_granularity;
  }
  if (g_key_pressed(input, G_Key_Down)) {
    ctrl->scale -= scale_granularity;
  }
  
  // Tile selection in map 
  
  // @Todo: Pass map storage instead
  G_TileMap *map = ed_map_get(maps, ctrl->current_map_idx);
  S32 tiles_count = map->width * map->height;
  
  ctrl->hovered_tile_idx = -1;
  ctrl->selected_tile_idx = -1;
  
  S32 hovered_tile_idx = -1;
  for (S32 tile_idx = 0; tile_idx < tiles_count; tile_idx += 1) {
    F32 col = tile_idx % map->width;
    F32 row = tile_idx / map->width;
    
    F32 tile_x_min = scale * col + base_x;
    F32 tile_y_min = scale * row + base_y;
    F32 tile_x_max = tile_x_min + scale;
    F32 tile_y_max = tile_y_min + scale;
    
    B32 mouse_in_tile = 
    (mx >= tile_x_min) && (my >= tile_y_min) && (mx < tile_x_max) && (my < tile_y_max);
    if (mouse_in_tile) {
      hovered_tile_idx = tile_idx;
    }
  }
  
  if (hovered_tile_idx >= 0 && hovered_tile_idx < tiles_count) {
    ctrl->hovered_tile_idx = hovered_tile_idx;
    if (g_mouse_pressed(input, G_MouseButton_Left)) {
      ctrl->selected_tile_idx = hovered_tile_idx;
    }
  }
  
  B32 indices_valid = (ctrl->selected_tile_idx >= 0 && choice_ctrl->selected_choice_idx >= 0);
  if (indices_valid) {
    ed_undo_push(undo, map);
    
    ED_ChoiceEntry *choice = &ed_choice_table[choice_ctrl->selected_choice_idx];
    map->tokens[ctrl->selected_tile_idx] = choice->token;
    
    ctrl->selected_tile_idx = -1;
  }
  
  if (g_mouse_down(input, G_MouseButton_Right)) {
    static F32 mouse_pan_speed = 2;
    F32 dmx = mx - pmx;
    F32 dmy = my - pmy;
    ctrl->base_x += dmx * mouse_pan_speed;
    ctrl->base_y += dmy * mouse_pan_speed;
  }
  ctrl->prev_mouse_x = mx;
  ctrl->prev_mouse_y = my;
}

function void
ed_map_ctrl_render(R_Context *renderer, G_AssetContext *assets, G_MapStorage *maps, ED_MapControl *ctrl)
{
  G_TileMap *map = ed_map_get(maps, ctrl->current_map_idx);
  
  S32 map_width = map->width;
  S32 map_height = map->height;
  G_TileToken *tokens = map->tokens;
  S32 token_count = map_width*map_height;
  
  F32 scale = ctrl->scale;
  F32 base_x = ctrl->base_x;
  F32 base_y = ctrl->base_y;
  
  F32 spr_x;
  F32 spr_y;
  G_Sprite *sprite = g_assets_get_sprite(assets, S8("sprites/empty.png"));
  
  // Background tiles
  for (U32 token_idx = 0; token_idx < token_count; token_idx += 1) {
    spr_x = base_x + scale * (token_idx % map_width);
    spr_y = base_y + scale * (token_idx / map_width);
    
    g_render_sprite(renderer, sprite, spr_x, spr_y, scale);
  }
  
  // All other tiles
  for (S32 token_idx = 0; token_idx < token_count; token_idx += 1) {
    sprite = g_assets_get_sprite(assets, S8("sprites/null.png"));
    
    spr_x = base_x + scale * (token_idx % map_width);
    spr_y = base_y + scale * (token_idx / map_width);
    
    G_TileToken token = tokens[token_idx];
    switch (token) {
      case G_TileToken_Empty: {
        sprite = g_assets_get_sprite(assets, S8("sprites/empty.png"));;
      }break;
      case G_TileToken_Dirt: {
        sprite = g_assets_get_sprite(assets, S8("sprites/dirt.png"));
      }break;
      case G_TileToken_StoneT0: {
        sprite = g_assets_get_sprite(assets, S8("sprites/stone_t0.png"));
      }break;
      case G_TileToken_StoneT1: {
        sprite = g_assets_get_sprite(assets, S8("sprites/stone_t1.png"));
      }break;
      case G_TileToken_OreT0: {
        sprite = g_assets_get_sprite(assets, S8("sprites/ore_t0.png"));
      }break;
      case G_TileToken_OreT1: {
        sprite = g_assets_get_sprite(assets, S8("sprites/ore_t1.png"));
      }break;
      case G_TileToken_OreT2: {
        sprite = g_assets_get_sprite(assets, S8("sprites/ore_t2.png"));
      }break;
      case G_TileToken_Bedrock: {
        sprite = g_assets_get_sprite(assets, S8("sprites/bedrock.png"));
      }break;
      case G_TileToken_Player: {
        sprite = g_assets_get_sprite(assets, S8("sprites/player.png"));
      }break;
      case G_TileToken_EnemyGround: {
        sprite = g_assets_get_sprite(assets, S8("sprites/enemy_ground.png"));
      }break;
      case G_TileToken_EnemyAir: {
        sprite = g_assets_get_sprite(assets, S8("sprites/enemy_air.png"));
      }break;
      case G_TileToken_BoulderSmall: {
        sprite = g_assets_get_sprite(assets, S8("sprites/boulder_small.png"));
      }break;
      case G_TileToken_BoulderLarge: {
        sprite = g_assets_get_sprite(assets, S8("sprites/boulder_large.png"));
      }break;
      case G_TileToken_Lava: {
        sprite = g_assets_get_sprite(assets, S8("sprites/lava.png"));
      }break;
    }
    
    V4F32 col = v4f32(1,1,1,1);
    if (ctrl->hovered_tile_idx == token_idx) {
      col = v4f32(0.88f,0.88f,0.88f,1.f);
    }
    ed_render_sprite(renderer, sprite, spr_x, spr_y, scale, col);
  }
}


//
// Miscellaneous helpers
//

function G_TileMap *
ed_map_get(G_MapStorage *maps, S32 map_idx)
{
  return &maps->maps[map_idx];
}

function U32
ed_codepoint_from_key(G_Key key)
{
  U32 codepoint = 0;
  
  if ((key >= G_Key_0) && (key <= G_Key_9)) {
    U32 delta = (U32)(key - G_Key_0);
    codepoint = '0' + delta;
  }
  else {
    // @Note: Temporary
    switch (key) {
      case G_Key_E: {
        codepoint = 'E';
      }break;
      case G_Key_R: {
        codepoint = 'R';
      }break;
    }
  }
  
  return codepoint;
}

//
// Rendering
//

function void
ed_render_sprite(R_Context *renderer, G_Sprite *sprite, F32 x, F32 y, F32 scale, V4F32 color)
{
  if (sprite) {
    R_Quad quad = {0};
    
    quad.rect = {x, y, x + scale, y + scale};
    quad.uv_rect = sprite->uv_rect;
    
    quad.colors[0] = color;
    quad.colors[1] = color;
    quad.colors[2] = color;
    quad.colors[3] = color;
    
    r_quad(renderer, &quad, sprite->texture);
  }
}
