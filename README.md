# n-body
## Clone
```bash
git clone --recurse-submodules -j2 https://github.com/NilEis/n-body.git
```

## Dependencies
```
pip install jinja2
```

### Multithreaded
- On windows: PThread (optional)

### CUDA
- CUDA Toolkit >= 12.1
- On windows: Visual Studio (not Visual Studio Code)

### OpenCL
- OpenCL

### Compute Shader
- OpenGL version 3.2(?) 

## Build
Build with CMake
```bash
cmake -S . -B build -D[USE_OPENCL, USE_CUDA, USE_COMPUTE_SHADER]=ON
cmake --build build
```

## Known issues
- VBO not updating on AMD with compute shader
