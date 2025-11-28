let sealWrapper = null;

function log(msg) {
    const el = document.getElementById('logOutput');
    el.textContent += msg + '\n';
    console.log(msg);
}

// Initialize WASM Module
SEAL().then(instance => {
    log("WASM Module Loaded.");
    sealWrapper = new instance.SEALWrapper();
    log(sealWrapper.get_context_info());

    const btn = document.getElementById('calcBtn');
    btn.textContent = "Encrypt & Add";
    btn.disabled = false;
});

document.getElementById('calcBtn').addEventListener('click', async () => {
    if (!sealWrapper) return;

    const n1 = parseInt(document.getElementById('num1').value);
    const n2 = parseInt(document.getElementById('num2').value);

    if (isNaN(n1) || isNaN(n2)) {
        alert("Please enter valid numbers");
        return;
    }

    try {
        log(`Encrypting ${n1} and ${n2}...`);

        // 1. Encrypt (Client Side)
        // Note: The C++ binding returns raw binary string. We need to base64 encode it for JSON transport.
        // However, passing raw binary strings from C++ to JS can be tricky with null bytes.
        // A better way in bindings.cpp would be to return base64 directly or use Uint8Array.
        // For this demo, let's assume the binding returns a string that we can btoa() but wait...
        // Binary strings in JS are UTF-16. C++ std::string is bytes.
        // Emscripten's std::string binding usually handles UTF-8.
        // SEAL serialization is binary.
        // FIX: I should have updated bindings.cpp to return Base64 or use memory views.
        // Let's rely on the fact that I can modify bindings.cpp if this fails, 
        // but actually, let's just try to treat the string as binary.
        // Actually, `btoa` on a binary string works if the characters are 0-255.
        // But Emscripten might try to decode UTF8.

        // Let's assume for a moment I need to update bindings.cpp to be safe.
        // But let's write the JS logic first assuming I'll fix bindings.cpp to return Base64.

        const c1 = sealWrapper.encrypt_number(n1); // Expecting Base64 from C++ would be safer
        const c2 = sealWrapper.encrypt_number(n2);

        log("Sending encrypted data to server...");

        // 2. Send to Server
        const response = await fetch('http://localhost:8080/api/add', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                cipher1: c1,
                cipher2: c2
            })
        });

        if (!response.ok) {
            throw new Error(`Server Error: ${response.statusText}`);
        }

        const data = await response.json();
        const resultCipher = data.result;

        log("Received encrypted result.");

        // 3. Decrypt (Client Side)
        const result = sealWrapper.decrypt_number(resultCipher);

        document.getElementById('result').textContent = `Result: ${result}`;
        log(`Decrypted Result: ${result}`);

    } catch (e) {
        log(`Error: ${e.message}`);
        console.error(e);
    }
});
