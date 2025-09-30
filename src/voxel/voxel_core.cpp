function void
vox_get_input(VOX_Context *ctx, OS_EventList *events)
{
  VOX_Input *input = &ctx->input;
  OS_Handle window = ctx->renderer->window;
  
  for (OS_Event *e = events->first; e != 0; e = e->next) {
    VOX_Key slot = VOX_Key_Null;
    VOX_MouseButton mouse_slot = VOX_MouseButton_Null;
    
    switch (e->key) {
      case OS_Key_Esc:   { slot = VOX_Key_Esc;   }break;
      case OS_Key_Space: { slot = VOX_Key_Space; }break;
      case OS_Key_Enter: { slot = VOX_Key_Enter; }break;
      case OS_Key_Up:    { slot = VOX_Key_Up;    }break;
      case OS_Key_Down:  { slot = VOX_Key_Down;  }break;
      case OS_Key_Left:  { slot = VOX_Key_Left;  }break;
      case OS_Key_Right: { slot = VOX_Key_Right; }break;
      case OS_Key_W:     { slot = VOX_Key_W;     }break;
      case OS_Key_A:     { slot = VOX_Key_A;     }break;
      case OS_Key_S:     { slot = VOX_Key_S;     }break;
      case OS_Key_D:     { slot = VOX_Key_D;     }break;
      case OS_Key_Q:     { slot = VOX_Key_Q;     }break;
      case OS_Key_E:     { slot = VOX_Key_E;     }break;
      case OS_Key_R:     { slot = VOX_Key_R;     }break;
      case OS_Key_Z:     { slot = VOX_Key_Z;     }break;
      case OS_Key_0:     { slot = VOX_Key_0;     }break;
      case OS_Key_1:     { slot = VOX_Key_1;     }break;
      case OS_Key_2:     { slot = VOX_Key_2;     }break;
      case OS_Key_3:     { slot = VOX_Key_3;     }break;
      case OS_Key_4:     { slot = VOX_Key_4;     }break;
      case OS_Key_5:     { slot = VOX_Key_5;     }break;
      case OS_Key_6:     { slot = VOX_Key_6;     }break;
      case OS_Key_7:     { slot = VOX_Key_7;     }break;
      case OS_Key_8:     { slot = VOX_Key_8;     }break;
      case OS_Key_9:     { slot = VOX_Key_9;     }break;
      case OS_Key_F1:    { slot = VOX_Key_F1;    }break;
      case OS_Key_F2:    { slot = VOX_Key_F2;    }break;
      case OS_Key_F3:    { slot = VOX_Key_F3;    }break;
      case OS_Key_F4:    { slot = VOX_Key_F4;    }break;
      case OS_Key_F5:    { slot = VOX_Key_F5;    }break;
      case OS_Key_F6:    { slot = VOX_Key_F6;    }break;
      case OS_Key_F7:    { slot = VOX_Key_F7;    }break;
      case OS_Key_F8:    { slot = VOX_Key_F8;    }break;
      case OS_Key_F9:    { slot = VOX_Key_F9;    }break;
      case OS_Key_F10:   { slot = VOX_Key_F10;   }break;
      case OS_Key_F11:   { slot = VOX_Key_F11;   }break;
      case OS_Key_F12:   { slot = VOX_Key_F12;   }break;
      case OS_Key_Minus: { slot = VOX_Key_Minus; }break;
      case OS_Key_Equal: { slot = VOX_Key_Equal; }break;
      
      case OS_Key_MouseLeft:   { mouse_slot = VOX_MouseButton_Left;   }break;
      case OS_Key_MouseMiddle: { mouse_slot = VOX_MouseButton_Middle; }break;
      case OS_Key_MouseRight:  { mouse_slot = VOX_MouseButton_Right;  }break;
    }
    
    switch (e->kind) {
      case OS_EventKind_KeyPress:   { input->keys[slot] = 1; }break;
      case OS_EventKind_KeyRelease: { input->keys[slot] = 0; }break;
      
      case OS_EventKind_MousePress:   { input->mouse.buttons[mouse_slot] = 1; }break;
      case OS_EventKind_MouseRelease: { input->mouse.buttons[mouse_slot] = 0; }break;
    }
  }
  
  V2S32 mouse_pos = os_window_cursor_pos(window);
  input->mouse.pos.x = mouse_pos.x;
  input->mouse.pos.y = mouse_pos.y;
}

function B32
vox_key_pressed(VOX_Input *input, VOX_Key key)
{
  B32 result = input->keys[key];
  input->keys[key] = 0;
  return result;
}

function B32
vox_key_down(VOX_Input *input, VOX_Key key)
{
  return input->keys[key];
}

function B32 
vox_mouse_pressed(VOX_Input *input, VOX_MouseButton btn)
{
  B32 result = input->mouse.buttons[btn];
  input->mouse.buttons[btn] = 0;
  return result;
}

function B32 
vox_mouse_down(VOX_Input *input, VOX_MouseButton btn)
{
  return input->mouse.buttons[btn];
}

function V2S32
vox_mouse_position(VOX_Input *input)
{
  return input->mouse.pos;
}

function VOX_Voxel *
vox_get_voxel(VOX_Chunk *chunk, S32 idx)
{
  VOX_Voxel *v = &chunk->voxels[idx];
  return v;
}