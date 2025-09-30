#include <GL/gl.h>
#include "glcorearb.h"
#include "wglext.h"

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "opengl32.lib")

struct R_GL_Win32_State {
  HDC dc; 
};

global R_GL_Win32_State r_gl_win32_state;

// Note: The code used here to create a modern OpenGL Context on Windows was taken 
// from https://gist.github.com/mmozeiko/ed2ad27f75edf9c26053ce332a1f6647

#define declare_gl_functions(X) \
X(PFNGLCREATEBUFFERSPROC,               glCreateBuffers               ) \
X(PFNGLNAMEDBUFFERSTORAGEPROC,          glNamedBufferStorage          ) \
X(PFNGLNAMEDBUFFERDATAPROC,             glNamedBufferData             ) \
X(PFNGLNAMEDBUFFERSUBDATAPROC,          glNamedBufferSubData          ) \
X(PFNGLBINDVERTEXARRAYPROC,             glBindVertexArray             ) \
X(PFNGLCREATEVERTEXARRAYSPROC,          glCreateVertexArrays          ) \
X(PFNGLVERTEXARRAYATTRIBBINDINGPROC,    glVertexArrayAttribBinding    ) \
X(PFNGLVERTEXARRAYVERTEXBUFFERPROC,     glVertexArrayVertexBuffer     ) \
X(PFNGLVERTEXARRAYELEMENTBUFFERPROC,    glVertexArrayElementBuffer    ) \
X(PFNGLVERTEXARRAYATTRIBFORMATPROC,     glVertexArrayAttribFormat     ) \
X(PFNGLVERTEXARRAYBINDINGDIVISORPROC,   glVertexArrayBindingDivisor   ) \
X(PFNGLENABLEVERTEXARRAYATTRIBPROC,     glEnableVertexArrayAttrib     ) \
X(PFNGLCREATESHADERPROGRAMVPROC,        glCreateShaderProgramv        ) \
X(PFNGLGETPROGRAMIVPROC,                glGetProgramiv                ) \
X(PFNGLGETPROGRAMINFOLOGPROC,           glGetProgramInfoLog           ) \
X(PFNGLGENPROGRAMPIPELINESPROC,         glGenProgramPipelines         ) \
X(PFNGLCREATEPROGRAMPIPELINESPROC,      glCreateProgramPipelines      ) \
X(PFNGLUSEPROGRAMSTAGESPROC,            glUseProgramStages            ) \
X(PFNGLBINDPROGRAMPIPELINEPROC,         glBindProgramPipeline         ) \
X(PFNGLBINDTEXTUREUNITPROC,             glBindTextureUnit             ) \
X(PFNGLCREATETEXTURESPROC,              glCreateTextures              ) \
X(PFNGLTEXTUREPARAMETERIPROC,           glTextureParameteri           ) \
X(PFNGLTEXTURESTORAGE2DPROC,            glTextureStorage2D            ) \
X(PFNGLTEXTURESUBIMAGE2DPROC,           glTextureSubImage2D           ) \
X(PFNGLTEXTURESTORAGE3DPROC,            glTextureStorage3D            ) \
X(PFNGLTEXTURESUBIMAGE3DPROC,           glTextureSubImage3D           ) \
X(PFNGLCOMPRESSEDTEXTURESUBIMAGE2DPROC, glCompressedTextureSubImage2D ) \
X(PFNGLCOMPRESSEDTEXTURESUBIMAGE3DPROC, glCompressedTextureSubImage3D ) \
X(PFNGLDEBUGMESSAGECALLBACKPROC,        glDebugMessageCallback        ) \
X(PFNGLGETUNIFORMLOCATIONPROC,          glGetUniformLocation          ) \
X(PFNGLPROGRAMUNIFORMMATRIX2FVPROC,     glProgramUniformMatrix2fv     ) \
X(PFNGLPROGRAMUNIFORMMATRIX4FVPROC,     glProgramUniformMatrix4fv     ) \
X(PFNGLPROGRAMUNIFORM2FVPROC,           glProgramUniform2fv           ) \
X(PFNGLPROGRAMUNIFORM3FVPROC,           glProgramUniform3fv           ) \
X(PFNGLPROGRAMUNIFORM4FVPROC,           glProgramUniform4fv           ) \
X(PFNGLPROGRAMUNIFORM1IPROC,            glProgramUniform1i            ) \
X(PFNGLPROGRAMUNIFORM1UIPROC,           glProgramUniform1ui           ) \
X(PFNGLPROGRAMUNIFORM1FPROC,            glProgramUniform1f            ) \
X(PFNGLCREATEFRAMEBUFFERSPROC,          glCreateFramebuffers          ) \
X(PFNGLBINDFRAMEBUFFERPROC,             glBindFramebuffer             ) \
X(PFNGLNAMEDFRAMEBUFFERTEXTUREPROC,     glNamedFramebufferTexture     ) \
X(PFNGLDRAWARRAYSINSTANCEDPROC,         glDrawArraysInstanced         ) \
X(PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC, glCheckNamedFramebufferStatus ) \
X(PFNGLDRAWBUFFERSPROC,                 glDrawBuffers                 ) \
X(PFNGLCREATERENDERBUFFERSPROC,         glCreateRenderbuffers         ) \
X(PFNGLNAMEDRENDERBUFFERSTORAGEPROC,    glNamedRenderbufferStorage    ) \
X(PFNGLNAMEDFRAMEBUFFERRENDERBUFFERPROC,glNamedFramebufferRenderbuffer) \
X(PFNGLNAMEDFRAMEBUFFERDRAWBUFFERSPROC, glNamedFramebufferDrawBuffers ) \
X(PFNGLBLITNAMEDFRAMEBUFFERPROC,        glBlitNamedFramebuffer        ) \
X(PFNGLGENERATETEXTUREMIPMAPPROC,       glGenerateTextureMipmap       ) \
X(PFNGLBINDBUFFERBASEPROC,              glBindBufferBase              ) \

