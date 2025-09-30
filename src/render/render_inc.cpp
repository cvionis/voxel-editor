// TODO: Put image loading in its own layer?
#pragma warning (push, 0)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#pragma warning (pop)

#if R_BACKEND_GL
# ifdef OS_WINDOWS
#  include "render/backend/gl/win32/render_backend_gl_win32.cpp"
# else 
#  error The selected rendering backend has not been implemented for your platform!
# endif
# include "render/backend/gl/render_backend_gl.cpp"
#elif R_BACKEND_D3D11
# include "render/backend/d3d11/render_backend_d3d11.cpp"
#else 
# error The selected rendering backend has not been implemented for your platform!
#endif

#include "render/render_core.cpp"
