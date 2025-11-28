let sealWrapper = null;
let submittedCount = 0;

function log(message) {
    const logDiv = document.getElementById('log');
    const timestamp = new Date().toLocaleTimeString();
    logDiv.innerHTML += `<div>[${timestamp}] ${message}</div>`;
    logDiv.scrollTop = logDiv.scrollHeight;
}

async function init() {
    try {
        log('Loading WASM module...');

        // Initialize the WASM module using the factory function 'SEAL'
        // defined by EXPORT_NAME="SEAL" and MODULARIZE=1 in Dockerfile
        const moduleInstance = await SEAL();

        log('Fetching Public Key from server...');
        const response = await fetch('http://localhost:8080/api/keys');
        const data = await response.json();

        log('Initializing SEAL with server Public Key...');
        sealWrapper = new moduleInstance.SEALWrapper(data.publicKey);

        log('✅ SEAL initialized (encryption-only mode)');
        log(sealWrapper.get_context_info());

        document.getElementById('submitBtn').disabled = false;
        document.getElementById('tallyBtn').disabled = false;
        document.getElementById('resetBtn').disabled = false;
    } catch (error) {
        log(`❌ Error: ${error.message}`);
        console.error('Initialization error:', error);
    }
}

async function submitNumber() {
    try {
        const input = document.getElementById('numberInput');
        const num = parseFloat(input.value);

        if (isNaN(num)) {
            log('❌ Please enter a valid number');
            return;
        }

        log(`Encrypting: ${num}...`);
        const encrypted = sealWrapper.encrypt_number(num);

        log('Sending encrypted value to server...');
        const response = await fetch('http://localhost:8080/api/submit', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ cipher: encrypted })
        });

        const result = await response.json();
        submittedCount = result.count;

        document.getElementById('voteCount').textContent = submittedCount;
        log(`✅ Submitted value #${submittedCount}`);

        input.value = '';
        input.focus();
    } catch (error) {
        log(`❌ Submission error: ${error.message}`);
        console.error('Submit error:', error);
    }
}

async function showTally() {
    try {
        log('Requesting tally from server...');
        const response = await fetch('http://localhost:8080/api/tally', {
            method: 'POST'
        });

        if (!response.ok) {
            const error = await response.text();
            throw new Error(error);
        }

        const result = await response.json();

        document.getElementById('result').innerHTML = `
            <strong>Final Sum:</strong> ${result.sum.toFixed(2)}<br>
            <small>From ${result.count} submissions</small>
        `;

        log(`🔓 Tally decrypted on server: ${result.sum.toFixed(2)}`);
    } catch (error) {
        log(`❌ Tally error: ${error.message}`);
        console.error('Tally error:', error);
    }
}

async function resetTally() {
    try {
        const confirmation = confirm('Are you sure you want to reset all submissions?');
        if (!confirmation) return;

        log('Resetting accumulator...');
        const response = await fetch('http://localhost:8080/api/reset', {
            method: 'POST'
        });

        const result = await response.json();

        submittedCount = 0;
        document.getElementById('voteCount').textContent = 0;
        document.getElementById('result').textContent = 'Tally not yet revealed';

        log('🔄 Accumulator reset successfully');
    } catch (error) {
        log(`❌ Reset error: ${error.message}`);
        console.error('Reset error:', error);
    }
}

// Enter key support
document.addEventListener('DOMContentLoaded', () => {
    const input = document.getElementById('numberInput');
    if (input) {
        input.addEventListener('keypress', (e) => {
            if (e.key === 'Enter') {
                submitNumber();
            }
        });
    }
});

// Initialize on page load
window.addEventListener('load', init);
