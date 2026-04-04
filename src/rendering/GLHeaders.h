#pragma once

// WebAssembly targets WebGL2 (OpenGL ES 3.0). Desktop targets vary by
// platform. Include the appropriate headers so code that uses this file
// compiles correctly in both environments.
//
// NOTE: Viewport3D currently uses the fixed-function OpenGL 1.x pipeline
// (glBegin/glEnd, matrix stack, etc.) which is NOT available in WebGL2.
// Those rendering paths must be replaced with shader-based code before the
// web build can render scenes.  In the meantime the WASM build omits
// Viewport3D from its source list entirely (see CMakeLists.txt).
#if defined(__EMSCRIPTEN__) || defined(SEICHE_WEB_BUILD)
// WebGL2 is based on OpenGL ES 3.0; include GLES3 if available, else GLES2.
#ifdef __has_include
#  if __has_include(<GLES3/gl3.h>)
#    include <GLES3/gl3.h>
#  else
#    include <GLES2/gl2.h>
#  endif
#else
#  include <GLES2/gl2.h>
#endif
// GLU is not available in WebGL/GLES — do not include it.
#elif defined(_WIN32)
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
