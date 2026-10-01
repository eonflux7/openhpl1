// ImGui's OpenGL 3 backend built against glad, the engine's GL loader,
// instead of the loader ImGui bundles.
#include <glad/gl.h>
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#include "backends/imgui_impl_opengl3.cpp"
