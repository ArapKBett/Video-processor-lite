Install Emscripten to compile C++ to WebAssembly:
```git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh```

Compile C++ to WebAssembly:
```emcc video_processor.cpp -o video_processor.js -s WASM=1 -s "EXPORTED_FUNCTIONS=['_process_video']" -s "EXTRA_EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap']"```
