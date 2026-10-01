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
	add_compile_options(-fsanitize=${OVERTURE_SANITIZERS} -fno-omit-frame-pointer)
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

# AngelScript 2.38.0 (zlib)
file(GLOB angelscript_sources CONFIGURE_DEPENDS
	vendor/angelscript/source/*.cpp
	vendor/angelscript/add_on/scriptstdstring/*.cpp)
add_library(angelscript STATIC ${angelscript_sources})
target_include_directories(angelscript SYSTEM PUBLIC
	vendor/angelscript/include
	vendor/angelscript/add_on)
target_compile_options(angelscript PRIVATE -w)
if(OVERTURE_SANITIZERS)
	# Bytecode stores pointers at 4-byte offsets by design; fine on x86-64
	target_compile_options(angelscript PRIVATE -fno-sanitize=alignment)
endif()

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

#--------------------------------------------------------------------
# HPL1 engine

file(GLOB_RECURSE hpl_sources CONFIGURE_DEPENDS
	engine/sources/*.cpp
	engine/sources/*.c)
list(FILTER hpl_sources EXCLUDE REGEX "/(win32\\.cpp|Video[A-Za-z]*Theora\\.cpp)$")

add_library(hpl STATIC ${hpl_sources})
target_include_directories(hpl PUBLIC engine/include)
target_link_libraries(hpl PUBLIC
	newton angelscript oalwrapper tinyxml glad
	SDL3::SDL3 SDL3_image::SDL3_image
	OpenGL::GL)
overture_warnings(hpl)

#--------------------------------------------------------------------
# Penumbra: Overture

file(GLOB_RECURSE game_sources CONFIGURE_DEPENDS game/*.cpp)
add_executable(overture ${game_sources})
target_include_directories(overture PRIVATE game)
target_link_libraries(overture PRIVATE hpl)
overture_warnings(overture)

# Shaders are loaded from <executable dir>/shaders/
add_custom_command(TARGET overture POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_CURRENT_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:overture>/shaders)

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
