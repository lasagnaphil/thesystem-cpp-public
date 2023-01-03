#include "imgui.h"
#include "core/log.h"

#ifdef _MSC_VER
#define SOKOL_D3D11
#elif __APPLE__
#define SOKOL_METAL
#else 
#define SOKOL_GLCORE33
#endif

#define SOKOL_TRACE_HOOKS

#define SOKOL_ASSERT(c) if (!(c)) { g_logger.log(LOG_FATAL, __FILE__, __LINE__, "Sokol: Assertion failed!"); __debugbreak(); }
#define SOKOL_UNREACHABLE { g_logger.log(LOG_FATAL, __FILE__, __LINE__, "Sokol: Unreachable code!"); __debugbreak(); }

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_imgui.h"
#include "sokol_gfx_imgui.h"
#include "sokol_time.h"
#include "sokol_gp.h"