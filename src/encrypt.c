#include "ece/keys.h"

#include <stdlib.h>
#include <string.h>

#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#define ECE_AES128GCM_PAD_SIZE 1
#define ECE_AESGCM_PAD_SIZE 2

typedef size_t (*min_block_pad_length_t)(size_t padLen, size_t dataPerBlock);

typedef int (*encrypt_block_t)(EVP_CIPHER_CTX* ctx, const uint8_t* key,
                               const uint8_t* iv, const uint8_t* plaintext,
                               size_t plaintextLen, uint8_t* pad, size_t padLen,
                               bool lastRecord, uint8_t* record);

typedef bool (*needs_trailer_t)(uint32_t rs, size_t ciphertextLen);

// Writes an unsigned 32-bit integer in network byte order.
static inline void
ece_write_uint32_be(uint8_t* bytes, uint32_t value) {
  bytes[0] = (value >> 24) & 0xff;
  bytes[1] = (value >> 16) & 0xff;
  bytes[2] = (value >> 8) & 0xff;
  bytes[3] = value & 0xff;
}

// Writes an unsigned 16-bit integer in network byte order.
static inline void
ece_write_uint16_be(uint8_t* bytes, uint16_t value) {
  bytes[0] = (value >> 8) & 0xff;
  bytes[1] = value & 0xff;
}

// Calculates the padding so that the block contains at least one plaintext
// byte.
static inline size_t
ece_min_block_pad_length(size_t padLen, size_t dataPerBlock) {
  size_t blockPadLen = dataPerBlock - 1;
  if (padLen && !blockPadLen) {
    // If `dataPerBlock` is 1, we can only include 1 byte of data, so write
    // the padding first.
    blockPadLen++;
  }
  return blockPadLen > padLen ? padLen : blockPadLen;
}

// Calculates the padding for an "aesgcm" block. We still want one plaintext
// byte per block, but the padding length must fit into a `uint16_t`.
static size_t
ece_aesgcm_min_block_pad_length(size_t padLen, size_t dataPerBlock) {
  size_t blockPadLen = ece_min_block_pad_length(padLen, dataPerBlock);
  return blockPadLen > UINT16_MAX ? UINT16_MAX : blockPadLen;
}

// Calculates the maximum length of an encrypted ciphertext. This does not
// account for the "aes128gcm" header length.
static inline size_t
ece_ciphertext_max_length(uint32_t rs, size_t padSize, size_t padLen,
                          size_t plaintextLen) {
  // The per-record overhead for the padding delimiter and authentication tag.
  size_t overhead = padSize + ECE_TAG_LENGTH;
  if (rs <= overhead) {
    return 0;
  }
  // The total length of the data to encrypt, including the plaintext and
  // padding.
  size_t dataLen = plaintextLen + padLen;
  // The maximum length of data to include in each record, excluding the
  // padding delimiter and authentication tag.
  size_t dataPerBlock = rs - overhead;
  // The total number of encrypted records.
  size_t numRecords = (dataLen / dataPerBlock) + 1;
  return dataLen + (overhead * numRecords);
}

// Indicates if an "aesgcm" ciphertext is a multiple of the record size, and
// needs a padding-only trailing block to prevent truncation attacks.
static bool
ece_aesgcm_needs_trailer(uint32_t rs, size_t ciphertextLen) {
  return !(ciphertextLen % rs);
}

// "aes128gcm" uses a padding scheme that doesn't need a trailer.
static bool
ece_aes128gcm_needs_trailer() {
  return false;
}

