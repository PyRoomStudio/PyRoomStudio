#pragma once

#if defined(_WIN32)
#include <GL/gl.h>
#include <GL/glu.h>
#include <windows.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
