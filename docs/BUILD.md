A "Markdown File Viewer" is recommended for reading this .MD file.
On Windows find and download one from the Store.

# Build instructions

## Windows Visual Studio MSVC

### 1. Tools Setup

* **Visual Studio**. Install separately 2022 (v143 19.38 and 19.44) or 2026 (v145).

* **Winget:**. Required for downloading tools. Check with `winget --version`.

  Type `winget --version` in your terminal. If it is not recognized (and not because of path problems), install it using one of these methods:
  - **Browser:** [App Installer (winget) on Microsoft Store](https://apps.microsoft.com/store/detail/app-installer/9NBLGGH4NNS1)
  - **PowerShell (Admin):** Run the following to force-update if the store is unavailable:
    ```powershell
    $url = "https://github.com/microsoft/winget-cli/releases/latest/download/Microsoft.DesktopAppInstaller_8wekyb3d8bbwe.msixbundle"
    Invoke-WebRequest -Uri $url -OutFile "$env:TEMP\winget.msixbundle"
    Add-AppxPackage "$env:TEMP\winget.msixbundle"
    ```
    
* **Prerequisite**: install nasm, ninja, pkg-config-lite and meson
  - `winget install Ninja-build.Ninja NASM.NASM bloodrock.pkg-config-lite mesonbuild.meson`
    
    If 'winget' is still not in your PATH, use the direct local path to install tools:
    
    **In PowerShell (Admin):** Run the following:
    ```
    "$env:LOCALAPPDATA\Microsoft\WindowsApps\winget.exe" install Ninja-build.Ninja NASM.NASM bloodrock.pkg-config-lite mesonbuild.meson --accept-source-agreements
    ```

### 2. Build process

**Note**: Close and reopen your terminal after installing tools. If you have MSYS2 installed, our scripts will isolate the PATH to avoid header conflicts.

**Note** After Get sources and AviSynth libs, you can use the helper `build_msvc.cmd` which contains all the steps below.
Adjust the paths of the isolated environment if needed. 
Use 

- `build_msvc.cmd VS2026v143` for a v143 toolset (VS2022-style) build on a Visual Studio 2026.
- `build_msvc.cmd VS2026` for a v145 toolset build with Visual Studio 2026.
- `build_msvc.cmd VS2022` for a v143 toolset build with Visual Studio 2022 Community.

It will create both the 32 and 64 bit assrender.dll files and the Visual Studio solution files as well.
If it does not work, go on with our detailed description.

#### 2.1 Get sources and AviSynth libs

* Clone repos
  - `git clone https://github.com/pinterf/assrender.git`
  - `cd assrender && git clone https://github.com/libass/libass.git`
* Copy Avisynth libs - avisynth.lib versions (x86 and x64):
  - Copy lib files to 
    - assrender\lib\x86-64\ 
    - assrender\lib\x86-32\
    
    32 and 64 bit versions respectively.

  When you have installed Avisynth through an installer and have installed FilterSDK  
  get them from `c:\Program Files (x86)\AviSynth+\FilterSDK\lib\x86` and `x64`.
 
  Or get them from the 'filesonly' builds at Avisynth+ releases https://github.com/AviSynth/AviSynthPlus/releases

#### 2.2 Build libass (assrender dependency)

We use separate prefixes (`C:/lib64` and `C:/lib32`) to prevent architecture mixing.

**IMPORTANT** Do not set a global PKG_CONFIG_PATH. We will set this manually for each build 
to ensure we don't mix 32-bit and 64-bit libraries.

**Step 1: Isolate environment**

In a developer machine, there can be multiple toolsets. MSYS2/MinGW64, Visual Studio 2022 Professional and Community, VS 2026,
Intel C++ Compiler 2023-2025, LLVM, Clang-cl, MSVC with v141, v143 (multiple version), v145 toolsets, Arm64 cross compiling tools, 
multiple CMake versions, etc...
These all co-exist and mess up each other's paths and environment variables, and the wrong tool is found.

Open a standard CMD and run (Isolate PATH to avoid MSYS2/MinGW conflicts):
```cmd
set PATH=C:\Windows\System32;C:\Windows\System32\WindowsPowerShell\v1.0;C:\Windows;C:\Program Files\Meson;%LocalAppData%\Microsoft\WinGet\Links;C:\Program Files\WinGet\Links
```

And these (adjust path for your version) (default: 64-bit)

- For Visual Studio 2022 non-Community Edition:

    `call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"`

- For Visual Studio 2022 Community Edition: 

    `call "c:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"`

- For Visual Studio 2026 Community Edition: 

    `call "c:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"`

- For 32-bit build + Visual Studio 2022 Community Edition: 

    `call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"`

**Optional: check environment, verify tools**

  Issue commands:
  
  ```
  where cl
  where meson
  where pkg-config
  ```
  
  **IMPORTANT** Before proceeding, ensure that all utilities are accessible:
    
  - cl: Must point to Microsoft Visual Studio's compiler.
  - meson: E.g. `C:\Program Files\Meson`.
  - pkg-config: E.g. `...\WinGet\Links\pkg-config.exe`, if you have downloaded it through app store.

**Step 2: Actual build libass (Static):**

Note 1: We use `--wrap-mode=forcefallback` to ensure all dependencies (Freetype, Fribidi, etc.) are built 
with the MSVC compiler rather than using incompatible system libraries. (found in mingw64 for example, and
not downloaded from source).
  
Note 2: We use separate `C:/lib64` and `C:/lib32` prefixes for library and package targets for 64 and 32-bit versions.

Note 3: By setting CC and CXX environment variables manually, we force MSVC.
Meson defaults to the CL tool whatever it has in the path or C/CXX env variable.
When you want non-standard choice (VS 2026 with v143 toolset with specific version), you'll have to experiment with it on your own.

- In `libass` folder run (Assuming that we are still in `assrender` folder; changing into `libass` is in the script!)

**Step 2a: Build for 64-bit (x64)**

```
cd libass
set CC=cl
set CXX=cl
meson wrap update-db
meson setup build/x64 --wrap-mode=forcefallback -Ddefault_library=static -Dbuildtype=release -Dasm=enabled -Db_vscrt=static_from_buildtype -Dc_std=c11 -Dcpp_std=c++17 --prefix=C:/lib64
meson compile -C build/x64
meson install -C build/x64
```

**Step 2b: Build for 32-bit (x86)**

```
cd libass
set CC=cl
set CXX=cl
meson setup build/Win32 --wrap-mode=forcefallback -Ddefault_library=static -Dbuildtype=release -Dasm=enabled -Db_vscrt=static_from_buildtype  -Dc_std=c11 -Dcpp_std=c++17 --prefix=C:/lib32
meson compile -C build/Win32
meson install -C build/Win32
```

#### 2.3 Build assrender (the plugin)

  We set the 32-bit/64-bit `PKG_CONFIG_PATH` here, before running CMake, we must tell it where the newly installed libass configuration is located.

- go back to `assrender` folder and run

For 64-bit (x64):

```
set PKG_CONFIG_PATH=C:/lib64/lib/pkgconfig
cmake -G "Visual Studio 17 2022" -A x64 -D CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded -S . -B build/x64
msbuild /t:Rebuild /m /p:Configuration=Release /p:Platform=x64 .\build\x64\assrender.sln
```

For 32-bit (x86):

```
set PKG_CONFIG_PATH=C:/lib32/lib/pkgconfig
cmake -G "Visual Studio 17 2022" -A Win32 -D CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded -S . -B build/Win32
msbuild /t:Rebuild /m /p:Configuration=Release /p:Platform=Win32 .\build\Win32\assrender.sln
```

Important: The generator must match with the one from the vsvars64.bat or vsvars32.bat was run.

Example on non-standard toolset alternative (here for Visual Studio 2026 v143 toolset + Win32); replace the cmake line in the above script:
```
cmake -G "Visual Studio 18 2026" -T "v143" -A Win32 -D CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded -D PKG_CONFIG_EXECUTABLE="%LocalAppData%\Microsoft\WinGet\Links\pkg-config.exe" -S . -B build\Win32
```
#### 2.4 Find results

You can find your compiled plugin and the associated import library here:

- 64 bit DLL: <yourrootfolder>\assrender\build\x64\src\Release\assrender.dll
- 32-bit DLL: <yourrootfolder>\assrender\build\Win32\src\Release\assrender.dll
- 64-bit Lib: <yourrootfolder>\assrender\build\x64\src\Release\assrender.lib
- 32-bit Lib: <yourrootfolder>\assrender\build\Win32\src\Release\assrender.lib
- Static Libraries: Stored in C:\lib64 and C:\lib32 respectively.
- VS2022 64 bit .sln file: <yourrootfolder>\assrender\build\x64\assrender.sln
- VS2022 32 bit .sln file: <yourrootfolder>\assrender\build\Win32\assrender.sln

#### 2.5 Outputs to check, troubleshooting

**cmake config output**

- The CXX compiler identification (e.g.: MSVC 19.38.33145.0, which means an older VS2022 v143 toolset); 19.38 and 19.44 is common and still Win7 compatible.

  However this implies the minimum Microsoft VC Redistributable version. 
  The used 19.44 toolset requires a 19.44 VC Redistributable version.
  Even Windows 7 is still fine with 19.44, so if your dependencies are missing, just grab and install that.
  Visual Studio 2026 v145 will start from 19.50.

- The proper path the the working CXX compiler: C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.38.33130/bin/Hostx64/x64/cl.exe
- "Found PkgConfig". If not found, you have PATH problems or pkg-config is not installed.
- "Found libass": if not found, then the LIB path is wrong, or `libass` was not built successfully.


**msbuild output**

When you have mingw errors, check the Troubleshooting section

**Troubleshooting**

- unresolved external symbol __mingw...: This means your libass.a was built with MinGW. Delete the libass/build (build32/build64) folder 
  and ensure you run set CC=cl and set CXX=cl before running meson setup.

- Package 'freetype2' not found: Ensure PKG_CONFIG_PATH is set to C:\libXX\lib\pkgconfig. If building statically, you may need 
  to edit C:\libXX\lib\pkgconfig\libass.pc and clear the Requires: line if dependencies aren't being found.

- cannot open input file 'm.lib': Open libass.pc and remove -lm from the Libs: line. Windows does not use a separate math library.

- LNK1112 (Machine Type Conflict): You are trying to link 64-bit libass into a 32-bit assrender (or vice versa). 
  Check that your PKG_CONFIG_PATH matches the target architecture.

### 3. Build (alternative, x64 only variant, for clean VS2022 dev systems)

#### 3.1 Get sources and AviSynth libs
 
Same as in the main Build description.

#### 3.2 Build libass and assrender

  We'll set only one `PKG_CONFIG_PATH` here, for x64 where the newly installed libass configuration is located.
  Lib and Package folder root is `C:/lib64`, to match the 64-bit versions of the previous section.
  Build folders are `build/x64`.

- run command `call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"`

  or if you have Community Edition: 
  `call "c:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"`
  
  or if you have VS 2026, run command: 
  `call "c:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"`
- go to `libass` folder and run `meson wrap update-db && meson wrap install fribidi && meson wrap install freetype2 && meson wrap install expat && meson wrap install harfbuzz && meson wrap install libpng && meson wrap install zlib`
- run `meson setup build/x64 --wrap-mode=forcefallback -Ddefault_library=static -Dbuildtype=release -Dasm=enabled -Db_vscrt=static_from_buildtype -Dc_std=c11 -Dcpp_std=c++17 --prefix=C:/lib64`
- run `meson compile -C build/x64`
- run `meson install -C build/x64`
- go back to `assrender` folder and run
  `set PKG_CONFIG_PATH=C:/lib64/lib/pkgconfig`
- run `cmake -G "Visual Studio 17 2022" -A x64 -D CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded -S . -B build/x64`

  It is important that the generator match with the one from the vsvars64.bat was run.
  pkg-config must also be in the PATH.
  "libass not found" is a bad sign.

- run `msbuild /t:Rebuild /m /p:Configuration=Release /p:Platform=x64 .\build\x64\assrender.sln`

### Windows GCC (UCRT64 via MSYS2)

This environment builds against the modern **Universal C Runtime (UCRT)**. 

**1. Clone repo**
  ```
  git clone https://github.com/pinterf/assrender
  ```

Note: This environment uses system packages for libass; git submodules are not required.

**2. Prerequisite: AviSynth+ libraries**

Copy `AviSynth.lib` for both architectures into the following project folders:

  * 64-bit: `assrender/lib/x86-64/`
  * 32-bit: `assrender/lib/x86-32/`

You can find these in your AviSynth+ FilterSDK installation or the "filesonly" GitHub releases.

**3. Prerequisite: MSYS2 (UCRT64) packages**

Open the MSYS2 UCRT64 terminal (ucrt64.exe) and install the required tools:

```
pacman -S mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-cmake \
          mingw-w64-ucrt-x86_64-ninja \
          mingw-w64-ucrt-x86_64-libass
```

**4. Build**

Run the following commands within the MSYS2 UCRT64 terminal:

```
mkdir build_ucrt && cd build_ucrt
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

**5. Deployment & dependencies (UCRT64)**

Because this build uses the system libass package, the resulting assrender.dll is not 
a standalone file. To use it on another system (or in an AviSynth+ plugins folder), you must 
have the dependencies on your system (as you need Visual C++ Redistributables on non-MSYS2 builds).

Check dependencies:

```
ldd src/assrender.dll
```


### Windows GCC (Legacy MinGW64 via MSYS2)

From the previous section, perform Step 1 and 2.

**3. Prerequisite: MSYS2 Packages**

Open the MSYS2 MINGW64 terminal (mingw64.exe):

```
pacman -S mingw-w64-x86_64-gcc \
          mingw-w64-x86_64-cmake \
          mingw-w64-x86_64-ninja \
          mingw-w64-x86_64-libass
```

**4. Build**
```
mkdir build_mingw64 && cd build_mingw64
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Linux
* Clone repo
  - `git clone https://github.com/pinterf/assrender`
  - `cd assrender`
  - `cmake -B build -S .`
  - `cmake --build build --clean-first`

* Remark:
  - submodules are not needed, libass is used as a package.

* Find binaries at
   - `build/assrender/libassrender.so`

* Install binaries
  - `cd build`
  - `sudo make install`
