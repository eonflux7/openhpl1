#!/usr/bin/env python3
"""Check the game's actual image compositor with Python/Pillow, without SDL.

Run: python3 tests/check_comparison_image.py
Requires a C++17 compiler and Pillow, but no game data or development libraries.
"""
import io
from pathlib import Path
import subprocess
import tempfile
import unittest

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
HARNESS = r'''
#include "impl/ComparisonImage.h"
#include <cstdlib>
#include <iostream>
int main(int argc, char **argv)
{
    const int w = std::atoi(argv[1]), h = std::atoi(argv[2]);
    const size_t count = w > 0 && h > 0 ? (size_t)w*h*3 : 0;
    std::vector<unsigned char> left(count), right(count);
    for(int y=0; y<h; ++y)
        for(int x=0; x<w; ++x)
        {
            const size_t i = ((size_t)y*w+x)*3;
            left[i] = x%256; left[i+1] = y%256; left[i+2] = 31;
            right[i] = 219; right[i+1] = y%256; right[i+2] = x%256;
        }
    if(argc > 3) right.push_back(0);
    hpl::cComparisonImage image;
    const bool ok = hpl::ComposeComparisonImage(image, hpl::cVector2l(w,h), left, right);
    if(argc > 3 || w < 52 || h <= 0) return ok ? 2 : 0;
    if(!ok) return 3;
    std::cout << "P6\n" << image.mvSize.x << " " << image.mvSize.y << "\n255\n";
    std::cout.write((const char*)image.mvPixels.data(), image.mvPixels.size());
}
'''


class ComparisonImageTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix="overture-comparison-test-")
        cls.addClassCleanup(cls.temp.cleanup)
        directory = Path(cls.temp.name)
        harness = directory / "comparison_test.cpp"
        harness.write_text(HARNESS)
        cls.executable = directory / "comparison_test"
        subprocess.run([
            "c++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
            "-I", str(ROOT / "engine/include"), str(harness),
            str(ROOT / "engine/sources/impl/ComparisonImage.cpp"),
            "-o", str(cls.executable),
        ], check=True)

    def render(self, width, height, *extra):
        return subprocess.check_output([
            str(self.executable), str(width), str(height), *extra,
        ])

    @staticmethod
    def frame_pixels(width, height, improved, start=0, end=None):
        return bytes(
            component
            for y in range(height)
            for x in range(start, width if end is None else end)
            for component in ((219, y%256, x%256) if improved
                              else (x%256, y%256, 31))
        )

    def check_header(self, image):
        colors = dict((color, count) for count, color in image.getcolors())
        self.assertEqual(set(colors), {(240, 242, 246), (22, 25, 31)})
        self.assertGreater(colors[(240, 242, 246)], 0)
        self.assertGreater(colors[(22, 25, 31)], 0)

    def test_vertical_full_frames_and_orientation(self):
        # Odd widths catch row alignment bugs; tall images exercise the
        # label scaling used at 720p and high display resolutions.
        for width, height in [(320, 180), (321, 181), (128, 720), (256, 1440)]:
            with self.subTest(size=(width, height)):
                image = Image.open(io.BytesIO(self.render(width, height)))
                header = 15 * max(1, min(4, height // 360, width // 52))
                self.assertEqual(image.size, (width, 2*(height+header)))
                for index, improved in enumerate([False, True]):
                    top = index*(height+header)
                    self.check_header(image.crop((0, top, width, top+header)))
                    self.assertEqual(
                        image.crop((0, top+header, width, top+header+height)).tobytes(),
                        self.frame_pixels(width, height, improved),
                    )

    def test_invalid_and_mismatched_frames(self):
        for width, height in [(0, 180), (-1, 180), (51, 180), (320, 0), (320, -1)]:
            with self.subTest(size=(width, height)):
                self.assertEqual(self.render(width, height), b"")
        self.assertEqual(self.render(320, 180, "mismatch"), b"")


if __name__ == "__main__":
    unittest.main()
