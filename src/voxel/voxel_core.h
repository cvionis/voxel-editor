#pragma once

// @Todo: Why make these static? Just pass them to init and allow chunk size (slice size)
// to be changed.
#define VOX_SLICE_SIZE (32)
#define VOX_CHUNK_SIZE (VOX_SLICE_SIZE * VOX_SLICE_SIZE * VOX_SLICE_SIZE)

// @Todo: Can pack into single U8
struct VOX_Voxel {
  U8 opacity;
  U8 color;
  U8 id0;
  U8 id1;
};

struct VOX_Chunk {
  VOX_Voxel voxels[VOX_CHUNK_SIZE];
};

enum VOX_Key {
  VOX_Key_Null,
  VOX_Key_Esc,
  VOX_Key_Space,
  VOX_Key_Enter,
  VOX_Key_Up,
  VOX_Key_Left,
  VOX_Key_Down,
  VOX_Key_Right,
  VOX_Key_W,
  VOX_Key_A,
  VOX_Key_S,
  VOX_Key_D,
  VOX_Key_Q,
  VOX_Key_E,
  VOX_Key_R,
  VOX_Key_Z,
  VOX_Key_0,
  VOX_Key_1,
  VOX_Key_2,
  VOX_Key_3,
  VOX_Key_4,
  VOX_Key_5,
  VOX_Key_6,
  VOX_Key_7,
  VOX_Key_8,
  VOX_Key_9,
  VOX_Key_F1,
  VOX_Key_F2,
  VOX_Key_F3,
  VOX_Key_F4,
  VOX_Key_F5,
  VOX_Key_F6,
  VOX_Key_F7,
  VOX_Key_F8,
  VOX_Key_F9,
  VOX_Key_F10,
  VOX_Key_F11,
  VOX_Key_F12,
  VOX_Key_Minus,
  VOX_Key_Equal,
  VOX_Key_COUNT,
};

enum VOX_MouseButton {
  VOX_MouseButton_Null,
  VOX_MouseButton_Left,
  VOX_MouseButton_Middle,
  VOX_MouseButton_Right,
  VOX_MouseButton_COUNT,
};

struct VOX_Input {
  B32 keys[VOX_Key_COUNT];
  struct {
    V2S32 pos;
    B32 buttons[VOX_MouseButton_COUNT];
  }mouse;
};

enum VOX_EditMode {
  VOX_EditMode_Delete,
  VOX_EditMode_Add,
};

struct VOX_EditState {
  S32 selected_voxel_idx = -1;  
  S32 nearest_empty_voxel_idx = -1;
  S32 brush_size = 1;
	VOX_EditMode mode;
};

function VOX_Voxel *vox_get_voxel(VOX_Chunk *chunk, S32 idx);

function B32 vox_key_pressed(VOX_Input *input, VOX_Key key);
function B32 vox_key_down(VOX_Input *input, VOX_Key key);
function B32 vox_mouse_pressed(VOX_Input *input, VOX_MouseButton btn);
function B32 vox_mouse_down(VOX_Input *input, VOX_MouseButton btn);