// Encrypts an "aes128gcm" block into `record`.
static int
ece_aes128gcm_encrypt_block(EVP_CIPHER_CTX* ctx, const uint8_t* key,
                            const uint8_t* iv, const uint8_t* plaintext,
                            size_t plaintextLen, uint8_t* pad, size_t padLen,
                            bool lastRecord, uint8_t* record) {
  if (EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, key, iv) <= 0) {
    return ECE_ERROR_ENCRYPT;
  }

  int chunkLen = -1;
  size_t offset = 0;

  // The plaintext block precedes the padding.
  if (EVP_EncryptUpdate(ctx, record, &chunkLen, plaintext,
                        (int) plaintextLen) <= 0) {
    return ECE_ERROR_ENCRYPT;
  }
  offset += plaintextLen;

  // The padding block comprises the delimiter, followed by zeros up to the end
  // of the block.
  pad[0] = lastRecord ? 2 : 1;
  if (EVP_EncryptUpdate(ctx, &record[offset], &chunkLen, pad,
                        (int) (padLen + ECE_AES128GCM_PAD_SIZE)) <= 0) {
    return ECE_ERROR_ENCRYPT;
  }
  offset += padLen + ECE_AES128GCM_PAD_SIZE;

  // OpenSSL requires us to finalize the encryption, but, since we're using a
  // stream cipher, finalization shouldn't write out any bytes.
  if (EVP_EncryptFinal_ex(ctx, NULL, &chunkLen) <= 0) {
    return ECE_ERROR_ENCRYPT;
  }

  // Append the authentication tag.
  if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, ECE_TAG_LENGTH,
                          &record[offset]) <= 0) {
    return ECE_ERROR_ENCRYPT;
  }

  EVP_CIPHER_CTX_reset(ctx);
  return ECE_OK;
}

// Encrypts an "aesgcm" block into `record`.
static int
ece_aesgcm_encrypt_block(EVP_CIPHER_CTX* ctx, const uint8_t* key,
                         const uint8_t* iv, const uint8_t* plaintext,
                         size_t plaintextLen, uint8_t* pad, size_t padLen,
                         bool lastRecord, uint8_t* record) {
  ECE_UNUSED(lastRecord);

  if (EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, key, iv) <= 0) {
    return ECE_ERROR_ENCRYPT;
  }

  int chunkLen = -1;
  size_t offset = 0;

  // The padding block comprises the padding length as a 16-bit integer,
  // followed by that many zeros. We checked that the length fits into a
  // `uint16_t` in `ece_aesgcm_min_block_pad_length`, so this cast is safe.
  ece_write_uint16_be(pad, (uint16_t) padLen);
  if (EVP_EncryptUpdate(ctx, record, &chunkLen, pad,
                        (int) (padLen + ECE_AESGCM_PAD_SIZE)) <= 0) {
    return ECE_ERROR_ENCRYPT;
  }
  offset += padLen + ECE_AESGCM_PAD_SIZE;

  // The plaintext block follows the padding.
  if (EVP_EncryptUpdate(ctx, &record[offset], &chunkLen, plaintext,
                        (int) plaintextLen) <= 0) {
    return ECE_ERROR_ENCRYPT;
  }
  offset += plaintextLen;

  if (EVP_EncryptFinal_ex(ctx, NULL, &chunkLen) <= 0) {
    return ECE_ERROR_ENCRYPT;
  }

  if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, ECE_TAG_LENGTH,
                          &record[offset]) <= 0) {
    return ECE_ERROR_ENCRYPT;
  }

  EVP_CIPHER_CTX_reset(ctx);
  return ECE_OK;
}

