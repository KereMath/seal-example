# Stage 1: Build WASM
FROM emscripten/emsdk:3.1.51 as builder

# Copy SEAL source
COPY seal /seal

# Build SEAL for WASM
WORKDIR /seal
# We need to configure SEAL for WASM (disable shared libs, enable intrinsics if supported or disable if not)
# Emscripten usually handles intrinsics well now with -msimd128 if needed, but standard SEAL might need tweaks.
# For simplicity, we'll try a standard build with Emscripten toolchain.
RUN emcmake cmake -S . -B build \
    -DSEAL_BUILD_EXAMPLES=OFF \
    -DSEAL_BUILD_TESTS=OFF \
    -DSEAL_BUILD_BENCH=OFF \
    -DSEAL_USE_ZLIB=OFF \
    -DSEAL_USE_ZSTD=OFF \
    -DSEAL_USE_MSGSL=OFF \
    -DCMAKE_BUILD_TYPE=Release

RUN emmake make -C build -j4

# Copy Frontend Source
WORKDIR /app
COPY frontend/wasm /app/wasm

# Build Bindings
# We link against the libseal-4.1.a we just built
RUN emcc -O3 \
    -I/seal/native/src \
    -I/seal/build/native/src \
    /app/wasm/bindings.cpp \
    /seal/build/lib/libseal-4.1.a \
    -o /app/seal_wasm.js \
    -s WASM=1 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s MODULARIZE=1 \
    -s EXPORT_NAME="SEAL" \
    --bind \
    -fexceptions \
    -std=c++17

# Stage 2: Serve with Nginx
FROM nginx:alpine

COPY frontend/public /usr/share/nginx/html
COPY frontend/public/favicon.ico /usr/share/nginx/html/
COPY --from=builder /app/seal_wasm.js /usr/share/nginx/html/
COPY --from=builder /app/seal_wasm.wasm /usr/share/nginx/html/

EXPOSE 80
