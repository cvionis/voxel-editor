#pragma once

#pragma warning (push, 0)
#include "stb_image.h"
#pragma warning (pop)

#include "render/render_core.h"
#include "render/backend/render_backend.h"

#if R_BACKEND_GL 
# include "render/backend/gl/render_backend_gl.h"
#elif R_BACKEND_D3D11
# include "render/backend/d3d11/render_backend_d3d11.h"
#else 
# error The selected rendering backend has not been implemented for your platform!
#endif
