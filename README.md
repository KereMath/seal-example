# 🛡️ Homomorphic Encryption Demo: Microsoft SEAL + Docker

![Microsoft SEAL](https://img.shields.io/badge/Microsoft%20SEAL-4.1-blue?style=for-the-badge)
![Docker](https://img.shields.io/badge/Docker-2496ED?style=for-the-badge&logo=docker&logoColor=white)
![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![WebAssembly](https://img.shields.io/badge/WebAssembly-654FF0?style=for-the-badge&logo=webassembly&logoColor=white)
![Intel SGX](https://img.shields.io/badge/Intel%20SGX%20Ready-0071C5?style=for-the-badge&logo=intel&logoColor=white)

> **Privacy‑First Computing:** Perform calculations on encrypted data without ever decrypting it.

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

| **Frontend** | HTML5, Vanilla JS, Emscripten | User interface and client‑side encryption |
| **WASM Module** | C++ (SEAL) compiled to WebAssembly | Cryptographic operations in browser |
| **Backend** | C++17, Crow Microframework | REST API for homomorphic operations |
| **SEAL Library** | Native C++ (x86_64) | Server‑side encrypted computation |
| **Docker** | Docker Compose | Containerized deployment |

---

## 🔄 How It Works
### Step‑by‑step execution flow
1. **Key Generation (Client‑Side)** – When the page loads, the WASM module initializes and creates a public/secret key pair. The secret key never leaves the browser.
2. **Encryption (Client‑Side)** – Numbers entered by the user are encrypted with the public key.
3. **Transmission** – Encrypted ciphertexts are sent to the backend via `POST /api/add`.
4. **Homomorphic Addition (Server‑Side)** – The server adds ciphertexts without decrypting them.
5. **Result Return** – The server returns the encrypted sum.
6. **Decryption (Client‑Side)** – The browser decrypts the final sum using the secret key.

**Privacy guarantee:** The server never sees plaintext inputs or outputs.

---

## 📂 Project Structure
```
sealdockertrial/
├── seal/                          # Microsoft SEAL library (submodule)
├── backend/
│   ├── Dockerfile                 # Standard backend build
│   ├── Dockerfile.gramine          # Experimental TEE build
│   ├── CMakeLists.txt             # Build config
│   └── src/server.cpp             # Crow + SEAL API
├── frontend/
│   ├── Dockerfile                 # Multi‑stage: Emscripten → Nginx
│   ├── wasm/bindings.cpp           # C++ → WASM bindings (Embind)
│   └── public/
│       ├── index.html             # UI (script src now `/seal_wasm.js`)
│       ├── style.css              # Styling
│       ├── script.js              # Client logic
│       └── favicon.ico            # New 16×16 blue square icon
├── docker-compose.yml             # Standard deployment
├── docker-compose.tee.yml         # Experimental TEE deployment
├── voting_system_design.md        # Design doc for e‑voting use case
└── README.md                      # This file (updated)
```

---

## 🚀 Quick Start
### Prerequisites
- **Docker** and **Docker Compose**
- **Git** (to clone the repo)
- Modern browser with WebAssembly support

### Installation & Run
```bash
# Clone the repository
git clone https://github.com/yourusername/sealdockertrial.git
cd sealdockertrial

# Initialise SEAL submodule
git submodule update --init --recursive

# Build and start the services
docker-compose up --build -d
```
Wait a few moments for the containers to become healthy (frontend on port **3000**, backend on **8080**).

### Access the application
Open your browser at **[http://localhost:3000](http://localhost:3000)**.
- The page now loads a favicon (`favicon.ico`).
- The script tag in `index.html` points to `/seal_wasm.js`, ensuring the WASM bundle is correctly served.
- Enter a number and click **📤 Submit** – the activity log will show encryption, submission, and tally steps.

---

## 🔒 Security Model
### Cryptographic parameters
| Parameter | Value | Description |
|---|---|---|
| **Scheme** | CKKS | Approximate arithmetic (real numbers) |
| **Poly Modulus Degree** | 32768 | ~128‑bit security |
| **Coeff Modulus** | `{60,60,60,60,60,60,60}` | RNS representation |
| **Scale** | 2^40 | Fixed‑point precision |
| **Compression** | Disabled | Guarantees WASM ↔ native compatibility |

### Threat model
**Protected against**: honest‑but‑curious server, network eavesdropping (use HTTPS in production), data breaches.
**Not protected against**: malicious client, side‑channel attacks, quantum adversaries.

---

## 🛠️ Technical Deep Dive
### Backend (Crow + SEAL)
- Global CORS middleware (`Access‑Control‑Allow‑Origin: *`).
- Base64 encoding for ciphertext transport.
- No compression (`compr_mode_type::none`).

**API endpoint** `POST /api/add` expects JSON `{ "cipher1": "...", "cipher2": "..." }` and returns `{ "result": "..." }`.

### Frontend (WASM bindings)
Compile with:
```bash
emcc bindings.cpp \
    -I/seal/native/src \
    -I/seal/build/native/src \
    -L/seal/build/lib \
    -lseal-4.1 \
    -o seal_wasm.js \
    -s WASM=1 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s MODULARIZE=1 \
    -s EXPORT_NAME="SEAL" \
    --bind \
    -fexceptions
```
Exposed functions: `encrypt_number`, `decrypt_number`, `get_context_info`.

---

## 🔐 TEE Support (Experimental)
See `backend/Dockerfile.gramine` and `docker-compose.tee.yml` for SGX‑enabled builds.

---

## ⚡ Performance Considerations
- Ciphertext size ≈ 512 KB per value (poly degree 32768).
- Typical timings on Intel i7‑10th Gen: key generation ~500 ms, encrypt ~100 ms, add ~50 ms, decrypt ~80 ms.
- Scaling to 1 M voters → ~512 GB storage, ~14 h serial compute (parallelizable).

---

## 🐛 Troubleshooting
- **`ERR_CONNECTION_REFUSED`** – Backend may have failed. Run `docker logs sealdockertrial-backend-1` and rebuild with `docker-compose down && docker-compose up --build --force-recreate`.
- **`loaded SEALHeader is invalid`** – Ensure both backend and WASM use `compr_mode_type::none`.
- **WASM fails to load** – Rebuild frontend: `docker-compose up --build frontend`.
- **CORS errors** – Verify `CORSHandler` middleware is applied in `backend/src/server.cpp`.

---

## 📜 License
This project is released under the **MIT License**.

---

## 🔗 Related Resources
- [Microsoft SEAL Documentation](https://github.com/microsoft/SEAL)
- [Gramine Project](https://gramineproject.io/)
- [E‑Voting Design Document](voting_system_design.md)

---

**Built with ❤️ for privacy‑preserving computing**