// A generic encryption function shared by "aesgcm" and "aes128gcm".
// `deriveKeyAndNonce`, `minBlockPadLen`, `encryptBlock`, and `needsTrailer`
// change depending on the scheme.
static int
ece_webpush_encrypt_plaintext(
  EC_KEY* senderPrivKey, EC_KEY* recvPubKey, const uint8_t* authSecret,
  size_t authSecretLen, const uint8_t* salt, size_t saltLen, uint32_t rs,
  size_t padSize, size_t padLen, const uint8_t* plaintext, size_t plaintextLen,
  derive_key_and_nonce_t deriveKeyAndNonce,
  min_block_pad_length_t minBlockPadLen, encrypt_block_t encryptBlock,
  needs_trailer_t needsTrailer, uint8_t* ciphertext, size_t* ciphertextLen) {

  int err = ECE_OK;

  EVP_CIPHER_CTX* ctx = NULL;
  uint8_t* pad = NULL;

  if (authSecretLen != ECE_WEBPUSH_AUTH_SECRET_LENGTH) {
    err = ECE_ERROR_INVALID_AUTH_SECRET;
    goto end;
  }
  if (saltLen != ECE_SALT_LENGTH) {
    err = ECE_ERROR_INVALID_SALT;
    goto end;
  }
  if (!plaintextLen) {
    err = ECE_ERROR_ZERO_PLAINTEXT;
    goto end;
  }

  // Make sure the ciphertext buffer is large enough to hold the header and
  // ciphertext.
  size_t maxCiphertextLen =
    ece_ciphertext_max_length(rs, padSize, padLen, plaintextLen);
  if (!maxCiphertextLen) {
    err = ECE_ERROR_INVALID_RS;
    goto end;
  }
  if (*ciphertextLen < maxCiphertextLen) {
    err = ECE_ERROR_OUT_OF_MEMORY;
    goto end;
  }

  ctx = EVP_CIPHER_CTX_new();
  if (!ctx) {
    err = ECE_ERROR_OUT_OF_MEMORY;
    goto end;
  }

  // Allocate enough memory to hold the padding and padding delimiter.
  pad = calloc(padSize + padLen, sizeof(uint8_t));
  if (!pad) {
    err = ECE_ERROR_OUT_OF_MEMORY;
    goto end;
  }

  uint8_t key[ECE_AES_KEY_LENGTH];
  uint8_t nonce[ECE_NONCE_LENGTH];
  err = deriveKeyAndNonce(ECE_MODE_ENCRYPT, senderPrivKey, recvPubKey,
                          authSecret, ECE_WEBPUSH_AUTH_SECRET_LENGTH, salt,
                          ECE_SALT_LENGTH, key, nonce);
  if (err) {
    goto end;
  }

  size_t overhead = padSize + ECE_TAG_LENGTH;

  // The maximum amount of plaintext and padding that will fit into a block.
  // The last block can be smaller.
  size_t dataPerBlock = rs - overhead;

  // The offset at which to start reading the plaintext.
  size_t plaintextStart = 0;

  // The offset at which to start writing the ciphertext.
  size_t ciphertextStart = 0;

  // The record sequence number, used to generate the IV.
  size_t counter = 0;

  bool lastRecord = false;
  while (!lastRecord) {
    size_t blockPadLen = minBlockPadLen(padLen, dataPerBlock);
    padLen -= blockPadLen;

    // Fill the rest of the block with plaintext.
    size_t plaintextEnd = plaintextStart + dataPerBlock - blockPadLen;
    bool plaintextExhausted = false;
    if (plaintextEnd >= plaintextLen) {
      plaintextEnd = plaintextLen;
      plaintextExhausted = true;
    }
    size_t blockPlaintextLen = plaintextEnd - plaintextStart;

    // The full length of the plaintext and padding.
    size_t blockLen = blockPlaintextLen + blockPadLen;

    size_t ciphertextEnd = ciphertextStart + blockLen + overhead;
    if (ciphertextEnd > maxCiphertextLen) {
      ciphertextEnd = maxCiphertextLen;
    }

    // We've reached the last record when the padding and plaintext are
    // exhausted, and we don't need to write an empty trailing block.
    lastRecord =
      !padLen && plaintextExhausted && !needsTrailer(rs, ciphertextEnd);

    if (!lastRecord && blockLen < dataPerBlock) {
      // We have padding left, but not enough plaintext to form a full record.
      // Writing trailing padding-only records will still leak size information,
      // so we force the caller to pick a smaller padding length.
      err = ECE_ERROR_ENCRYPT_PADDING;
      goto end;
    }

    // Generate the IV for this record using the nonce.
    uint8_t iv[ECE_NONCE_LENGTH];
    ece_generate_iv(nonce, counter, iv);

    // Encrypt and pad the block.
    err =
      encryptBlock(ctx, key, iv, &plaintext[plaintextStart], blockPlaintextLen,
                   pad, blockPadLen, lastRecord, &ciphertext[ciphertextStart]);
    if (err) {
      goto end;
    }

    plaintextStart = plaintextEnd;
    ciphertextStart = ciphertextEnd;
    counter++;
  }

  // Finally, set the actual ciphertext length.
  *ciphertextLen = ciphertextStart;

end:
  EVP_CIPHER_CTX_free(ctx);
  free(pad);
  return err;
}

