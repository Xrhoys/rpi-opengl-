#include "renderer.h"

#ifdef BE_OPENGL

#include "renderer_opengl.h"
#include "renderer_opengl.cpp"

#elif BE_VULKAN

#include "renderer_vulkan.h"
#include "renderer_vulkan.cpp"

#endif

