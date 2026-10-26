Inflate (tinfl) from miniz 3.1.2 (https://github.com/richgel999/miniz), MIT licence (LICENSE).
Used to read the zlib tiles of navmap.bin. Changes: miniz_tinfl.c renamed to .cpp (the build only globs .cpp) and
includes miniz_tinfl.h instead of miniz.h; miniz_export.h is a stub for the CMake-generated header.
