#pragma once

#if defined(_WIN32)
#  include <windows.h>
#  include <GL/gl.h>
#  include <GL/glu.h>
#elif defined(__APPLE__)
#  include <OpenGL/gl.h>
#  include <OpenGL/glu.h>
#else
#  include <GL/gl.h>
#  include <GL/glu.h>
#endif
