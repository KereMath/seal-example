FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libasio-dev \
    && rm -rf /var/lib/apt/lists/*

# Install Crow (Header only)
RUN git clone https://github.com/CrowCpp/Crow.git /crow && cp -r /crow/include/* /usr/local/include/

# Copy SEAL source
COPY seal /seal

# Build SEAL
WORKDIR /seal
RUN cmake -S . -B build -DSEAL_BUILD_EXAMPLES=OFF -DSEAL_BUILD_TESTS=OFF -DSEAL_BUILD_BENCH=OFF .
RUN cmake --build build
RUN cmake --install build

# Copy Backend Source
WORKDIR /app
ARG CACHEBUST=3
COPY backend /app

# Build Backend
RUN cmake -S . -B build
RUN cmake --build build

# Run Server
CMD ["./build/seal_server"]