// Encrypts a Web Push message using the "aes128gcm" scheme.
static int
ece_webpush_aes128gcm_encrypt_plaintext(
  EC_KEY* senderPrivKey, EC_KEY* recvPubKey, const uint8_t* authSecret,
  size_t authSecretLen, const uint8_t* salt, size_t saltLen, uint32_t rs,
  size_t padLen, const uint8_t* plaintext, size_t plaintextLen,
  uint8_t* payload, size_t* payloadLen) {

  size_t headerLen =
    ECE_AES128GCM_HEADER_LENGTH + ECE_WEBPUSH_PUBLIC_KEY_LENGTH;
  if (*payloadLen < headerLen) {
    return ECE_ERROR_OUT_OF_MEMORY;
  }

  // Write the header.
  memcpy(payload, salt, ECE_SALT_LENGTH);
  ece_write_uint32_be(&payload[ECE_SALT_LENGTH], rs);
  payload[ECE_SALT_LENGTH + 4] = ECE_WEBPUSH_PUBLIC_KEY_LENGTH;
  if (!EC_POINT_point2oct(
        EC_KEY_get0_group(senderPrivKey), EC_KEY_get0_public_key(senderPrivKey),
        POINT_CONVERSION_UNCOMPRESSED, &payload[ECE_AES128GCM_HEADER_LENGTH],
        ECE_WEBPUSH_PUBLIC_KEY_LENGTH, NULL)) {
    return ECE_ERROR_ENCODE_PUBLIC_KEY;
  }

  // Write the ciphertext.
  size_t ciphertextLen = *payloadLen - headerLen;
  int err = ece_webpush_encrypt_plaintext(
    senderPrivKey, recvPubKey, authSecret, authSecretLen, salt, saltLen, rs,
    ECE_AES128GCM_PAD_SIZE, padLen, plaintext, plaintextLen,
    &ece_webpush_aes128gcm_derive_key_and_nonce, &ece_min_block_pad_length,
    &ece_aes128gcm_encrypt_block, &ece_aes128gcm_needs_trailer,
    &payload[headerLen], &ciphertextLen);
  if (err) {
    return err;
  }

  *payloadLen = headerLen + ciphertextLen;
  return ECE_OK;
}

size_t
ece_aes128gcm_payload_max_length(uint32_t rs, size_t padLen,
                                 size_t plaintextLen) {
  size_t ciphertextLen =
    ece_ciphertext_max_length(rs, ECE_AES128GCM_PAD_SIZE, padLen, plaintextLen);
  if (!ciphertextLen) {
    return 0;
  }
  return ECE_AES128GCM_HEADER_LENGTH + ECE_AES128GCM_MAX_KEY_ID_LENGTH +
         ciphertextLen;
}

int
ece_aes128gcm_encrypt(const uint8_t* rawRecvPubKey, size_t rawRecvPubKeyLen,
                      const uint8_t* authSecret, size_t authSecretLen,
                      uint32_t rs, size_t padLen, const uint8_t* plaintext,
                      size_t plaintextLen, uint8_t* payload,
                      size_t* payloadLen) {
  int err = ECE_OK;

  EC_KEY* recvPubKey = NULL;
  EC_KEY* senderPrivKey = NULL;

  // Generate a random salt.
  uint8_t salt[ECE_SALT_LENGTH];
  if (RAND_bytes(salt, ECE_SALT_LENGTH) <= 0) {
    err = ECE_ERROR_INVALID_SALT;
    goto end;
  }

  // Import the receiver public key.
  recvPubKey = ece_import_public_key(rawRecvPubKey, rawRecvPubKeyLen);
  if (!recvPubKey) {
    err = ECE_ERROR_INVALID_PUBLIC_KEY;
    goto end;
  }

  // Generate the sender ECDH key pair.
  senderPrivKey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
  if (!senderPrivKey) {
    err = ECE_ERROR_OUT_OF_MEMORY;
    goto end;
  }
  if (EC_KEY_generate_key(senderPrivKey) <= 0) {
    err = ECE_ERROR_INVALID_PRIVATE_KEY;
    goto end;
  }

  // Encrypt the message.
  err = ece_webpush_aes128gcm_encrypt_plaintext(
    senderPrivKey, recvPubKey, authSecret, authSecretLen, salt, ECE_SALT_LENGTH,
    rs, padLen, plaintext, plaintextLen, payload, payloadLen);

end:
  EC_KEY_free(recvPubKey);
  EC_KEY_free(senderPrivKey);
  return err;
}

