# For use as a visual studio code cmake plugin "kit" for LLVM+MinGW builds.
set(CMAKE_C_FLAGS "-target x86_64-pc-windows-gnu" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "-target x86_64-pc-windows-gnu" CACHE STRING "" FORCE)
set(CMAKE_C_COMPILER "clang.exe" CACHE STRING "" FORCE)
set(CMAKE_CXX_COMPILER "clang++.exe" CACHE STRING "" FORCE)
