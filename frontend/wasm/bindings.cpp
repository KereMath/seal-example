#include <emscripten/bind.h>
#include "seal/seal.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

using namespace emscripten;
using namespace seal;
using namespace std;

// Base64 encoding/decoding
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

class SEALWrapper {
private:
    std::unique_ptr<SEALContext> context;
    std::unique_ptr<PublicKey> publicKey;
    std::unique_ptr<Encryptor> encryptor;
    std::unique_ptr<CKKSEncoder> encoder;
    double scale;

public:
    // Constructor now accepts Public Key from server
    SEALWrapper(std::string pk_base64) {
        try {
            std::cout << "Initializing SEALWrapper with server-provided Public Key..." << std::endl;
            
            // Setup SEAL context (same parameters as server)
            EncryptionParameters parms(scheme_type::ckks);
            size_t poly_modulus_degree = 32768;
            parms.set_poly_modulus_degree(poly_modulus_degree);
            parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, {60, 60, 60, 60, 60, 60, 60}));
            
            context = std::make_unique<SEALContext>(parms);
            scale = pow(2.0, 40);
            
            // Deserialize Public Key from server
            std::string pk_str = base64_decode(pk_base64);
            std::stringstream ss_pk(pk_str);
            publicKey = std::make_unique<PublicKey>();
            publicKey->load(*context, ss_pk);
            
            // Initialize encryption tools
            encryptor = std::make_unique<Encryptor>(*context, *publicKey);
            encoder = std::make_unique<CKKSEncoder>(*context);
            
            std::cout << "SEALWrapper initialized successfully (encryption-only mode)." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error in SEALWrapper constructor: " << e.what() << std::endl;
            throw;
        }
    }

    // Encrypt a number and return Base64-encoded ciphertext
    std::string encrypt_number(double value) {
        try {
            Plaintext plain;
            encoder->encode(value, scale, plain);
            
            Ciphertext encrypted;
            encryptor->encrypt(plain, encrypted);
            
            std::stringstream ss;
            encrypted.save(ss, compr_mode_type::none);
            std::string cipher_str = ss.str();
            
            return base64_encode(reinterpret_cast<const unsigned char*>(cipher_str.c_str()), cipher_str.length());
        } catch (const std::exception& e) {
            std::cerr << "Error encrypting: " << e.what() << std::endl;
            return "";
        }
    }

    std::string get_context_info() const {
        return "SEAL Context: CKKS, Degree: 32768 (Client can only encrypt)";
    }
};

EMSCRIPTEN_BINDINGS(seal_module) {
    class_<SEALWrapper>("SEALWrapper")
        .constructor<std::string>()
        .function("encrypt_number", &SEALWrapper::encrypt_number)
        .function("get_context_info", &SEALWrapper::get_context_info);
}
