## Build instructions

### Windows

* Prequisite: install nasm, ninja, pkg-config-lite and meson
  - `winget install Ninja-build.Ninja NASM.NASM bloodrock.pkg-config-lite mesonbuild.meson`
  - set PKG_CONFIG_PATH env `setx PKG_CONFIG_PATH "C:/lib/pkgconfig"`

* Clone repos
  - Clone `git clone https://github.com/pinterf/assrender.git`
  - and clone libass as subdirectory `cd assrender && git clone https://github.com/libass/libass.git`

* Prequisite: avisynth.lib versions (x86 and x64)
  - When you have installed Avisynth through an installer and have installed FilterSDK  
    get it from c:\Program Files (x86)\AviSynth+\FilterSDK\lib\x86 and x64
  - Or get it from the 'filesonly' builds at Avisynth+ releases
    https://github.com/AviSynth/AviSynthPlus/releases
  - Copy lib files to assrender\lib\x86-64\ and assrender\lib\x86-32\
    32 and 64 bit versions respectively

* Build:
  - run command `call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"`
  - go to `libass` folder and run `meson wrap update-db && meson wrap install fribidi && meson wrap install freetype2 && meson wrap install expat && meson wrap install harfbuzz && meson wrap install libpng && meson wrap install zlib`
  - run `meson setup build -Ddefault_library=static -Dbuildtype=release -Dasm=enabled -Db_vscrt=static_from_buildtype -Dc_std=c11 -Dcpp_std=c++17`
  - run `meson compile -C build`
  - run `meson install -C build`
  - back to assrender folder and run ...
  - run `cmake -D CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded -A x64 -S . -B build`
  - run `msbuild /t:Rebuild /m /p:Configuration=Release /p:Platform=x64 .\build\assrender.sln`

### Linux
* Clone repo
  - git clone https://github.com/pinterf/assrender
  - cd assrender
  - cmake -B build -S .
  - cmake --build build --clean-first

* Remark:
  - submodules are not needed, libass is used as a package.

* Find binaries at
   - build/assrender/libassrender.so

* Install binaries
  - cd build
  - sudo make install
