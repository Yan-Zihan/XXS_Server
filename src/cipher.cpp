#include <cmath>
#include <algorithm>
#include <execution>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <cstring>
#include "core/cipher.h"

namespace cipher {

ustring fromString(std::string_view s) {
    ustring res;
    res.resize(s.length());
    std::transform(std::execution::par_unseq,
        s.begin(), s.end(), res.begin(),
        [](const char &x){
            return (unsigned char)x;
        }
    );
    return res;
}

std::string encodeBase64(ustring_view s) {
    BIO *b64, *bio;
    BUF_MEM *buff;
    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    BIO_write(bio, s.data(), s.size());
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &buff);
    std::string res{buff->data, buff->length};
    BIO_free_all(bio);
    return res;
}

inline int calcDecodeLength(std::string_view input) { //Calculates the length of a decoded base64 string
    int len = input.size();
    int padding = 0;
    if (input[len-1] == '=' && input[len-2] == '=') //last two chars are =
        padding = 2;
    else if (input[len-1] == '=') //last char is =
        padding = 1;
    return (int)len*0.75 - padding;
}

ustring decodeBase64(std::string_view s) {
    BIO *bio, *b64;
    auto buff = (unsigned char *)malloc(calcDecodeLength(s));
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_mem_buf(s.data(), s.size());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Do not use newlines to flush buffer
    auto len = BIO_read(bio, buff, s.size());
    ustring res{buff, (size_t)len};
    BIO_free_all(bio);
    return res;
}

ustring encryptAES(
    std::string_view input,
    ustring_view key)
{
    static unsigned char iv[] = {
        0x7,0x31,0x3c,0xdd,
        0x14,0xa2,0xf7,0xde,
        0xb9,0x4d,0xc2,0xde,
        0xd8,0xc5,0x1b,0x37};
    ustring input_ = fromString(input);
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx,
        EVP_aes_128_cbc(),
        0, key.data(), iv
    );
    int c_len = input.length()+AES_BLOCK_SIZE;
    int f_len{0};
    auto cipher = (unsigned char *)malloc(c_len);
    EVP_EncryptUpdate(ctx, cipher, &c_len, input_.data(), input_.length());
    EVP_EncryptFinal(ctx, cipher+c_len, &f_len);
    EVP_CIPHER_CTX_free(ctx);
    return ustring(cipher, c_len+f_len);
}

std::string decryptAES(
    ustring_view input,
    ustring_view key)
{
    static unsigned char iv[] = {
        0x7,0x31,0x3c,0xdd,
        0x14,0xa2,0xf7,0xde,
        0xb9,0x4d,0xc2,0xde,
        0xd8,0xc5,0x1b,0x37};
    int p_len = input.length(), f_len = 0;
    auto plaintext = (unsigned char *)malloc(p_len);
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx,
        EVP_aes_128_cbc(),
        0, key.data(), iv
    );
    EVP_DecryptUpdate(ctx, plaintext, &p_len, input.data(), input.length());
    EVP_DecryptFinal_ex(ctx, plaintext+p_len, &f_len);
    std::string res(p_len+f_len, 0);
    for(int i = 0; i < res.length(); i++)
        res[i] = plaintext[i];
    EVP_CIPHER_CTX_free(ctx);
    return res;
}

}