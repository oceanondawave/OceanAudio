# Building OceanAudio on Windows

## Prerequisites
1. **Visual Studio 2022** (Community edition or higher)
   - Install "Desktop development with C++"
2. **CMake 3.24+** (https://cmake.org/download/)
   - During install, select "Add CMake to system PATH"
3. **Git** (https://git-scm.com/download/win)
4. **Internet connection** (for first build only - downloads JUCE and VST3 SDK)

## Build Steps

### Step 1: Clone/Download Dependencies
If CMake FetchContent fails, manually download dependencies:

```bash
# In the project root
mkdir -p external
cd external

# Download JUCE
git clone --depth 1 --branch 7.0.9 https://github.com/juce-framework/JUCE.git juce

# Download VST3 SDK
git clone --depth 1 --branch v3.7.12_build_20 https://github.com/steinbergmedia/vst3sdk.git vst3sdk
cd vst3sdk
git submodule update --init --recursive
cd ../..
```

### Step 2: Update CMake Files to Use Local Dependencies
If you downloaded dependencies manually, edit `cmake/modules/FetchJUCE.cmake`:

Replace:
```cmake
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.9
)
```

With:
```cmake
FetchContent_Declare(
    juce
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/juce
)
```

Do the same for `cmake/modules/FetchVST3SDK.cmake`:
```cmake
FetchContent_Declare(
    vst3sdk
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/vst3sdk
)
```

### Step 3: Generate Build Files
```bash
cmake -S . -B build -G "Visual Studio 17 2022"
```

### Step 4: Build the Host Application
```bash
cmake --build build --config Release --target OceanAudioHost
```
Output: `build\host\Release\OceanAudioHost.exe`

### Step 5: Build the Bridge Service
```bash
cmake --build build --config Release --target OceanAudioBridgeService
```
Output: `build\driver\service\Release\OceanAudioBridgeService.exe`

### Step 6: Build the Core Plugins
```bash
cmake --build build --config Release --target CoreEQ_VST3
cmake --build build --config Release --target CoreCompressor_VST3
cmake --build build --config Release --target CoreGate_VST3
```

## Troubleshooting

### "CMake step for juce failed"
- **Cause**: Network issue or firewall blocking git/https
- **Solution**: Download dependencies manually (see Step 1 above)

### "LINK : fatal error LNK1181: cannot open input file"
- **Cause**: Missing dependency or incorrect paths
- **Solution**: Clean and rebuild: `cmake --build build --config Release --clean-first`

### "Cannot find JUCE modules"
- **Cause**: FetchContent didn't complete
- **Solution**: Verify `build/_deps/juce-src` exists, or use manual download method

### Build is very slow
- **Cause**: CMake downloading large repos
- **Solution**: Use manual download method and local SOURCE_DIR

## Next Steps
After building successfully, proceed to driver installation (see `driver_setup.md`)

