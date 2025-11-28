#include "seal/seal.h"
#include "crow.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <memory>

using namespace seal;
using namespace std;

// Base64 Helper Functions
static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

std::string base64_decode(std::string const& encoded_string) {
  int in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::string ret;

  while (in_len-- && ( encoded_string[in_] != '=') && (isalnum(encoded_string[in_]) || (encoded_string[in_] == '+') || (encoded_string[in_] == '/'))) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = base64_chars.find(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret += char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
  }

  return ret;
}

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i) {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while((i++ < 3))
      ret += '=';
  }

  return ret;
}

// CORS Middleware
struct CORSHandler {
    struct context {};

    void before_handle(crow::request& req, crow::response& res, context& ctx) {
        // No-op
    }

    void after_handle(crow::request& req, crow::response& res, context& ctx) {
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
    }
};

// Global state for server-side key management
static std::unique_ptr<SEALContext> global_context;
static std::unique_ptr<KeyGenerator> global_keygen;
static std::unique_ptr<SecretKey> global_secret_key;
static std::unique_ptr<PublicKey> global_public_key;
static std::unique_ptr<Evaluator> global_evaluator;
static std::unique_ptr<CKKSEncoder> global_encoder;
static std::unique_ptr<Decryptor> global_decryptor;
static std::unique_ptr<Ciphertext> accumulated_tally;
static int submission_count = 0;
static double scale_global = pow(2.0, 40);

void initialize_seal() {
    std::cout << "🔑 Initializing SEAL with server-side key generation..." << std::endl;
    
    // SEAL Context Setup
    EncryptionParameters parms(scheme_type::ckks);
    size_t poly_modulus_degree = 32768;
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, {60, 60, 60, 60, 60, 60, 60}));
    
    global_context = std::make_unique<SEALContext>(parms);
    
    // Generate Keys
    global_keygen = std::make_unique<KeyGenerator>(*global_context);
    global_secret_key = std::make_unique<SecretKey>(global_keygen->secret_key());
    global_public_key = std::make_unique<PublicKey>();
    global_keygen->create_public_key(*global_public_key);
    
    // Initialize tools
    global_evaluator = std::make_unique<Evaluator>(*global_context);
    global_encoder = std::make_unique<CKKSEncoder>(*global_context);
    global_decryptor = std::make_unique<Decryptor>(*global_context, *global_secret_key);
    
    std::cout << "✅ Keys generated successfully. Secret Key is stored server-side." << std::endl;
}

int main() {
    crow::App<CORSHandler> app;

    // Initialize SEAL on startup
    initialize_seal();

    // GET /api/keys - Returns Public Key only
    CROW_ROUTE(app, "/api/keys").methods(crow::HTTPMethod::GET)([]() {
        std::stringstream ss;
        global_public_key->save(ss, compr_mode_type::none);
        std::string pk_str = ss.str();
        std::string pk_b64 = base64_encode(reinterpret_cast<const unsigned char*>(pk_str.c_str()), pk_str.length());
        
        crow::json::wvalue response;
        response["publicKey"] = pk_b64;
        return crow::response(response);
    });

    // Handle OPTIONS for /api/keys
    CROW_ROUTE(app, "/api/keys").methods(crow::HTTPMethod::OPTIONS)([]() {
        return crow::response(204);
    });

    // POST /api/submit - Accepts encrypted value, accumulates
    CROW_ROUTE(app, "/api/submit").methods(crow::HTTPMethod::POST)([](const crow::request& req) {
        auto x = crow::json::load(req.body);
        if (!x) return crow::response(400, "Invalid JSON");

        std::string cipher_b64 = x["cipher"].s();
        std::string cipher_str = base64_decode(cipher_b64);

        Ciphertext new_cipher;
        std::stringstream ss(cipher_str);
        
        try {
            new_cipher.load(*global_context, ss);
            
            // Initialize accumulator on first submission
            if (!accumulated_tally) {
                accumulated_tally = std::make_unique<Ciphertext>(new_cipher);
                submission_count = 1;
            } else {
                global_evaluator->add_inplace(*accumulated_tally, new_cipher);
                submission_count++;
            }
            
            crow::json::wvalue response;
            response["status"] = "accepted";
            response["count"] = submission_count;
            
            std::cout << "📥 Submission #" << submission_count << " accepted" << std::endl;
            
            return crow::response(response);
        } catch (std::exception& e) {
            return crow::response(500, e.what());
        }
    });

    // Handle OPTIONS for /api/submit
    CROW_ROUTE(app, "/api/submit").methods(crow::HTTPMethod::OPTIONS)([]() {
        return crow::response(204);
    });

    // POST /api/tally - Decrypts and returns final sum
    CROW_ROUTE(app, "/api/tally").methods(crow::HTTPMethod::POST)([]() {
        if (!accumulated_tally) {
            return crow::response(400, "No submissions yet");
        }
        
        try {
            Plaintext result_plain;
            global_decryptor->decrypt(*accumulated_tally, result_plain);
            
            std::vector<double> values;
            global_encoder->decode(result_plain, values);
            
            crow::json::wvalue response;
            response["sum"] = values[0];  // First slot contains the sum
            response["count"] = submission_count;
            
            std::cout << "🔓 Tally decrypted: " << values[0] << " (from " << submission_count << " submissions)" << std::endl;
            
            return crow::response(response);
        } catch (std::exception& e) {
            return crow::response(500, e.what());
        }
    });

    // Handle OPTIONS for /api/tally
    CROW_ROUTE(app, "/api/tally").methods(crow::HTTPMethod::OPTIONS)([]() {
        return crow::response(204);
    });

    // POST /api/reset - Resets the accumulator
    CROW_ROUTE(app, "/api/reset").methods(crow::HTTPMethod::POST)([]() {
        accumulated_tally.reset();
        submission_count = 0;
        
        std::cout << "🔄 Accumulator reset" << std::endl;
        
        crow::json::wvalue response;
        response["status"] = "reset";
        return crow::response(response);
    });

    // Handle OPTIONS for /api/reset
    CROW_ROUTE(app, "/api/reset").methods(crow::HTTPMethod::OPTIONS)([]() {
        return crow::response(204);
    });

    std::cout << "🚀 Server starting on port 8080..." << std::endl;
    app.port(8080).multithreaded().run();
}
