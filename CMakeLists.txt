cmake_minimum_required(VERSION 3.20)
project(OvertureRemade LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_image REQUIRED CONFIG)
find_package(OpenGL REQUIRED)
find_package(OpenAL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(VORBIS REQUIRED IMPORTED_TARGET vorbisfile vorbis ogg)

#--------------------------------------------------------------------
# Vendored libraries

# Newton Game Dynamics 2.36 (zlib), taken from ScummVM's HPL1 port
file(GLOB_RECURSE newton_sources CONFIGURE_DEPENDS vendor/newton/*.cpp)
add_library(newton STATIC ${newton_sources})
target_include_directories(newton PUBLIC vendor/newton)
target_compile_options(newton PRIVATE -w)

# AngelScript 2.38.0 (zlib)
file(GLOB angelscript_sources CONFIGURE_DEPENDS
	vendor/angelscript/source/*.cpp
	vendor/angelscript/add_on/scriptstdstring/*.cpp)
add_library(angelscript STATIC ${angelscript_sources})
target_include_directories(angelscript PUBLIC
	vendor/angelscript/include
	vendor/angelscript/add_on)
target_compile_options(angelscript PRIVATE -w)

# OALWrapper (zlib), Frictional Games' OpenAL helper
file(GLOB oalwrapper_sources CONFIGURE_DEPENDS vendor/oalwrapper/sources/*.cpp)
add_library(oalwrapper STATIC ${oalwrapper_sources})
target_include_directories(oalwrapper PUBLIC vendor/oalwrapper/include)
target_link_libraries(oalwrapper PUBLIC OpenAL::OpenAL PkgConfig::VORBIS SDL3::SDL3)

#--------------------------------------------------------------------
# HPL1 engine

file(GLOB_RECURSE hpl_sources CONFIGURE_DEPENDS
	engine/sources/*.cpp
	engine/sources/*.c)
list(FILTER hpl_sources EXCLUDE REGEX "/(win32\\.cpp|Video[A-Za-z]*Theora\\.cpp)$")

add_library(hpl STATIC ${hpl_sources})
target_include_directories(hpl PUBLIC engine/include)
target_link_libraries(hpl PUBLIC
	newton angelscript oalwrapper
	SDL3::SDL3 SDL3_image::SDL3_image
	OpenGL::GL)

#--------------------------------------------------------------------
# Penumbra: Overture

file(GLOB game_sources CONFIGURE_DEPENDS game/*.cpp)
add_executable(overture ${game_sources})
target_link_libraries(overture PRIVATE hpl)