#define declare_function(type, name) static type name;
declare_gl_functions(declare_function)
#undef  declare_function 

static PFNWGLCHOOSEPIXELFORMATARBPROC    wglChoosePixelFormatARB    = 0;
static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = 0;
static PFNWGLSWAPINTERVALEXTPROC         wglSwapIntervalEXT         = 0;

function void 
r_gl_win32_get_extensions(void)
{
  //
  // Create a WGL context so we can get OpenGL extensions
  //
  
  // Because creating a wgl context requires a window, we need to create a 
  // temporary dummy window we can use to obtain a temporary context.
  HWND dummy = CreateWindowExW(
                               0, L"STATIC", L"DummyWindow", WS_OVERLAPPED,
                               CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                               NULL, NULL, NULL, NULL);
  Assert(dummy && "Failed to create dummy window for WGL");
  
  HDC dc = GetDC(dummy);
  Assert(dc && "Failed to get device context for WGL dummy window");
  
  PIXELFORMATDESCRIPTOR desc;
  desc.nSize      = sizeof(desc);
  desc.nVersion   = 1;
  desc.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  desc.iPixelType = PFD_TYPE_RGBA;
  desc.cColorBits = 24;
  
  S32 format = ChoosePixelFormat(dc, &desc);
  Assert(format && "Failed to choose OpenGL pixel format for WGL dummy window");
  
  S32 ok = DescribePixelFormat(dc, format, sizeof(desc), &desc);
  Assert(ok && "Failed to describe OpenGL pixel format");
  
  // Another reason to create a dummy window is that SetPixelFormat can be called 
  // only once per window.
  ok = SetPixelFormat(dc, format, &desc); 
  Assert(ok && "Failed to set OpenGL pixel format for WGL dummy window");
  
  HGLRC rc = wglCreateContext(dc);
  Assert(rc && "Failed to create OpenGL context for WGL dummy window");
  ok = wglMakeCurrent(dc, rc);
  Assert(ok && "Failed to make current OpenGL context for dummy window");
  
  //
  // Get OpenGL extensions
  //
  
#pragma warning(disable: 4152)
  
  // https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_extensions_string.txt
  auto wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
  Assert(wglGetExtensionsStringARB);
  
  const char *ext = wglGetExtensionsStringARB(dc);
  Assert(ext && "Failed to get OpenGL WGL extensions string");
  
  const char *start = ext;
  for (;;) {
    while (*ext != 0 && *ext != ' ') ext++;
    U64 length = ext - start;
    
    if (cstr_equal_n("WGL_ARB_pixel_format ", start, length)) {
      // https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_pixel_format.txt
      wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
    } 
    else if (cstr_equal_n("WGL_ARB_create_context ", start, length)) {
      // https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt
      wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    } 
    else if (cstr_equal_n("WGL_EXT_swap_control ", start, length)) {
      // https://www.khronos.org/registry/OpenGL/extensions/EXT/WGL_EXT_swap_control.txt
      wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    }
    
    if (*ext == 0) break;
    ext++; start = ext;
  }
  
#pragma warning(default: 4152)
  
  Assert(wglChoosePixelFormatARB && wglCreateContextAttribsARB && wglSwapIntervalEXT);
  
  //
  // Destroy temporary context and window
  //
  
  wglMakeCurrent(0, 0);
  wglDeleteContext(rc);
  ReleaseDC(dummy, dc);
  DestroyWindow(dummy);
}

