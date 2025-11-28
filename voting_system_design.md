# Advanced Secure Homomorphic E-Voting System Design

## 1. Executive Summary
To achieve **"Full Cryptographic Security"** and ensure that **no single administrator or server** possesses the power to decrypt individual votes or rig the election, we must move beyond a single "Secret Key" model.

This design proposes a **Threshold Cryptography (Multiparty Computation)** architecture. In this model, the "Secret Key" is never generated as a single file. Instead, it is mathematically split among multiple **Guardians** (Trustees). The Election Server acts purely as an aggregator and **cannot decrypt anything** on its own.

## 2. Core Security Philosophy
1.  **No Single Point of Trust**: Neither the Admin, nor the Server, nor any single Guardian holds the full key.
2.  **Privacy by Default**: Votes are encrypted on the client. Only the *final tally* is decrypted.
3.  **Verifiability**: Any observer can verify the mathematical correctness of the tally (using Zero-Knowledge Proofs).

---

## 3. System Architecture: The "Threshold" Model

### 3.1. Entities
1.  **Voter Clients (WASM)**: Encrypt votes using the **Joint Public Key**.
2.  **Election Server (Aggregator)**:
    *   Receives encrypted votes.
    *   Performs Homomorphic Addition.
    *   **Has NO decryption capability.**
3.  **The Guardians (Trustees)**:
    *   A group of $N$ entities (e.g., 3 officials, 1 auditor, 1 NGO).
    *   They hold **Key Shares**.
    *   A subset (Quorum $K$ of $N$) is required to decrypt the result.
4.  **Admin Console**: Orchestrates the process (Start/Stop) but **never sees keys**.

---

## 4. The Secure Pipeline (Step-by-Step)

### Phase 1: The Key Ceremony (Election Setup)
*Instead of the Server generating a key, the Guardians generate it together.*

1.  **Admin** initiates "Election Setup".
2.  **Election Server** signals the **Guardian Nodes**.
3.  **Distributed Key Generation (DKG)**:
    *   Each Guardian $G_i$ generates a random secret.
    *   They exchange public commitments (mathematical promises).
    *   **Result**:
        *   A **Joint Public Key ($PK_{joint}$)** is created and published to the Election Server.
        *   Each Guardian holds a **Secret Key Share ($SK_i$)**.
        *   **CRITICAL**: The full Secret Key $SK$ **never exists** anywhere in memory or disk. It is virtual.

### Phase 2: Voting & Aggregation (Automated)
1.  **Voter** fetches $PK_{joint}$ from Server.
2.  **Voter** encrypts vote: $C_{vote} = Encrypt(PK_{joint}, Vote)$.
3.  **Voter** generates a **Zero-Knowledge Proof (ZKP)**:
    *   Proves: "This ciphertext contains a valid vote (e.g., 1 for one candidate, 0 for others)" *without revealing the vote*.
4.  **Election Server**:
    *   Verifies the ZKP. (If invalid, reject vote).
    *   Adds vote to tally: $C_{tally} = C_{tally} + C_{vote}$.
    *   *Server sees only random noise.*

### Phase 3: Distributed Decryption (Election End)
*The Admin clicks "End Election". The Server cannot decrypt. It needs help.*

1.  **Election Server** publishes the final $C_{tally}$.
2.  **Admin** requests decryption from Guardians.
3.  **Partial Decryption**:
    *   Each Guardian $G_i$ downloads $C_{tally}$.
    *   Each Guardian computes a **Partial Decryption Share ($D_i$)** using their $SK_i$.
    *   $D_i$ is sent to the Election Server.
4.  **Result Combination**:
    *   The Election Server collects shares from at least $K$ Guardians.
    *   It combines them (Lagrange Interpolation) to reveal the **Plaintext Tally**.
5.  **Result**: The final counts are revealed. Individual votes remain encrypted forever.

---

## 5. Alternative: The "Black Box" Server (TEE Model)
*If you strictly want a single "Election Server" to handle everything automatically without external Guardians, you MUST use hardware protection.*

### Architecture
*   **Hardware**: Intel SGX (Software Guard Extensions) or AWS Nitro Enclaves.
*   **Concept**: The code runs inside a memory-encrypted **Enclave**. Even the Admin with root access cannot read the Enclave's memory.

### Pipeline
1.  **Start**: Admin sends command.
2.  **Enclave**: Generates $SK$ and $PK$ *inside* the protected memory.
3.  **Voting**: Enclave receives votes, adds them.
4.  **End**: Enclave decrypts the tally internally and outputs *only* the result.
5.  **Security**: The $SK$ is destroyed when the Enclave terminates.

**Pros**: Simpler architecture (no Guardians).
**Cons**: Requires trusting the hardware vendor (Intel/AWS) and that the Enclave code is bug-free.

---

## 6. Recommended "Full Security" Design (Hybrid)

For the highest security, combine **Threshold Cryptography** with **Homomorphic Encryption**.

### Where to implement what?

| Component | Location | Tech Stack | Security Responsibility |
| :--- | :--- | :--- | :--- |
| **Client** | Browser | JS / WASM (SEAL) | **Encrypt** vote with $PK_{joint}$. Generate ZKP. |
| **API Gateway** | Cloud | Nginx / Go | DDoS protection, Auth, Rate Limiting. |
| **Election Server** | Cloud (Docker) | C++ (SEAL) | **Verify** ZKP. **Aggregate** votes. Store Encrypted Tally. |
| **Guardian 1** | Admin's Laptop | C++ / Python | Holds Share 1. Participates in DKG & Decryption. |
| **Guardian 2** | Auditor's Server | C++ / Python | Holds Share 2. |
| **Guardian 3** | Offline USB | C++ / Python | Holds Share 3. |

### The "Admin Experience"
1.  Admin logs into **Admin Panel**.
2.  Clicks **"Initialize Election"**.
    *   System waits for Guardians to come online and perform DKG.
    *   "Election Live" status appears.
3.  Clicks **"End & Tally"**.
    *   System sends request to Guardians.
    *   Guardians approve/sign the decryption request.
    *   Results appear on screen.

## 7. Summary of Key Technologies

1.  **Microsoft SEAL (BFV Scheme)**: For Homomorphic Addition of integer votes.
2.  **Shamir's Secret Sharing / Threshold ElGamal**: For splitting the decryption key.
3.  **Zero-Knowledge Proofs (Schnorr / Bulletproofs)**: To prevent "negative votes" or "over-voting" by malicious clients.
4.  **Digital Signatures**: Every vote must be signed by the voter's private key (derived from their ID) to prevent tampering.
