#pragma once

#define ED_DEFAULT_MAPS_PATH S8("../data/maps/default.maps")
#define ED_CHOICES_COUNT 11

//
// Undo system
//

struct ED_UndoNode {
  ED_UndoNode *stack_next;
  ED_UndoNode *free_next;
  
  G_TileToken tokens[G_TILEMAP_TOKENS_MAX];
  S32 width;
  S32 height;
};

struct ED_UndoContext {
  Arena *arena;
  
  ED_UndoNode *first; // Most recent undo state
  ED_UndoNode *last;  // Least recent undo state
  S32 count;
  
  ED_UndoNode *free;
};

function ED_UndoContext ed_undo_make(void);
function void ed_undo_release(ED_UndoContext *undo);
function void ed_undo_clear(ED_UndoContext *undo);
function void ed_undo_push(ED_UndoContext *undo, G_TileMap *map);
function void ed_undo_pop(ED_UndoContext *undo, G_TileMap *map);
//
// Tile choice controls
//

struct ED_ChoiceControl {
  F32 scale;
  F32 base_x;
  F32 base_y;
  F32 pad_x;
  F32 pad_y;
  S32 column_size;
  
  S32 hovered_choice_idx = -1;
  S32 selected_choice_idx = -1;
  
  G_TransitionF32 base_x_transition;
};

struct ED_ChoiceEntry {
  String8 path;
  G_TileToken token;
  G_Key keybind;
};

global ED_ChoiceEntry ed_choice_table[ED_CHOICES_COUNT] = {
  { S8("sprites/empty.png"), G_TileToken_Empty, G_Key_1 },
  { S8("sprites/dirt.png"), G_TileToken_Dirt, G_Key_2 },
  { S8("sprites/stone_t1.png"), G_TileToken_StoneT1, G_Key_3 },
  { S8("sprites/ore_t2.png"), G_TileToken_OreT2, G_Key_4 },
  { S8("sprites/bedrock.png"), G_TileToken_Bedrock, G_Key_5 },
  { S8("sprites/lava.png"), G_TileToken_Lava, G_Key_6 },
  { S8("sprites/boulder_small.png"), G_TileToken_BoulderSmall, G_Key_7 },
  { S8("sprites/boulder_large.png"), G_TileToken_BoulderLarge, G_Key_8 },
  { S8("sprites/player.png"), G_TileToken_Player, G_Key_9 },
  { S8("sprites/enemy_ground.png"), G_TileToken_EnemyGround, G_Key_E },
  { S8("sprites/enemy_air.png"), G_TileToken_EnemyAir, G_Key_R },
};

function void ed_choice_ctrl_update(ED_ChoiceControl *ctrl, G_Input *input);
function void ed_choice_ctrl_render(Arena *arena, R_Context *renderer, R_Font *font, G_AssetContext *assets, ED_ChoiceControl *ctrl);

//
// Tilemap-editing controls
//

struct ED_MapControl {
  F32 base_x;
  F32 base_y;
  F32 scale; 
  
  F32 prev_mouse_x;
  F32 prev_mouse_y;
  
  S32 current_map_idx;
  S32 hovered_tile_idx = -1;
  S32 selected_tile_idx = -1;
};

function void ed_map_ctrl_update(ED_MapControl *ctrl, ED_ChoiceControl *choice_ctrl, G_MapStorage *maps, ED_UndoContext *undo, G_Input *input);
function void ed_map_ctrl_render(R_Context *renderer, G_AssetContext *assets, G_MapStorage *maps, ED_MapControl *ctrl);

//
// Tilemap resizing controls
//

enum ED_ResizeDirection {
  ED_ResizeDirection_None = -1,
  ED_ResizeDirection_Left,
  ED_ResizeDirection_Right,
  ED_ResizeDirection_Top,
  ED_ResizeDirection_Bottom,
  ED_ResizeDirection_COUNT,
};

enum ED_ResizeMode {
  ED_ResizeMode_Append,
  ED_ResizeMode_Remove,
};

struct ED_ResizeOption {
  RectF32 rect;
};

struct ED_ResizeControl {
  F32 base_x;
  F32 base_y;
  F32 square_size;
  
  ED_ResizeMode mode;
  ED_ResizeOption options[ED_ResizeDirection_COUNT];
  ED_ResizeDirection hovered_direction = ED_ResizeDirection_None;
  
  G_TransitionF32 base_x_transition;
};

function void ed_resize_ctrl_update(ED_ResizeControl *ctrl, ED_MapControl *map_ctrl, G_MapStorage *maps, ED_UndoContext *undo, G_Input *input);
function void ed_resize_ctrl_render(R_Context *renderer, ED_ResizeControl *ctrl);

//
// Tilemap growing/shrinking helpers
//

function void ed_tilemap_column_append_left(Arena *arena, G_TileMap *map);
function void ed_tilemap_column_append_right(Arena *arena, G_TileMap *map);
function void ed_tilemap_column_remove_left(Arena *arena, G_TileMap *map);
function void ed_tilemap_column_remove_right(Arena *arena, G_TileMap *map);
function void ed_tilemap_row_append_top(Arena *arena, G_TileMap *map);
function void ed_tilemap_row_append_bottom(Arena *arena, G_TileMap *map);
function void ed_tilemap_row_remove_top(Arena *arena, G_TileMap *map);
function void ed_tilemap_row_remove_bottom(Arena *arena, G_TileMap *map);
function G_TileMap *ed_map_get(G_MapStorage *maps, S32 map_idx);
function U32 ed_codepoint_from_key(G_Key key);

//
// Rendering
//

function void ed_render_sprite(R_Context *renderer, G_Sprite *sprite, F32 x, F32 y, F32 scale, V4F32 color);

//
// Miscellaneous helpers
//

function G_TileMap *ed_map_get(G_MapStorage *maps, S32 map_idx);
function U32 ed_codepoint_from_key(G_Key key);