# ClothSimulation

**语言 / Language：** [![简体中文](https://img.shields.io/badge/简体中文-当前语言-238636?style=for-the-badge)](./README.md) [![English](https://img.shields.io/badge/English-Switch-0969da?style=for-the-badge)](./README_EN.md)


基于 C++、OpenGL 和 CMake 的布料模拟项目。仓库使用统一的 CMake 构建配置，目标开发环境为：

- Windows 10/11：Visual Studio 2022 或 VS Code（MSVC）
- macOS：VS Code（Apple Clang + CodeLLDB）

> 本文记录项目在 Windows 和 macOS 上已验证过的开发流程，便于以后在新电脑上重新搭建环境。实际工具版本、可执行文件输出路径请以仓库中的 `CMakePresets.json` 和 `CMakeLists.txt` 为准。

## 环境要求

### 通用

- Git
- CMake **3.21 或更高版本**（项目使用 CMake Presets）
- 可正常使用 OpenGL 的图形环境与驱动

通过以下命令检查：

```bash
git --version
cmake --version
```

项目使用到的 GLFW、Assimp、GLAD、GLM、ImGui 等依赖源码放在 `external/` 中；正常情况下，不必分别为它们安装系统级开发包。

### Windows

- **Visual Studio 2022**，安装 **Desktop development with C++（使用 C++ 的桌面开发）** 工作负载，包含 MSVC x64 工具链和 Windows SDK。
- 如果使用 **VS Code**，安装 Microsoft 的 **C/C++**（`ms-vscode.cpptools`）与 **CMake Tools**（`ms-vscode.cmake-tools`）扩展。
- Windows 的 CMake Presets 使用 Visual Studio 2022 生成器，**不需要切换到 MinGW**。

### MacOS

安装 Xcode Command Line Tools（包含 Apple Clang 和 Make）：

```bash
xcode-select --install
```

安装 CMake。如果已安装 Homebrew，可使用：

```bash
brew install cmake
```

安装 VS Code，并启用以下扩展：

- **C/C++**：`ms-vscode.cpptools`
- **CMake Tools**：`ms-vscode.cmake-tools`
- **CodeLLDB**：`vadimcn.vscode-lldb`（用于断点调试）

## Windows：Visual Studio 2022

1. 启动 Visual Studio 2022。
2. 选择 **File → Open → Folder**，打开仓库根目录（包含 `CMakeLists.txt` 和 `CMakePresets.json` 的文件夹），不要打开 `src/` 或 `build/`。
3. 等待 Visual Studio 自动完成 CMake Configure / Generate，确认日志中出现 `Configuring done`、`Generating done`。
4. 选择对应的 CMake 配置 **`windows-vs2022`**，并使用 **Debug** 构建配置。
5. 在启动目标菜单中选择 **`ClothSimulation` / `ClothSimulation.exe`**，不要选择 `glfw`、`assimp` 等依赖目标。
6. 执行 **Build → Build All** 编译；按 **F5** 调试，或 **Ctrl+F5** 运行。

也可以在 Windows CMD 中从仓库根目录执行：

```cmd
cmake --preset windows-vs2022
cmake --build --preset windows-vs2022-debug --parallel 4
```

在当前构建输出设置下，Debug 程序通常位于：

```text
build\windows-vs2022\bin\Debug\ClothSimulation.exe
```



## Windows：VS Code

1. 使用 **File → Open Folder** 打开整个仓库根目录。
2. 确保已安装 **C/C++** 与 **CMake Tools** 扩展。
3. 按 **Ctrl+Shift+P**：
   - `CMake: Select Configure Preset` → **`windows-vscode`**
   - `CMake: Configure`
   - `CMake: Build`
   - `CMake: Set Launch/Debug Target` → **`ClothSimulation`**
4. 在 **Run and Debug** 面板中选中仓库提供的 Windows 调试配置（类型为 `cppvsdbg`），按 **F5** 调试；也可以通过 CMake Tools 启动程序。

命令行等价操作（Windows CMD）：

```cmd
cmake --preset windows-vscode
cmake --build --preset windows-vscode-debug --parallel 4
```

在当前构建输出设置下，Debug 程序通常位于：

```text
build\windows-vscode\bin\Debug\ClothSimulation.exe
```

VS Code 的 Windows 构建同样使用 **Visual Studio 2022 / MSVC**，所以需要先安装 Visual Studio C++ 工具链。



## MacOS：VS Code

1. 使用 VS Code 打开仓库根目录。
2. 确保安装了 **C/C++、CMake Tools、CodeLLDB** 扩展。
3. 按 **Cmd+Shift+P**：
   - `CMake: Select Configure Preset` → **`macos-vscode`**
   - `CMake: Configure`
   - `CMake: Build`
   - `CMake: Set Launch/Debug Target` → **`ClothSimulation`**
4. 在 **Run and Debug** 面板（**Cmd+Shift+D**）中选择 macOS / CodeLLDB 调试配置，按 **F5**（部分键盘需 **Fn+F5**）。
5. 在 `src/main.cpp` 的 `main()` 函数内设置断点，确认能够暂停。

命令行等价操作（在仓库根目录）：

```bash
cmake --preset macos-vscode
cmake --build --preset macos-vscode-debug --parallel 4
```

可执行文件的具体位置取决于 `CMakeLists.txt` 的输出目录设置。如果需要定位：

```bash
find build/macos-vscode -type f -name ClothSimulation
```

在仓库根目录运行找到的程序路径，以确保相对资源路径从项目根目录开始解析。例如，如果输出位于 `build/macos-vscode/bin/`：

```bash
./build/macos-vscode/bin/ClothSimulation
```

### MacOS 调试器注意事项

macOS 应使用 **CodeLLDB** 扩展对应的调试配置：

```json
{
  "name": "macOS - ClothSimulation",
  "type": "lldb",
  "request": "launch",
  "program": "${command:cmake.launchTargetPath}",
  "cwd": "${workspaceFolder}"
}
```

不要用 `cppdbg` + `MIMode: "lldb"` 指向 `/usr/bin/lldb`。这可能触发 `unknown option: --interpreter=mi` 错误。应从 VS Code 的 **Run and Debug** 面板明确选择 CodeLLDB 配置，而不是误用其他默认调试配置。



## 项目目录

```text
ClothSimulation/
├── .vscode/           # 共享的 VS Code 配置（如果已提交）
├── include/           # 项目头文件
├── src/               # C++ 源代码
├── Shaders/           # GLSL Shader 文件
├── external/          # 项目内集成的第三方依赖
├── Models/            # 本地模型资源；不纳入 Git
├── build/             # CMake 构建结果；不纳入 Git
├── CMakeLists.txt
├── CMakePresets.json
└── .gitignore
```

**不要把 `build/`、`.vs/`、`Models/` 等本地文件提交到 GitHub。** 通用的 `CMakePresets.json` 与经过验证的 `.vscode/launch.json`、`.vscode/settings.json`、`.vscode/extensions.json` 可以提交；个人专用的 `CMakeUserPresets.json` 应保留在本地。



## 常见问题

**CMake 配置成功但程序不能启动**：首先查看是否缺少 `Models/` 中的模型文件，以及 OBJ 对应的材质、贴图。

**Shader 加载失败或窗口黑屏**：确认运行时工作目录为仓库根目录，因为项目使用 `Shaders/` 等相对资源路径。

**macOS 调试出现 `--interpreter=mi`**：安装并启用 CodeLLDB；使用 `launch.json` 中 `"type": "lldb"` 的配置，从 Run and Debug 面板启动。

**VS Code 无法识别头文件**：确认 CMake 配置成功，已选中正确的 Configure Preset；可在 `.vscode/settings.json` 中使用 `"C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools"`。

**CMake Preset 不显示**：确认打开的是项目根目录、CMake 版本符合要求，并运行 `cmake --list-presets` 检查当前操作系统可用的配置。

**切换 IDE 后 CMake 缓存冲突**：不要让 Visual Studio 和 VS Code 共用同一个 `build/` 子目录。已有 Presets 为不同环境分配了独立构建目录。