int
ece_aes128gcm_encrypt_with_keys(
  const uint8_t* rawSenderPrivKey, size_t rawSenderPrivKeyLen,
  const uint8_t* authSecret, size_t authSecretLen, const uint8_t* salt,
  size_t saltLen, const uint8_t* rawRecvPubKey, size_t rawRecvPubKeyLen,
  uint32_t rs, size_t padLen, const uint8_t* plaintext, size_t plaintextLen,
  uint8_t* payload, size_t* payloadLen) {

  int err = ECE_OK;

  EC_KEY* senderPrivKey = NULL;
  EC_KEY* recvPubKey = NULL;

  senderPrivKey = ece_import_private_key(rawSenderPrivKey, rawSenderPrivKeyLen);
  if (!senderPrivKey) {
    err = ECE_ERROR_INVALID_PRIVATE_KEY;
    goto end;
  }
  recvPubKey = ece_import_public_key(rawRecvPubKey, rawRecvPubKeyLen);
  if (!recvPubKey) {
    err = ECE_ERROR_INVALID_PUBLIC_KEY;
    goto end;
  }

  err = ece_webpush_aes128gcm_encrypt_plaintext(
    senderPrivKey, recvPubKey, authSecret, authSecretLen, salt, saltLen, rs,
    padLen, plaintext, plaintextLen, payload, payloadLen);

end:
  EC_KEY_free(senderPrivKey);
  EC_KEY_free(recvPubKey);
  return err;
}

size_t
ece_aesgcm_ciphertext_max_length(uint32_t rs, size_t padLen,
                                 size_t plaintextLen) {
  return ece_ciphertext_max_length(rs, ECE_AESGCM_PAD_SIZE, padLen,
                                   plaintextLen);
}

int
ece_webpush_aesgcm_encrypt_with_keys(
  const uint8_t* rawSenderPrivKey, size_t rawSenderPrivKeyLen,
  const uint8_t* authSecret, size_t authSecretLen, const uint8_t* salt,
  size_t saltLen, const uint8_t* rawRecvPubKey, size_t rawRecvPubKeyLen,
  uint32_t rs, size_t padLen, const uint8_t* plaintext, size_t plaintextLen,
  uint8_t* ciphertext, size_t* ciphertextLen) {

  int err = ECE_OK;

  EC_KEY* senderPrivKey = NULL;
  EC_KEY* recvPubKey = NULL;

  senderPrivKey = ece_import_private_key(rawSenderPrivKey, rawSenderPrivKeyLen);
  if (!senderPrivKey) {
    err = ECE_ERROR_INVALID_PRIVATE_KEY;
    goto end;
  }
  recvPubKey = ece_import_public_key(rawRecvPubKey, rawRecvPubKeyLen);
  if (!recvPubKey) {
    err = ECE_ERROR_INVALID_PUBLIC_KEY;
    goto end;
  }

  rs += ECE_TAG_LENGTH;
  err = ece_webpush_encrypt_plaintext(
    senderPrivKey, recvPubKey, authSecret, authSecretLen, salt, saltLen, rs,
    ECE_AESGCM_PAD_SIZE, padLen, plaintext, plaintextLen,
    &ece_webpush_aesgcm_derive_key_and_nonce, &ece_aesgcm_min_block_pad_length,
    &ece_aesgcm_encrypt_block, &ece_aesgcm_needs_trailer, ciphertext,
    ciphertextLen);

end:
  EC_KEY_free(senderPrivKey);
  EC_KEY_free(recvPubKey);
  return err;
}
