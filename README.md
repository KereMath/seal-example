# 🛡️ Homomorphic Encryption Demo: Microsoft SEAL + Docker

![Microsoft SEAL](https://img.shields.io/badge/Microsoft%20SEAL-4.1-blue?style=for-the-badge)
![Docker](https://img.shields.io/badge/Docker-2496ED?style=for-the-badge&logo=docker&logoColor=white)
![WebAssembly](https://img.shields.io/badge/WebAssembly-654FF0?style=for-the-badge&logo=webassembly&logoColor=white)
![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)

> **Privacy-Preserving Voting Demo:** Securely accumulate votes without revealing individual choices.

This project demonstrates a **Homomorphic Encryption** voting system. The server manages the keys and accumulates encrypted votes. It can only decrypt the final aggregated tally, ensuring that individual submissions remain private during the accumulation process.

---

## 🏗️ System Architecture

The following diagram illustrates the actual data flow in this implementation.

```mermaid
sequenceDiagram
    participant U as User
    participant B as Browser (WASM)
    participant S as Server (C++ SEAL)

    Note over S: 1. Server Startup<br/>Generates PK & SK<br/>(SK stays on Server)
    Note over B: 2. Client Init<br/>Fetches PK from Server
    B->>S: GET /api/keys
    S-->>B: Return Public Key (PK)
    
    U->>B: Enter Vote (Number)
    Note over B: 3. Encrypt with PK<br/>E(Vote)
    B->>S: POST /api/submit { E(Vote) }
    Note over S: 4. Homomorphic Add<br/>Accumulator += E(Vote)<br/>(No Decryption yet)
    S-->>B: Acknowledge

    U->>B: Click "Show Tally"
    B->>S: POST /api/tally
    Note over S: 5. Decrypt Tally<br/>Uses SK to decrypt sum
    S-->>B: Return Plaintext Result
    B->>U: Display Final Sum
```

### Key Components

| Component | Responsibility |
| :--- | :--- |
| **Server (Backend)** | **Key Authority.** Generates and holds the Secret Key. Accumulates encrypted votes. Decrypts only the final result. |
| **Client (Frontend)** | **Voter.** Fetches Public Key. Encrypts individual votes. Displays the final tally. |
| **WASM Module** | Performs encryption in the browser using the server's Public Key. |

---

## 📂 Project Structure

```
sealdockertrial/
├── seal/                          # Microsoft SEAL library
├── backend/
│   ├── src/server.cpp             # Generates Keys, Accumulates Votes, Decrypts Tally
│   └── Dockerfile                 # C++ Server build
├── frontend/
│   ├── public/script.js           # Fetches PK, Encrypts Inputs
│   ├── wasm/bindings.cpp          # WASM Bindings for SEAL
│   └── Dockerfile                 # Frontend build
├── docker-compose.yml             # Orchestration
└── README.md                      # This documentation
```

---

## 🚀 Quick Start

### Installation & Run

```bash
# 1. Clone and Init
git clone https://github.com/yourusername/sealdockertrial.git
cd sealdockertrial
git submodule update --init --recursive

# 2. Build and Run
docker-compose up --build -d
```

### Usage

1.  Go to **[http://localhost:3000](http://localhost:3000)**.
2.  **Submit Votes**: Enter numbers and click Submit. The browser encrypts them and sends them to the server.
3.  **Show Tally**: Click "Decrypt & Show Tally". The server decrypts the accumulated sum and returns it.

---

## 🔒 Security Model

*   **Server-Side Key Management**: The server generates and holds the Secret Key.
*   **Homomorphic Accumulation**: The server adds encrypted votes together without decrypting them individually.
*   **Privacy**: Individual votes are never decrypted. Only the final aggregated sum is decrypted by the server upon request.

---

## 🛠️ Technical Details

*   **Encryption Scheme**: CKKS (Approximate arithmetic).
*   **Parameters**: Poly Modulus Degree 32768.
*   **API**:
    *   `GET /api/keys`: Returns Public Key.
    *   `POST /api/submit`: Accepts encrypted vote.
    *   `POST /api/tally`: Returns decrypted sum.
*   **WASM Bindings**:
    *   `encrypt_number(double)`: Encrypts value using loaded Public Key.
    *   `get_context_info()`: Returns SEAL context details.
    *   *Note: Decryption is not exposed to the client.*

---

**License**: MIT