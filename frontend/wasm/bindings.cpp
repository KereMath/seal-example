#include "seal/seal.h"
#include <emscripten/bind.h>
#include <string>
#include <vector>
#include <sstream>

using namespace seal;
using namespace emscripten;
using namespace std;

// Base64 Helper Functions
static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

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

// Wrapper class to manage SEAL context and operations
class SEALWrapper {
public:
    SEALWrapper() {
        try {
            std::cout << "Initializing SEALWrapper with CKKS..." << std::endl;
            EncryptionParameters parms(scheme_type::ckks);
            size_t poly_modulus_degree = 32768;
            parms.set_poly_modulus_degree(poly_modulus_degree);
            parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, {60, 60, 60, 60, 60, 60, 60}));
            
            context = std::make_shared<SEALContext>(parms);
            
            // Verify context
            if (!context->parameters_set()) {
                std::cerr << "SEAL Context parameter validation failed: " 
                          << context->key_context_data()->qualifiers().parameter_error_message() << std::endl;
                throw std::runtime_error("SEAL Context creation failed");
            }

            keygen = std::make_shared<KeyGenerator>(*context);
            secret_key = keygen->secret_key();
            keygen->create_public_key(public_key);
            
            encryptor = std::make_shared<Encryptor>(*context, public_key);
            decryptor = std::make_shared<Decryptor>(*context, secret_key);
            encoder = std::make_shared<CKKSEncoder>(*context);
            
            scale = pow(2.0, 40);
            
            std::cout << "SEALWrapper initialized successfully." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Exception in SEALWrapper constructor: " << e.what() << std::endl;
            throw;
        } catch (...) {
            std::cerr << "Unknown exception in SEALWrapper constructor" << std::endl;
            throw;
        }
    }

    std::string encrypt_number(int number) {
        try {
            Plaintext plain;
            std::vector<double> input = { (double)number };
            encoder->encode(input, scale, plain);
            
            Ciphertext encrypted;
            encryptor->encrypt(plain, encrypted);
            
            std::stringstream ss;
            encrypted.save(ss, compr_mode_type::none); // Disable compression
            std::string binary = ss.str();
            return base64_encode(reinterpret_cast<const unsigned char*>(binary.c_str()), binary.length());
        } catch (const std::exception& e) {
            std::cerr << "Exception in encrypt_number: " << e.what() << std::endl;
            throw;
        }
    }

    int decrypt_number(std::string encrypted_b64) {
        try {
            std::string binary = base64_decode(encrypted_b64);
            
            Ciphertext encrypted;
            std::stringstream ss(binary);
            encrypted.load(*context, ss);
            
            Plaintext plain;
            decryptor->decrypt(encrypted, plain);
            
            std::vector<double> result;
            encoder->decode(plain, result);
            
            if (result.empty()) return 0;
            return (int)round(result[0]);
        } catch (const std::exception& e) {
            std::cerr << "Exception in decrypt_number: " << e.what() << std::endl;
            throw;
        }
    }
    
    std::string get_context_info() {
        return "SEAL Context Initialized (WASM). Scheme: CKKS, Degree: 32768";
    }

private:
    std::shared_ptr<SEALContext> context;
    std::shared_ptr<KeyGenerator> keygen;
    SecretKey secret_key;
    PublicKey public_key;
    std::shared_ptr<Encryptor> encryptor;
    std::shared_ptr<Decryptor> decryptor;
    std::shared_ptr<CKKSEncoder> encoder;
    double scale;
};

EMSCRIPTEN_BINDINGS(my_module) {
    class_<SEALWrapper>("SEALWrapper")
        .constructor<>()
        .function("encrypt_number", &SEALWrapper::encrypt_number)
        .function("decrypt_number", &SEALWrapper::decrypt_number)
        .function("get_context_info", &SEALWrapper::get_context_info);
}
