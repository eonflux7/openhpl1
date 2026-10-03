cmake_minimum_required(VERSION 3.20)
project(OvertureRemade LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

option(OVERTURE_WARNINGS "Compiler warnings for the engine and game (not vendor/)" ON)
set(OVERTURE_SANITIZERS "" CACHE STRING
	"Comma-separated -fsanitize= list for all targets, e.g. address,undefined")

if(OVERTURE_SANITIZERS)
	# Alignment is off: AngelScript packs pointers and std::string arguments
	# at 4-byte offsets by design, which is fine on x86-64.
	add_compile_options(-fsanitize=${OVERTURE_SANITIZERS} -fno-sanitize=alignment
		-fno-omit-frame-pointer)
	add_link_options(-fsanitize=${OVERTURE_SANITIZERS})
endif()

function(overture_warnings target)
	if(OVERTURE_WARNINGS)
		# The -Wno- groups are high-volume noise in the original code
		# (stdcall on script callbacks, member init order, unhandled enum
		# values, unused locals). Re-enable one at a time to clean up.
		target_compile_options(${target} PRIVATE
			-Wall -Wextra
			-Wno-unused-parameter
			-Wno-attributes
			-Wno-reorder
			-Wno-switch
			-Wno-unused-variable
			-Wno-unused-but-set-variable
			-Wno-sign-compare)
	endif()
endfunction()

find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_image REQUIRED CONFIG)
find_package(OpenGL REQUIRED)
find_package(OpenAL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(VORBIS REQUIRED IMPORTED_TARGET vorbisfile vorbis ogg)

# FreeType renders the fonts. The system library is used if its headers are
# installed; otherwise the source is downloaded and built into the engine.
find_package(Freetype)
if(NOT Freetype_FOUND)
	message(STATUS "FreeType headers not found, building FreeType 2.13.3 from source")
	include(FetchContent)
	set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
	set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
	set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
	set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
	set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
	FetchContent_Declare(freetype
		URL https://download.savannah.gnu.org/releases/freetype/freetype-2.13.3.tar.xz
		URL_HASH SHA256=0550350666d427c74daeb85d5ac7bb353acba5f76956395995311a9c6f063289
		EXCLUDE_FROM_ALL)
	set(overture_build_shared ${BUILD_SHARED_LIBS})
	set(BUILD_SHARED_LIBS OFF)
	FetchContent_MakeAvailable(freetype)
	set(BUILD_SHARED_LIBS ${overture_build_shared})
	target_compile_options(freetype PRIVATE -w)
	add_library(Freetype::Freetype ALIAS freetype)
endif()

#--------------------------------------------------------------------
# Vendored libraries

# glad 2 OpenGL loader (GL 4.6 compatibility + legacy extensions the engine probes)
add_library(glad STATIC vendor/glad/src/gl.c)
target_include_directories(glad SYSTEM PUBLIC vendor/glad/include)

# Newton Game Dynamics 2.36 (zlib), taken from ScummVM's HPL1 port
file(GLOB_RECURSE newton_sources CONFIGURE_DEPENDS vendor/newton/*.cpp)
add_library(newton STATIC ${newton_sources})
target_include_directories(newton SYSTEM PUBLIC vendor/newton)
target_compile_options(newton PRIVATE -w)
if(OVERTURE_SANITIZERS)
	# Upstream Newton relies on shifts of negative values and the like;
	# keep ASan on it but not UBSan.
	target_compile_options(newton PRIVATE -fno-sanitize=undefined)
endif()

# AngelScript 2.38.0 (zlib)
file(GLOB angelscript_sources CONFIGURE_DEPENDS
	vendor/angelscript/source/*.cpp
	vendor/angelscript/add_on/scriptstdstring/*.cpp
	vendor/angelscript/add_on/scriptarray/*.cpp)
add_library(angelscript STATIC ${angelscript_sources})
target_include_directories(angelscript SYSTEM PUBLIC
	vendor/angelscript/include
	vendor/angelscript/add_on)
target_compile_options(angelscript PRIVATE -w)

# OALWrapper (zlib), Frictional Games' OpenAL helper
file(GLOB oalwrapper_sources CONFIGURE_DEPENDS vendor/oalwrapper/sources/*.cpp)
add_library(oalwrapper STATIC ${oalwrapper_sources})
target_include_directories(oalwrapper SYSTEM PUBLIC vendor/oalwrapper/include)
target_link_libraries(oalwrapper PUBLIC OpenAL::OpenAL PkgConfig::VORBIS SDL3::SDL3)
target_compile_options(oalwrapper PRIVATE -w)

# TinyXML 2.x (zlib), as shipped with HPL1
file(GLOB tinyxml_sources CONFIGURE_DEPENDS vendor/tinyxml/*.cpp)
add_library(tinyxml STATIC ${tinyxml_sources})
target_include_directories(tinyxml SYSTEM PUBLIC vendor/tinyxml)
target_compile_options(tinyxml PRIVATE -w)

# Dear ImGui 1.92.9b (MIT), the debug overlay; SDL3 platform backend and
# the OpenGL 3 backend on glad (vendor/imgui_glad)
add_library(imgui STATIC
	vendor/imgui/imgui.cpp
	vendor/imgui/imgui_demo.cpp
	vendor/imgui/imgui_draw.cpp
	vendor/imgui/imgui_tables.cpp
	vendor/imgui/imgui_widgets.cpp
	vendor/imgui/backends/imgui_impl_sdl3.cpp
	vendor/imgui_glad/imgui_impl_opengl3_glad.cpp)
target_include_directories(imgui SYSTEM PUBLIC vendor/imgui vendor/imgui/backends)
target_link_libraries(imgui PUBLIC glad SDL3::SDL3)
target_compile_options(imgui PRIVATE -w)

#--------------------------------------------------------------------
# HPL1 engine

file(GLOB_RECURSE hpl_sources CONFIGURE_DEPENDS
	engine/sources/*.cpp
	engine/sources/*.c)
list(FILTER hpl_sources EXCLUDE REGEX "/win32\\.cpp$")

add_library(hpl STATIC ${hpl_sources})
target_include_directories(hpl PUBLIC engine/include)
# The area and search textures of the SMAA reference implementation
target_include_directories(hpl SYSTEM PRIVATE vendor/smaa)
target_link_libraries(hpl PUBLIC
	newton angelscript oalwrapper tinyxml glad imgui
	SDL3::SDL3 SDL3_image::SDL3_image
	Freetype::Freetype
	OpenGL::GL)
overture_warnings(hpl)

#--------------------------------------------------------------------
# Penumbra: Overture

file(GLOB_RECURSE game_sources CONFIGURE_DEPENDS game/*.cpp)
add_executable(overture ${game_sources})
target_include_directories(overture PRIVATE game)
target_precompile_headers(overture PRIVATE game/core/StdAfx.h)
target_link_libraries(overture PRIVATE hpl)
overture_warnings(overture)

include(CTest)
if(BUILD_TESTING)
	# Opt-in in-game checks use the original dog mesh, clips and physics.
	target_sources(overture PRIVATE tests/dog_regressions.cpp)
	target_compile_definitions(overture PRIVATE OVERTURE_DOG_REGRESSIONS)
	add_executable(engine_regressions tests/engine_regressions.cpp)
	target_link_libraries(engine_regressions PRIVATE hpl)
	overture_warnings(engine_regressions)
	add_test(NAME engine_regressions COMMAND engine_regressions)
	set_tests_properties(engine_regressions PROPERTIES
		ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1")
endif()
# The reference image viewpoints are read from, and saved to, the source tree
target_compile_definitions(overture PRIVATE
	OVERTURE_REFVIEWS_FILE="${CMAKE_CURRENT_SOURCE_DIR}/tools/refviews.txt")

# Shaders are loaded from <executable dir>/shaders/. Copied on every build:
# as a step after linking, an edited shader was not copied until some C++
# changed too.
add_custom_target(overture_shaders ALL
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_CURRENT_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:overture>/shaders)
add_dependencies(overture_shaders overture)

# Fonts are loaded from <executable dir>/fonts/
add_custom_command(TARGET overture POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_CURRENT_SOURCE_DIR}/fonts $<TARGET_FILE_DIR:overture>/fonts)

# The physics test map is generated into <executable dir>/testmap/
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
	set(testmap_dir ${CMAKE_CURRENT_BINARY_DIR}/testmap)
	add_custom_command(
		OUTPUT ${testmap_dir}/testmap.dae ${testmap_dir}/testmap.hps
		COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/testmap/generate.py ${testmap_dir}
		COMMAND ${CMAKE_COMMAND} -E copy
			${CMAKE_CURRENT_SOURCE_DIR}/testmap/testmap.hps ${testmap_dir}/testmap.hps
		DEPENDS testmap/generate.py testmap/testmap.hps
		COMMENT "Generating the test map")
	add_custom_target(testmap ALL DEPENDS ${testmap_dir}/testmap.dae ${testmap_dir}/testmap.hps)
	add_custom_command(
		OUTPUT ${testmap_dir}/dogtest.dae ${testmap_dir}/dogtest.cfg ${testmap_dir}/dogtest.hps
		COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/testmap/generate_dogtest.py ${testmap_dir}
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/testmap/dogtest.hps ${testmap_dir}/dogtest.hps
		DEPENDS testmap/generate_dogtest.py testmap/generate.py testmap/dogtest.hps
			${testmap_dir}/testmap.dae
		COMMENT "Generating the Dog behaviour test map")
	add_custom_target(dogtest ALL DEPENDS ${testmap_dir}/dogtest.dae ${testmap_dir}/dogtest.cfg ${testmap_dir}/dogtest.hps)
else()
	message(STATUS "Python 3 not found, the test map will not be built")
endif()

#--------------------------------------------------------------------
# Static analysis: `cmake --build build --target tidy` (checks in .clang-tidy)

find_program(RUN_CLANG_TIDY run-clang-tidy)
if(RUN_CLANG_TIDY)
	add_custom_target(tidy
		COMMAND ${RUN_CLANG_TIDY} -quiet -p ${CMAKE_BINARY_DIR}
			"/(engine/sources|game)/"
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		USES_TERMINAL VERBATIM)
endif()
