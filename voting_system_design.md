# TEE-Based Secure Homomorphic E-Voting System Design

## 1. Executive Summary
This design leverages **Trusted Execution Environments (TEEs)**—specifically **Intel SGX** or **AWS Nitro Enclaves**—to create a "Black Box" Election Server.

In this model, the **Election Server** runs inside a hardware-protected memory region (Enclave). Even the system administrator with root access **cannot read the memory** of the Enclave. This guarantees that the **Secret Key**, which is generated and held strictly inside the Enclave, remains secure.

## 2. Can I use Microsoft SEAL in a TEE?
**YES.**
*   **Compatibility**: Microsoft SEAL is a standard C++ library with no dependencies on system calls (filesystem, network) for its core arithmetic. It relies on standard C++ math libraries, which are fully supported in TEEs.
*   **Performance**: Homomorphic encryption is CPU-intensive. Running it inside an SGX Enclave has a minor performance overhead (due to memory encryption/decryption paging), but it is fully functional and performant enough for voting aggregation.

## 3. Do I need to write hardware-specific code?
**It depends on your implementation strategy.** There are two paths:

### Path A: The "Native SDK" Route (Hard Mode)
*   **How**: You use the **Intel SGX SDK** or **Open Enclave SDK**.
*   **Code Changes**: You must split your C++ application into two parts:
    1.  **Trusted Part (Enclave)**: Contains SEAL logic, Key Generation, and Tallying.
    2.  **Untrusted Part (App)**: Handles Networking (Crow), File I/O, and calls the Trusted Part via "ECALLs".
*   **Pros**: Smallest attack surface.
*   **Cons**: Requires significant rewriting of your `server.cpp`. You cannot just "run" your existing Docker container.

### Path B: The "LibOS" / Container Route (Recommended)
*   **How**: You use a Library OS like **Gramine** (formerly Graphene) or **Occlum**, or **AWS Nitro Enclaves**.
*   **Code Changes**: **Zero to Minimal.**
    *   These tools allow you to run an **unmodified Docker container** or binary inside an Enclave.
    *   The LibOS acts as a compatibility layer, translating system calls (like Network/File I/O) securely.
*   **Pros**: You can use your existing `server.cpp` (Crow + SEAL) as is.
*   **Cons**: Slightly larger Trusted Computing Base (TCB) than Path A, but acceptable for this use case.

---

## 4. System Architecture (TEE Model)

### 4.1. The "Black Box" Server
The Election Server is a Docker container running inside a TEE (e.g., via Gramine-SGX).

1.  **Boot & Attestation**:
    *   When the server starts, the TEE hardware generates a **Remote Attestation Quote**.
    *   This is a cryptographic proof signed by the CPU manufacturer (e.g., Intel) proving: "I am a genuine SGX Enclave running *this specific code hash*."
    *   Clients (or the Admin) verify this quote before trusting the server.

2.  **Key Generation (Inside Enclave)**:
    *   The `server.cpp` generates `SecretKey` and `PublicKey`.
    *   **Crucial**: The `SecretKey` stays in RAM (encrypted by hardware). It is **never** written to disk.

3.  **Voting Phase**:
    *   Clients send encrypted votes via HTTPS (TLS terminates *inside* the Enclave for maximum security, or at the boundary).
    *   Server adds votes to the in-memory Tally.

4.  **Decryption Phase**:
    *   Admin sends "End Election" command.
    *   Server decrypts the result internally.
    *   Server returns the **Plaintext Result**.
    *   Server terminates and the `SecretKey` vanishes from memory.

---

## 5. Implementation Roadmap (LibOS Approach)

1.  **Develop Application**: Build your C++ Crow + SEAL application (Already done!).
2.  **Containerize**: Package it in Docker (Already done!).
3.  **Gramine Manifest**: Create a `manifest.sgx` file.
    *   This file tells Gramine which files/libraries the app needs (e.g., `libseal.so`, `server` binary).
4.  **Sign Enclave**: Use `gramine-sgx-sign` to generate the Enclave signature.
5.  **Run**: Execute `gramine-sgx server`.

## 6. Security Guarantees
*   **Confidentiality**: Admin cannot dump RAM to find the Secret Key.
*   **Integrity**: Admin cannot modify the code (e.g., to rig the tally) without changing the "Measurement Hash," which would fail Remote Attestation.
*   **Privacy**: Since the Server is the only entity that *could* decrypt (but is constrained by code to only decrypt the *final tally*), individual voter privacy is preserved.