function void
r_gl_win32_context_from_window(OS_Handle window)
{
  HWND hwnd = os_win32_window_from_handle(window)->hwnd;
  HDC dc = GetDC(hwnd);
  Assert(dc && "Failed to create Windows Device Context for window");
  r_gl_win32_state.dc = dc;
  
  // Set pixel format for OpenGL context
  {
    S32 attribs[] = {
      WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
      WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
      WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
      WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
      WGL_COLOR_BITS_ARB,     24,
      WGL_DEPTH_BITS_ARB,     24,
      WGL_STENCIL_BITS_ARB,   8, 0,
    };
    
    S32 format; UINT formats;
    Assert(wglChoosePixelFormatARB(dc, attribs, NULL, 1, &format, &formats) && formats);
    
    PIXELFORMATDESCRIPTOR desc = {0};
    desc.nSize = sizeof(desc);
    
    S32 ok = DescribePixelFormat(dc, format, sizeof(desc), &desc);
    Assert(ok && "Failed to describe OpenGL pixel format");
    
    Assert(SetPixelFormat(dc, format, &desc) && "Cannot set OpenGL selected pixel format");
  }
  
  // Create a modern (4.5) OpenGL context
  {
    int attrib[] = {
      WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
      WGL_CONTEXT_MINOR_VERSION_ARB, 5,
      WGL_CONTEXT_PROFILE_MASK_ARB, 
      WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0,
    };
    
    HGLRC rc = wglCreateContextAttribsARB(dc, NULL, attrib);
    Assert(rc && "Cannot create modern OpenGL context! OpenGL version 4.5 not supported?");
    
    BOOL ok = wglMakeCurrent(dc, rc);
    Assert(ok && "Failed to make current OpenGL context");
  }
  
  // Load the OpenGL functions we declared earlier
#define load_func_wgl(type, name) name = (type)wglGetProcAddress(#name); 
  declare_gl_functions(load_func_wgl)
#undef load_func_wgl 
}

function void
r_backend_init(void)
{
  r_gl_win32_get_extensions(); 
}

function void
r_backend_equip_window(OS_Handle window)
{
  r_gl_win32_context_from_window(window);
  r_gl_backend_init(); 
}

function void
r_backend_end_frame(void)
{
  HDC dc = r_gl_win32_state.dc;
  SwapBuffers(dc);
}
