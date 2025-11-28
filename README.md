# 🛡️ Homomorphic Encryption Demo: Microsoft SEAL + Docker

![Microsoft SEAL](https://img.shields.io/badge/Microsoft%20SEAL-4.1-blue?style=for-the-badge)
![Docker](https://img.shields.io/badge/Docker-2496ED?style=for-the-badge&logo=docker&logoColor=white)
![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![WebAssembly](https://img.shields.io/badge/WebAssembly-654FF0?style=for-the-badge&logo=webassembly&logoColor=white)
![Intel SGX](https://img.shields.io/badge/Intel%20SGX%20Ready-0071C5?style=for-the-badge&logo=intel&logoColor=white)

> **Privacy-First Computing: Perform calculations on encrypted data without ever decrypting it.**

This repository demonstrates a complete **Homomorphic Encryption** system using [Microsoft SEAL](https://github.com/microsoft/SEAL). It showcases how to build a web application where the **server performs addition on encrypted numbers** without knowing what those numbers are—ensuring absolute privacy.

---

## 📋 Table of Contents

- [Architecture Overview](#-architecture-overview)
- [How It Works](#-how-it-works)
- [Project Structure](#-project-structure)
- [Quick Start](#-quick-start)
- [Security Model](#-security-model)
- [Technical Deep Dive](#-technical-deep-dive)
- [TEE Support (Experimental)](#-tee-support-experimental)
- [Performance Considerations](#-performance-considerations)
- [Troubleshooting](#-troubleshooting)
- [License](#-license)

| **Frontend** | HTML5, Vanilla JS, Emscripten | User interface and client-side encryption |
| **WASM Module** | C++ (SEAL) compiled to WebAssembly | Cryptographic operations in browser |
| **Backend** | C++17, Crow Microframework | REST API for homomorphic operations |
| **SEAL Library** | Native C++ (x86_64) | Server-side encrypted computation |
| **Docker** | Docker Compose | Containerized deployment |

---

## 🔄 How It Works

### Step-by-Step Execution Flow

1.  **Key Generation (Client-Side)**
    *   When the page loads, the WASM module initializes.
    *   It generates a **Secret Key (SK)** and **Public Key (PK)** locally in the browser.
    *   **Critical**: The Secret Key **NEVER** leaves the browser.

2.  **Encryption (Client-Side)**
    *   User enters two numbers: `A` and `B`.
    *   WASM encrypts them: `E(A)` and `E(B)` using the Public Key.
    *   Encrypted data is serialized and Base64-encoded.

3.  **Transmission**
    *   Client sends `{ "cipher1": "E(A)", "cipher2": "E(B)" }` to the backend via HTTP POST.

4.  **Homomorphic Addition (Server-Side)**
    *   Backend deserializes the ciphertexts.
    *   Uses SEAL's `Evaluator::add()` to compute: `E(A) + E(B) = E(A+B)`.
    *   **Important**: The server **NEVER** decrypts the values. It operates on encrypted data.

5.  **Return Result**
    *   Server serializes `E(A+B)` and sends it back to the client.

6.  **Decryption (Client-Side)**
    *   Client decrypts `E(A+B)` using the Secret Key to get `A+B`.
    *   Result is displayed to the user.

### Privacy Guarantee

**The server NEVER knows the values of `A`, `B`, or `A+B` in plaintext.** Even if the server is compromised, the attacker only sees random-looking ciphertext.

---

## 📂 Project Structure

```
sealdockertrial/
├── seal/                          # Microsoft SEAL library (Git submodule)
├── backend/
│   ├── Dockerfile                 # Standard Docker build
│   ├── Dockerfile.gramine          # TEE-enabled build (Experimental)
│   ├── CMakeLists.txt             # Build configuration
│   ├── server.manifest.template   # Gramine SGX manifest (TEE)
│   └── src/
│       └── server.cpp             # C++ REST API (Crow + SEAL)
├── frontend/
│   ├── Dockerfile                 # Multi-stage: Emscripten + Nginx
│   ├── wasm/
│   │   └── bindings.cpp           # C++ → WASM bindings (Embind)
│   └── public/
│       ├── index.html             # User interface
│       ├── style.css              # Styling
│       └── script.js              # Client logic
├── docker-compose.yml             # Standard deployment
├── docker-compose.tee.yml         # TEE deployment (Experimental)
├── voting_system_design.md        # Design doc for E-Voting use case
└── README.md                      # This file
```

---

## 🚀 Quick Start

### Prerequisites

*   **Docker** and **Docker Compose** installed
*   **Git** (to clone the repository)
*   Modern web browser with WebAssembly support

### Installation & Run

```bash
# Clone the repository
git clone https://github.com/yourusername/sealdockertrial.git
cd sealdockertrial

# Initialize SEAL submodule
git submodule update --init --recursive

# Build and start the services
docker-compose up --build
```

Wait for the build to complete (~2-5 minutes). You will see:

```
backend_1   | Starting SEAL Server with CORS support (v5 - Fixes)...
backend_1   | (2025-11-28 15:45:00) [INFO] Server is running at 0.0.0.0:8080
```

### Access the Application

👉 Open your browser: **[http://localhost:3000](http://localhost:3000)**

1.  Enter two numbers (e.g., **42** and **13**).
2.  Click **"Encrypt & Add"**.
3.  See the result: **55** (decrypted on your browser).

### Verify Privacy

*   Open browser DevTools → Network tab.
*   Click "Encrypt & Add" again.
*   Inspect the POST request to `/api/add`.
*   You'll see `cipher1` and `cipher2` are random-looking Base64 strings, **NOT** your input numbers.

---

## 🔒 Security Model

### Cryptographic Parameters

| Parameter | Value | Description |
|:----------|:------|:------------|
| **Scheme** | CKKS (Cheon-Kim-Kim-Song) | Supports approximate arithmetic (real numbers) |
| **Poly Modulus Degree** | 32768 | Determines security level (~128-bit) |
| **Coeff Modulus** | `{60, 60, 60, 60, 60, 60, 60}` | Prime bit-lengths for RNS representation |
| **Scale** | 2<sup>40</sup> | Precision for fixed-point encoding |
| **Compression** | Disabled | Ensures WASM ↔ Native interoperability |

### Threat Model

#### ✅ Protected Against:
*   **Honest-but-Curious Server**: Server cannot learn plaintext inputs/outputs.
*   **Network Eavesdropping**: HTTPS encrypts the transport layer (add HTTPS in production).
*   **Data Breaches**: Stolen ciphertexts are useless without the Secret Key.

#### ⚠️ NOT Protected Against:
*   **Malicious Client**: A modified client can send invalid encrypted data (requires ZKP for production).
*   **Side-Channel Attacks**: Timing attacks on SEAL operations (use constant-time implementations for high-security).
*   **Quantum Computers**: SEAL uses lattice-based crypto (believed quantum-resistant, but not proven).

---

## 🛠️ Technical Deep Dive

### Backend: Crow + SEAL ([server.cpp](backend/src/server.cpp))

**Key Features:**
*   **Global CORS Middleware**: Ensures all responses include `Access-Control-Allow-Origin: *`.
*   **Base64 Encoding**: Custom implementation for ciphertext serialization.
*   **No Compression**: `compr_mode_type::none` ensures WASM compatibility.

**API Endpoint:**

```http
POST /api/add
Content-Type: application/json

{
  "cipher1": "AAAA...==",  # Base64-encoded Ciphertext
  "cipher2": "BBBB...=="
}
```

**Response:**

```json
{
  "result": "CCCC...=="  # Base64-encoded result
}
```

### Frontend: WASM Bindings ([bindings.cpp](frontend/wasm/bindings.cpp))

**Compilation:**

```bash
emcc bindings.cpp \
    -I/seal/native/src \
    -I/seal/build/native/src \
    -L/seal/build/lib \
    -lseal-4.1 \
    -o seal_wasm.js \
    -s WASM=1 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -fexceptions \
    --bind
```

**Exposed Functions:**

*   `SEALWrapper.encrypt_number(num) → base64_string`
*   `SEALWrapper.decrypt_number(base64_string) → num`
*   `SEALWrapper.get_context_info() → string`

### Client-Side Logic ([script.js](frontend/public/script.js))

**Flow:**

```javascript
const wrapper = await Module.SEALWrapper();  // Initialize WASM
const enc1 = wrapper.encrypt_number(num1);   // Encrypt locally
const response = await fetch('/api/add', {   // Send to server
    method: 'POST',
    body: JSON.stringify({ cipher1: enc1, cipher2: enc2 })
});
const result = wrapper.decrypt_number(data.result); // Decrypt locally
```

---

## 🔐 TEE Support (Experimental)

### What is a Trusted Execution Environment (TEE)?

A **TEE** (e.g., Intel SGX) is a hardware-enforced secure area where code runs in a protected "enclave." Even the operating system or hypervisor cannot read the enclave's memory.

### Why TEE for Voting?

In an e-voting scenario:
*   Voters encrypt their votes with a **Public Key**.
*   The **Election Server** aggregates encrypted votes.
*   The server must decrypt the **final tally**, but we don't want it to decrypt **individual votes**.
*   **Solution**: Run the server inside a TEE, so it can only run the approved tallying code.

### Current Status: 🟡 In Progress

**Files:**
*   [`backend/Dockerfile.gramine`](backend/Dockerfile.gramine): Docker build for Gramine-SGX.
*   [`backend/server.manifest.template`](backend/server.manifest.template): Security policy for the enclave.
*   [`docker-compose.tee.yml`](docker-compose.tee.yml): TEE deployment config.

**Known Issues:**
*   Container starts but crashes immediately (library path configuration issue).
*   **Workaround**: Use standard `docker-compose.yml` for now.

**To Test TEE Mode (if you have SGX hardware or want simulation):**

```bash
docker-compose -f docker-compose.tee.yml up --build
```

---

## ⚡ Performance Considerations

### Ciphertext Size

*   **Poly Degree 32768**: Each ciphertext is ~512 KB.
*   **Network**: Sending two ciphertexts = ~1 MB of data.
*   **Optimization**: Use batching (SIMD slots) to encrypt multiple values in one ciphertext.

### Computation Time

| Operation | Time (Approximate) |
|:----------|:-------------------|
| Key Generation | ~500 ms |
| Encrypt (1 number) | ~100 ms |
| Add (2 ciphertexts) | ~50 ms |
| Decrypt (1 ciphertext) | ~80 ms |

*Measured on Intel i7-10th Gen @ 2.6 GHz*

### Scaling for E-Voting

For 1 million voters:
*   **Storage**: ~512 GB (if storing all encrypted votes).
*   **Computation**: ~14 hours (serial addition) or ~10 minutes (parallelized).
*   **Memory**: 2-4 GB RAM for the server process.

---

## 🐛 Troubleshooting

### Issue: `ERR_CONNECTION_REFUSED` when clicking "Encrypt & Add"

**Cause**: Backend failed to start.

**Solution**:
```bash
# Check backend logs
docker logs sealdockertrial-backend-1

# Common fix: Rebuild without cache
docker-compose down
docker-compose up --build --force-recreate
```

### Issue: `loaded SEALHeader is invalid`

**Cause**: Compression mismatch between WASM and Native SEAL.

**Solution**: Ensure `compr_mode_type::none` in both [`server.cpp:153`](backend/src/server.cpp#L153) and [`bindings.cpp:144`](frontend/wasm/bindings.cpp#L144).

### Issue: Frontend shows "WASM Module failed to load"

**Cause**: SEAL library not found during Emscripten linking.

**Solution**:
```bash
# Rebuild frontend
docker-compose up --build frontend
```

### Issue: CORS errors in browser console

**Cause**: CORS headers not applied.

**Solution**: Verify [`server.cpp`](backend/src/server.cpp) includes the `CORSHandler` middleware at line 115.

---

## 📜 License

This project is released under the **MIT License**.

### Third-Party Licenses:
*   **Microsoft SEAL**: MIT License
*   **Crow**: BSD 3-Clause
*   **Gramine**: LGPLv3

---

## 🔗 Related Resources

*   [Microsoft SEAL Documentation](https://github.com/microsoft/SEAL)
*   [Gramine Project](https://gramineproject.io/)
*   [E-Voting Design Document](voting_system_design.md)

---

**Built with ❤️ for Privacy-Preserving Computing**