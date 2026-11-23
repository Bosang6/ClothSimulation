# ClothSimulation

**Language / 语言:** [![简体中文](https://img.shields.io/badge/简体中文-切换-0969da?style=for-the-badge)](./README.md) [![English](https://img.shields.io/badge/English-Current-238636?style=for-the-badge)](./README_EN.md)

A cross-platform cloth simulation project built with C++, OpenGL, and CMake. The repository uses a shared CMake build configuration and targets the following development environments:

- Windows 10/11: Visual Studio 2022 or VS Code (MSVC)
- macOS: VS Code (Apple Clang + CodeLLDB)

> This guide documents the development workflows verified on Windows and macOS, so you can set up the project again on another computer. For the authoritative tool requirements and executable output paths, check `CMakePresets.json` and `CMakeLists.txt` in the repository.

## Requirements

### General

- Git
- **CMake 3.21 or newer** (the project uses CMake Presets)
- A graphics environment and drivers capable of running OpenGL

Check your installations:

```bash
git --version
cmake --version
```

The source code for GLFW, Assimp, GLAD, GLM, ImGui, and other dependencies is included under `external/`. You normally do not need to install their development packages separately.

### Windows

- Install **Visual Studio 2022** with the **Desktop development with C++** workload, including the MSVC x64 toolchain and Windows SDK.
- For **VS Code**, install the Microsoft **C/C++** (`ms-vscode.cpptools`) and **CMake Tools** (`ms-vscode.cmake-tools`) extensions.
- The Windows CMake Presets use the Visual Studio 2022 generator; **you do not need MinGW**.

### MacOS

Install Xcode Command Line Tools (including Apple Clang and Make):

```bash
xcode-select --install
```

Install CMake. If you use Homebrew:

```bash
brew install cmake
```

Install VS Code with these extensions:

- **C/C++**: `ms-vscode.cpptools`
- **CMake Tools**: `ms-vscode.cmake-tools`
- **CodeLLDB**: `vadimcn.vscode-lldb` (for breakpoint debugging)



## Windows: Visual Studio 2022

1. Open Visual Studio 2022.
2. Select **File → Open → Folder** and open the repository root (the folder containing `CMakeLists.txt` and `CMakePresets.json`), not `src/` or `build/`.
3. Let Visual Studio run CMake Configure / Generate automatically; confirm that the output contains `Configuring done` and `Generating done`.
4. Select the **`windows-vs2022`** CMake configure preset and the **Debug** build configuration.
5. Set **`ClothSimulation` / `ClothSimulation.exe`** as the startup target, rather than a dependency target such as `glfw` or `assimp`.
6. Select **Build → Build All**. Press **F5** to debug or **Ctrl+F5** to run without debugging.

Alternatively, build from the repository root in Windows Command Prompt:

```cmd
cmake --preset windows-vs2022
cmake --build --preset windows-vs2022-debug --parallel 4
```

With the current output settings, the Debug executable is typically located at:

```text
build\windows-vs2022\bin\Debug\ClothSimulation.exe
```



## Windows: VS Code

1. Use **File → Open Folder** to open the entire repository root.
2. Install the **C/C++** and **CMake Tools** extensions.
3. Press **Ctrl+Shift+P** and run these commands:
   - `CMake: Select Configure Preset` → **`windows-vscode`**
   - `CMake: Configure`
   - `CMake: Build`
   - `CMake: Set Launch/Debug Target` → **`ClothSimulation`**
4. In **Run and Debug**, select the shared Windows launch configuration (type `cppvsdbg`) and press **F5**; alternatively, start the application using CMake Tools.

Equivalent commands in Windows Command Prompt:

```cmd
cmake --preset windows-vscode
cmake --build --preset windows-vscode-debug --parallel 4
```

With the current output settings, the Debug executable is typically located at:

```text
build\windows-vscode\bin\Debug\ClothSimulation.exe
```

The Windows VS Code build also uses **Visual Studio 2022 / MSVC**, so the Visual Studio C++ toolchain must be installed.



## MacOS: VS Code

1. Open the repository root in VS Code.
2. Install the **C/C++**, **CMake Tools**, and **CodeLLDB** extensions.
3. Press **Cmd+Shift+P** and run these commands:
   - `CMake: Select Configure Preset` → **`macos-vscode`**
   - `CMake: Configure`
   - `CMake: Build`
   - `CMake: Set Launch/Debug Target` → **`ClothSimulation`**
4. In **Run and Debug** (**Cmd+Shift+D**), select the macOS / CodeLLDB launch configuration and press **F5** (or **Fn+F5** on some keyboards).
5. Set a breakpoint inside `main()` in `src/main.cpp` and verify that the debugger stops there.

Equivalent commands from the repository root:

```bash
cmake --preset macos-vscode
cmake --build --preset macos-vscode-debug --parallel 4
```

The executable path depends on the output directory set in `CMakeLists.txt`. To locate it:

```bash
find build/macos-vscode -type f -name ClothSimulation
```

Launch the resulting executable **from the repository root**, so relative asset paths are resolved from that directory. For example, if the binary is in `build/macos-vscode/bin/`:

```bash
./build/macos-vscode/bin/ClothSimulation
```

### MacOS Debugger Notes

Use the **CodeLLDB** debug configuration on macOS:

```json
{
  "name": "macOS - ClothSimulation",
  "type": "lldb",
  "request": "launch",
  "program": "${command:cmake.launchTargetPath}",
  "cwd": "${workspaceFolder}"
}
```

Do not point `cppdbg` with `MIMode: "lldb"` to `/usr/bin/lldb`. That combination may produce `unknown option: --interpreter=mi`. Explicitly select the CodeLLDB launch configuration in VS Code's **Run and Debug** view instead of an incompatible default debug configuration.



## Project Layout

```text
ClothSimulation/
├── .vscode/           # Shared VS Code settings (if committed)
├── include/           # Project headers
├── src/               # C++ source code
├── Shaders/           # GLSL shader files
├── external/          # Vendored third-party dependencies
├── Models/            # Local model assets; excluded from Git
├── build/             # CMake build output; excluded from Git
├── CMakeLists.txt
├── CMakePresets.json
└── .gitignore
```

**Do not commit `build/`, `.vs/`, or `Models/` to GitHub.** Shared files such as `CMakePresets.json` and tested `.vscode/launch.json`, `.vscode/settings.json`, and `.vscode/extensions.json` may be committed. Keep machine-specific `CMakeUserPresets.json` local.



## Troubleshooting

**CMake configures successfully, but the application cannot start:** First check whether model files are missing from `Models/`, including any materials and textures referenced by the OBJ.

**Shaders fail to load, or the window is black:** Run the application with the repository root as its working directory; the project uses relative paths such as `Shaders/`.

**macOS debugging fails with `--interpreter=mi`:** Install and enable CodeLLDB, use a `launch.json` configuration with `"type": "lldb"`, and start it from **Run and Debug**.

**VS Code cannot resolve project headers:** Make sure CMake configuration completed and the appropriate Configure Preset is selected. You can configure IntelliSense using `"C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools"` in `.vscode/settings.json`.

**A CMake Preset is missing:** Open the repository root, check the CMake version, and run `cmake --list-presets` to see the presets available for the current operating system.

**CMake cache conflicts after switching IDEs:** Do not share one `build/` subdirectory between Visual Studio and VS Code. The presets assign distinct build directories to each environment.
