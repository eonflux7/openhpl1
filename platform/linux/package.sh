#!/bin/sh
# Builds a portable Linux x86_64 release of OpenHPL1:
#   openhpl1-<version>-linux-x86_64.tar.gz
# SDL3, SDL3_image and OpenAL Soft are built from pinned sources and shipped
# in lib/ beside the executable (found through RPATH $ORIGIN/lib); Ogg, Vorbis
# and FreeType are linked statically, and so are libstdc++ and libgcc. The
# result needs only the system's C library, OpenGL driver and (loaded at run
# time by SDL and OpenAL Soft) X11/Wayland and PipeWire/PulseAudio/ALSA.
# The release workflow runs this in the Steam Runtime 3 "sniper" SDK
# (glibc 2.31), so the binary runs on that and anything newer.
# Usage: platform/linux/package.sh <version> [work dir] [output dir]
set -eu

VERSION="${1:?usage: package.sh <version> [work dir] [output dir]}"
SRC="$(cd "$(dirname "$0")/../.." && pwd)"
WORK="${2:-$SRC/build-release}"
OUT="${3:-$WORK}"
JOBS="$(nproc)"

SDL_VERSION=3.4.16
SDL_IMAGE_VERSION=3.4.6
OPENAL_VERSION=1.25.2
OGG_VERSION=1.3.6
VORBIS_VERSION=1.3.7

mkdir -p "$WORK/src" "$OUT"
PREFIX="$WORK/prefix"
DIST_NAME="openhpl1-$VERSION-linux-x86_64"
DIST="$WORK/$DIST_NAME"

# The newest GCC available; OpenAL Soft needs C++20
if [ -z "${CC:-}" ]; then
	for v in 14 13 12; do
		if command -v "g++-$v" >/dev/null 2>&1; then
			export CC="gcc-$v" CXX="g++-$v"
			break
		fi
	done
fi
export CC="${CC:-gcc}" CXX="${CXX:-g++}"
echo "Compilers: $CC, $CXX"

fetch()
{
	# fetch <url> <directory name>
	if [ ! -d "$WORK/src/$2" ]; then
		curl -fL --retry 3 -o "$WORK/src/$2.tar.gz" "$1"
		mkdir -p "$WORK/src/$2"
		tar -xzf "$WORK/src/$2.tar.gz" -C "$WORK/src/$2" --strip-components=1
	fi
}

build_dep()
{
	# build_dep <directory name> <cmake options...>
	name="$1"
	shift
	cmake -S "$WORK/src/$name" -B "$WORK/deps/$name" -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX="$PREFIX" \
		-DCMAKE_INSTALL_LIBDIR=lib \
		-DCMAKE_PREFIX_PATH="$PREFIX" \
		-DCMAKE_POSITION_INDEPENDENT_CODE=ON \
		"$@"
	cmake --build "$WORK/deps/$name" -j "$JOBS"
	cmake --install "$WORK/deps/$name"
}

fetch "https://github.com/libsdl-org/SDL/releases/download/release-$SDL_VERSION/SDL3-$SDL_VERSION.tar.gz" sdl
fetch "https://github.com/libsdl-org/SDL_image/releases/download/release-$SDL_IMAGE_VERSION/SDL3_image-$SDL_IMAGE_VERSION.tar.gz" sdl_image
fetch "https://github.com/kcat/openal-soft/archive/refs/tags/$OPENAL_VERSION.tar.gz" openal
fetch "https://github.com/xiph/ogg/releases/download/v$OGG_VERSION/libogg-$OGG_VERSION.tar.gz" ogg
fetch "https://github.com/xiph/vorbis/releases/download/v$VORBIS_VERSION/libvorbis-$VORBIS_VERSION.tar.gz" vorbis

build_dep sdl -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF
# PNG and JPEG through the bundled stb loaders: no libpng or libjpeg needed
build_dep sdl_image -DBUILD_SHARED_LIBS=ON -DSDLIMAGE_BACKEND_STB=ON -DSDLIMAGE_VENDORED=OFF \
	-DSDLIMAGE_AVIF=OFF -DSDLIMAGE_JXL=OFF -DSDLIMAGE_TIF=OFF -DSDLIMAGE_WEBP=OFF \
	-DSDLIMAGE_SAMPLES=OFF -DSDLIMAGE_TESTS=OFF
