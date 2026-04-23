# CommonLibF4

### Requirements
* C++23 Compiler (MSVC or Clang-CL)
* One of:
  * [CMake](https://cmake.org) 3.24+
  * [XMake](https://xmake.io) 3.0.0+

## Getting Started
```bat
git clone --recurse-submodules https://github.com/libxse/commonlibf4
cd commonlibf4
```

### Build With CMake
To configure and build the project with CMake, run:
```bat
cmake -S . -B build/cmake -G "Visual Studio 17 2022" ^
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
cmake --build build/cmake --config RelWithDebInfo
```

If your IDE supports CMake presets, you can use the included `vs2022-vcpkg` preset instead.

Before configuring, install manifest dependencies with vcpkg:
```bat
vcpkg install --x-manifest-root=.
```

The repository now manages `spdlog` through `vcpkg.json`.
The manifest disables spdlog's default `fmt` feature so the package is built with `std::format`, and enables `wchar` support to match the previous xmake configuration.
Optional dependencies are also mapped into manifest features:
```bat
vcpkg install --x-manifest-root=. --x-feature=ini --x-feature=json --x-feature=toml --x-feature=xbyak
```

Optional features map to the previous xmake switches:
```bat
cmake -S . -B build/cmake ^
  -DCOMMONLIBF4_ENABLE_INI=ON ^
  -DCOMMONLIBF4_ENABLE_JSON=ON ^
  -DCOMMONLIBF4_ENABLE_TOML=ON ^
  -DCOMMONLIBF4_ENABLE_XBYAK=ON
```

To consume the library from another CMake project:
```cmake
add_subdirectory(extern/commonlibfo4)
target_link_libraries(your_target PRIVATE CommonLibF4::commonlibf4)
```

If you need the plugin metadata/resource helpers in your own plugin target:
```cmake
include(extern/commonlibfo4/cmake/CommonLibF4Plugin.cmake)

add_library(MyPlugin SHARED src/plugin.cpp)
commonlibf4_configure_plugin(MyPlugin
    AUTHOR "Your Name"
    DESCRIPTION "Your plugin description"
    VERSION "1.0.0")
```

### Build With XMake
To build the project with xmake, run the following command:
```bat
xmake build
```

> ***Note:*** *This will generate a `build/windows/` directory in the **project's root directory** with the build output.*

### Project Generation (Optional)
If you use Visual Studio, run the following command:
```bat
xmake project -k vsxmake
```

> ***Note:*** *This will generate a `vsxmakeXXXX/` directory in the **project's root directory** using the latest version of Visual Studio installed on the system.*

**Alternatively**, if you do not use Visual Studio, you can generate a `compile_commands.json` file for use with a laguage server like clangd in any code editor that supports it, like vscode:
```bat
xmake project -k compile_commands
```

> ***Note:*** *You must have a language server extension installed to make use of this file. I recommend `clangd`. Do not have more than one installed at a time as they will conflict with each other. I also recommend installing the `xmake` extension if available to make building the project easier.*

## Notes

CommonLibF4 is intended to replace F4SE as a static dependency. However, the runtime component of F4SE is still required.