build_dep openal -DLIBTYPE=SHARED -DALSOFT_UTILS=OFF -DALSOFT_EXAMPLES=OFF -DALSOFT_TESTS=OFF \
	-DALSOFT_INSTALL_EXAMPLES=OFF -DALSOFT_INSTALL_UTILS=OFF \
	-DCMAKE_SHARED_LINKER_FLAGS="-static-libstdc++ -static-libgcc"
build_dep ogg -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF -DINSTALL_DOCS=OFF
build_dep vorbis -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF -DCMAKE_POLICY_VERSION_MINIMUM=3.5

# FreeType comes from the engine's static fallback rather than the system
PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig" cmake -S "$SRC" -B "$WORK/openhpl1" -G Ninja \
	-DCMAKE_BUILD_TYPE=RelWithDebInfo \
	-DCMAKE_PREFIX_PATH="$PREFIX" \
	-DCMAKE_DISABLE_FIND_PACKAGE_Freetype=ON \
	-DBUILD_TESTING=OFF \
	-DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
	-DCMAKE_INSTALL_RPATH='$ORIGIN/lib' \
	-DCMAKE_EXE_LINKER_FLAGS="-static-libstdc++ -static-libgcc"
cmake --build "$WORK/openhpl1" -j "$JOBS"

rm -rf "$DIST"
mkdir -p "$DIST/lib" "$DIST/licenses"
cp "$WORK/openhpl1/openhpl1" "$DIST/"
strip "$DIST/openhpl1"
cp -r "$WORK/openhpl1/shaders" "$WORK/openhpl1/fonts" "$WORK/openhpl1/hud" "$DIST/"
for lib in libSDL3.so.0 libSDL3_image.so.0 libopenal.so.1; do
	cp -L "$PREFIX/lib/$lib" "$DIST/lib/"
	strip --strip-unneeded "$DIST/lib/$lib"
done
cp "$SRC/COPYING" "$SRC/README.md" "$SRC/CHANGES.md" "$DIST/"
cp "$WORK/src/sdl/LICENSE.txt" "$DIST/licenses/SDL3.txt"
cp "$WORK/src/sdl_image/LICENSE.txt" "$DIST/licenses/SDL3_image.txt"
cp "$WORK/src/openal/COPYING" "$DIST/licenses/openal-soft.txt"
cp "$WORK/src/ogg/COPYING" "$DIST/licenses/libogg.txt"
cp "$WORK/src/vorbis/COPYING" "$DIST/licenses/libvorbis.txt"
cp "$WORK/openhpl1/_deps/freetype-src/LICENSE.TXT" "$DIST/licenses/freetype.txt"
cp "$WORK/openhpl1/_deps/freetype-src/docs/FTL.TXT" "$DIST/licenses/freetype-FTL.txt"
for dep in angelscript glad imgui imgui_glad newton oalwrapper smaa tinyxml; do
	for f in "$SRC/vendor/$dep/LICENSE" "$SRC/vendor/$dep/LICENSE.txt"; do
		if [ -f "$f" ]; then cp "$f" "$DIST/licenses/$dep.txt"; fi
	done
done
cat > "$DIST/licenses/README.txt" <<EOF
OpenHPL1 is GPLv3 (../COPYING). The fonts' licences are in ../fonts/ and the
HUD images' terms in ../hud/README.md.

Bundled libraries and their sources:
  SDL3 $SDL_VERSION (zlib)            https://github.com/libsdl-org/SDL/releases/tag/release-$SDL_VERSION
  SDL3_image $SDL_IMAGE_VERSION (zlib)      https://github.com/libsdl-org/SDL_image/releases/tag/release-$SDL_IMAGE_VERSION
  OpenAL Soft $OPENAL_VERSION (LGPL-2.1)  https://github.com/kcat/openal-soft/releases/tag/$OPENAL_VERSION
  libogg $OGG_VERSION, libvorbis $VORBIS_VERSION (BSD)  https://github.com/xiph
  FreeType 2.13.3 (FTL)            https://download.savannah.gnu.org/releases/freetype/
lib/libopenal.so.1 can be replaced by any compatible OpenAL Soft build.
EOF

tar -czf "$OUT/$DIST_NAME.tar.gz" -C "$WORK" "$DIST_NAME"
echo "Packaged $OUT/$DIST_NAME.tar.gz"
