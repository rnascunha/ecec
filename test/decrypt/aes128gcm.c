#include "test.h"

#include <string.h>

typedef struct webpush_aes128gcm_decrypt_ok_test_s {
  const char* desc;
  const char* plaintext;
  const char* recvPrivKey;
  const char* authSecret;
  const char* payload;
  size_t payloadLen;
  size_t maxPlaintextLen;
  size_t plaintextLen;
} webpush_aes128gcm_decrypt_ok_test_t;

static webpush_aes128gcm_decrypt_ok_test_t
  webpush_aes128gcm_decrypt_ok_tests[] = {
    {
      .desc = "rs = 24",
      .plaintext = "I am the walrus",
      .recvPrivKey = "\xc8\x99\xd1\x1d\x32\xe2\xb7\xe6\xfe\x74\x98\x78\x6f\x50"
                     "\xf2\x3b\x98\xac\xe5\x39\x7a\xd2\x61\xde\x39\xba\x64\x49"
                     "\xec\xc1\x2c\xad",
      .authSecret =
        "\x99\x6f\xad\x8b\x50\xaa\x2d\x02\xb8\x3f\x26\x41\x2b\x2e\x2a\xee",
      .payload = "\x49\x5c\xe6\xc8\xde\x93\xa4\x53\x9e\x86\x2e\x86\x34\x99\x3c"
                 "\xbb\x00\x00"
                 "\x00\x18\x41\x04\x3c\x33\x78\xa2\xc0\xab\x95\x4e\x14\x98\x71"
                 "\x8e\x85\xf0"
                 "\x8b\xb7\x23\xfb\x7d\x25\xe1\x35\xa6\x63\xfe\x38\x58\x84\xeb"
                 "\x81\x92\x33"
                 "\x6b\xf9\x0a\x54\xed\x72\x0f\x1c\x04\x5c\x0b\x40\x5e\x9b\xbc"
                 "\x3a\x21\x42"
                 "\xb1\x6c\x89\x08\x67\x34\xc3\x74\xeb\xaf\x70\x99\xe6\x42\x7e"
                 "\x2d\x32\xc8"
                 "\xad\xa5\x01\x87\x03\xc5\x4b\x10\xb4\x81\xe1\x02\x7d\x72\x09"
                 "\xd8\xc6\xb4"
                 "\x35\x53\xfa\x13\x3a\xfa\x59\x7f\x2d\xdc\x45\xa5\xba\x81\x40"
                 "\x94\x4e\x64"
                 "\x90\xbb\x8d\x6d\x99\xba\x1d\x02\xe6\x0d\x95\xf4\x8c\xe6\x44"
                 "\x47\x7c\x17"
                 "\x23\x1d\x95\xb9\x7a\x4f\x95\xdd",
      .payloadLen = 152,
      .maxPlaintextLen = 18,
      .plaintextLen = 15,
    },
    {
      .desc = "Example from draft-ietf-webpush-encryption-latest",
      .plaintext = "When I grow up, I want to be a watermelon",
      .recvPrivKey = "\xab\x57\x57\xa7\x0d\xd4\xa5\x3e\x55\x3a\x6b\xbf\x71\xff"
                     "\xef\xea\x28\x74\xec\x07\xa6\xb3\x79\xe3\xc4\x8f\x89\x5a"
                     "\x02\xdc\x33\xde",
      .authSecret =
        "\x05\x30\x59\x32\xa1\xc7\xea\xbe\x13\xb6\xce\xc9\xfd\xa4\x88\x82",
      .payload = "\x0c\x6b\xfa\xad\xad\x67\x95\x88\x03\x09\x2d\x45\x46\x76\xf3"
                 "\x97\x00\x00\x10\x00\x41\x04\xfe\x33\xf4\xab\x0d\xea\x71\x91"
                 "\x4d\xb5\x58\x23\xf7\x3b\x54\x94\x8f\x41\x30\x6d\x92\x07\x32"
                 "\xdb\xb9\xa5\x9a\x53\x28\x64\x82\x20\x0e\x59\x7a\x7b\x7b\xc2"
                 "\x60\xba\x1c\x22\x79\x98\x58\x09\x92\xe9\x39\x73\x00\x2f\x30"
                 "\x12\xa2\x8a\xe8\xf0\x6b\xbb\x78\xe5\xec\x0f\xf2\x97\xde\x5b"
                 "\x42\x9b\xba\x71\x53\xd3\xa4\xae\x0c\xaa\x09\x1f\xd4\x25\xf3"
                 "\xb4\xb5\x41\x4a\xdd\x8a\xb3\x7a\x19\xc1\xbb\xb0\x5c\xf5\xcb"
                 "\x5b\x2a\x2e\x05\x62\xd5\x58\x63\x56\x41\xec\x52\x81\x2c\x6c"
                 "\x8f\xf4\x2e\x95\xcc\xb8\x6b\xe7\xcd",
      .payloadLen = 144,
      .maxPlaintextLen = 42,
      .plaintextLen = 41,
    },
};

typedef struct aes128gcm_ok_decrypt_test_s {
  const char* desc;
  const char* plaintext;
  const char* ikm;
  const char* payload;
  size_t payloadLen;
  size_t maxPlaintextLen;
  size_t plaintextLen;
} aes128gcm_ok_decrypt_test_t;

typedef struct aes128gcm_err_decrypt_test_s {
  const char* desc;
  const char* ikm;
  const char* payload;
  size_t payloadLen;
  size_t maxPlaintextLen;
  int err;
} aes128gcm_err_decrypt_test_t;

static aes128gcm_ok_decrypt_test_t aes128gcm_ok_decrypt_tests[] = {
  {
    .desc = "rs = 18, pad = 8",
    .plaintext = "When I grow up, I want to be a watermelon",
    .ikm = "\x28\xc0\x66\x11\x4a\x2d\xa5\x21\xca\x89\xf4\x21\x9d\xa8\xac\xc0",
    .payload =
      "\x1f\xc2\xec\x59\x4d\xbd\xa8\xc8\xab\x26\x25\x47\x04\x65\xb8\xcd\x00\x00"
      "\x00\x12\x00\x92\x56\xfe\x1c\x43\x4f\x71\x8e\x85\x16\x3a\x0f\x52\x69\xc1"
      "\xb8\x24\x55\x73\x60\x7d\x06\x06\xc3\x97\xfc\xfd\xc3\x27\xd5\xf9\x0c\x44"
      "\x8d\x6a\x11\xa0\xc4\xb8\xd0\x51\xc8\x54\x94\xb0\x0f\xb5\xeb\xb9\xe6\x85"
      "\x38\x2f\x88\xee\x5a\xce\x19\x1b\xfa\x73\x1d\xa2\xc9\xb2\x3f\x0a\xe4\xfe"
      "\x4b\x9a\xd5\xf5\x4d\xf0\xec\xc8\x17\x9f\xc6\xdb\xed\x3a\x94\x33\xbe\x4f"
      "\x92\xa8\xdd\xf1\x0d\x5f\x29\x9f\x76\x73\xfb\x79\x33\x69\xc9\x6b\xf5\x20"
      "\x5b\x4e\xa5\x47\xef\xa3\xd4\x4b\x6c\xaa\x47\xac\x97\x9a\xa1\x69\x45\x2a"
      "\xf6\xf6\x84\x65\xda\xba\x9b\x8a\xb3\x9c\xed\x91\x15\xd4\x4f\xbb\x7c\xf6"
      "\xc6\xfa\x0f\x86\x71\xa2\xa1\x2c\xf6\x18\x18\x86\x94\xf1\x7c\x2f\x63\xb7"
      "\x46\xe0\x6e\x9a\x51\x20\x6a\x8c\x54\xc9\x91\x54\xb1\x84\xa9\xec\x8a\x29"
      "\x71\x4e\xfd\xb6\x8f\xde\xe4\xc4\x2f\x57\xb3\x2e\x48\x8d\x5c\x47\x51\x05"
      "\xd0\x57\xb6\x55\x15\xc4\xa0\xeb\x59\x5b\xd6\xe8\xa7\x11\x65\x18\xad\xfb"
      "\xc5\xdf\xbc\x51\x71\x01\xae\x72\x2b\x19\x14\xa1\x47\x3e\x35\xbb\x52\xa7"
      "\xc9\xad\x22\x09\xc6\xea\x8f\x2b\x60\x5f\x8d\xf9\x78\x65\x4e\xd5\x2c\x71"
      "\x17\x5c\x12\x4e\xb3\xa5\x6e\xfa\xfe\x64\x77\xd8\x05\x07\x4d\xd0\x29\x15"
      "\x65\x37\x4b\xb1\x02\x8c\x9b\xbd\x59\xd6\x4d\x56\x27\xe7\x28\x02\x5a\x30"
      "\x59\x10\x0f\x48\xe8\x88\x5f\x7f\xe4\x4e\x0d\xec\xbb\x7b\x98\x08\x3d\x85"
      "\xe1\xc8\x2b\x11\x8a\x8a\xf5\xb3\x8f\x33\xb3\xb6\x7b\xa6\xd5\x8e\x58\xc1"
      "\x3a\xff\xaf\x8c\x2b\x99\x9d\x4f\xc2\x09\xed\x73\x7a\x04\x74\x93\x48\x1b"
      "\xfa\xfd\x71\x9d\x49\x8d\xf0\xa9\x8c\x6f\x43\x48\xc7\x24\x3a\xa6\x78\x9a"
      "\xf3\x36\x85\x4b\x7e\x87\xf9\x5a\x05\x47\xcf\x73\x5e\xc3\x83\xa8\x27\x4d"
      "\xdc\xf5\xd9\x76\x43\x85\x37\x36\xb4\xc6\x06\x3f\x48\x95\xab\x38\x38\xc9"
      "\x99\x9c\xec\x7b\x73\x1c\xda\xcb\xd5\x0f\x8c\x06\xde\x9f\xe8\x0a\xad\xaf"
      "\x91\xd1\x1b\x9a\x35\xaa\xdf\x41\xa0\x5c\x6b\xae\xda\x0c\x6d\x00\xb4\xa8"
      "\xc0\xc3\x69\xf9\x8f\x4c\x6e\x6a\x97\x76\xab\x41\x7e\x28\x39\x1b\x47\x5c"
      "\xe7\xfc\x01\x65\xdb\xe4\x9e\xf9\x89\x1f\x9c\xef\x82\xe2\x86\x7e\xd6\xd6"
      "\x7c\x4a\x5a\x71\xda\xa1\xf7\x5d\x26\x5f\x85\x92\xa8\x1d\xb4\x8c\xbc\x92"
      "\xc3\x82\xd6\x3a\x96\xf5\x80\x0f\xa8\xec\xa9\xe2\x02\x7b\xaf\xb7\x4b\xc9"
      "\xe3\x3b\xdc\xd3\xb8\xf4\xd8\xe0\x5f\x36\xdd\xa5\x44\xf8\x97\x5e\xcb\xea"
      "\x47\x8d\xb8\x36\x61\xa1\xdb\xc5\xfc\xcb\x7f\xeb\x05\x57\xde\xd7\x3a\x37"
      "\x90\xc3\x52\x69\xfa\x59\xe4\x75\x0e\x55\xc7\x29\xa0\x08\xc9\x8c\xe9\xee"
      "\x88\x82\xe0\xc2\xae\xaf\x1e\xbe\x40\x3b\xe9\x6d\xaa\x25\xb4\x2a\xc0\x1b"
      "\x6a\xd4\x35\x5b\xc3\x60\xcd\xd1\x31\x10\xe3\xff\xc7\x6a\xb4\x51\xf5\x9e"
      "\x04\xa8\xab\x3f\x1a\x4a\x69\xdf\x21\x91\xab\x4b\x60\xfd\x31\x76\x13\x2b"
      "\x8f\x99\x1d\x3a\xb2\x96\xa0\x36\x93\x36\xb5\x35\xaa\xcc\x15\x97\x7d\x50"
      "\x5b\xe2\xc5\xd4\xb6\xb7\xbc\x55\xd8\x3c\xd1\x7c\x1e\x80\x35\x7f\x4a\x21"
      "\x8d\x72\x93\x3f\xa6\x09\x74\x73\x29\x6d\x7d\xdc\x30\xd7\xa1\x7b\x73\x23"
      "\xac\x17\x3e\xc3\x47\x72\x45\x1a\xa6\x70\x95\xca\xcf\xbc\x87\x6c\x05\x56"
      "\xa7\xae\x2f\x2c\x64\x55\x50\xd8\xab\x05\xe6\xa7\x87\xa5\x5a\x1c\x0c\xe1"
      "\x59\x7b\x95\x8e\xe7\xea\xff\x10\x29\x93\x02\xd9\x9c\x35\xca\xc9\x83\xb9"
      "\x6c\x0f\xec\xaf\x61\x59\x3a\x17\x66\xd3\xbc\xc7\xc2\xd5\x00\x4a\x4c\x43"
      "\x91\xcb\x41\xb1\x36\x79\x35\xe3\x9d\x73\xf3\xa2\xe0\x84\x59\xd2\x83\x2c"
      "\x18\x34\xf2\x60\x6a\x87\x40\x10\xd7\xcf\x17\x7e\x7b\xcf\x61\xad\x41\x3d"
      "\x0f\xfc\xe3\x3d\x6b\xef\x39\xb9\x61\x39\xda\x24\xaf\xc9\xac\xe7\x94\x28"
      "\x8d\x79\x75\xac\x74\xc9\x86\x66\x1d\x40\x34\x42\x25\xf7\x99\xf5\x96\x35"
      "\xa9\x1f\x98\x7b\x54\x42\x3a\x5e\x10\x60\x9b\x6d\x8e\xb4\xda\xe5\xc2\xc8"
      "\x2e\x53\xae\xc3\xa3\xdd\xe5\xaf\xbf\x06\x2c\x42\xe2\x95\x91\xd9\x3e\x49"
      "\xe4\x54\x80\x90\xb5\x22\x7e\x13\xda\x62\x70\x14\x7e\x5d\x9d\xee\x3f\x2e"
      "\x8d\x2f\x7d\xe0\xaf\x1d\xd7\x61\x27\x9d\xd3\xf2\x8a\xce\x13\x74\x73\x15"
      "\x11\xf2\x1a",
    .payloadLen = 903,
    .maxPlaintextLen = 82,
    .plaintextLen = 41,
  },
};

static aes128gcm_err_decrypt_test_t aes128gcm_err_decrypt_tests[] = {
  {
    .desc = "rs <= block overhead",
    .ikm = "\x2f\xb1\x75\xc2\x71\xb9\x2f\x6b\x55\xe4\xf2\xa2\x52\xd1\x45\x43",
    .payload = "\x76\xf9\x1d\x48\x4e\x84\x91\xda\x55\xc5\xf7\xbf\xe6\xd3\x3e"
               "\x89\x00\x00\x00\x02\x00",
    .payloadLen = 21,
    .maxPlaintextLen = 0,
    .err = ECE_ERROR_INVALID_RS,
  },
  {
    .desc = "Zero plaintext",
    .ikm = "\x64\xc7\x0e\x64\xa7\x25\x55\x14\x51\xf2\x08\xdf\xba\xa0\xb9\x72",
    .payload = "\xaa\xd2\x05\x7d\x33\x53\xb7\xff\x37\xbd\xe4\x2a\xe1\xd5\x0f"
               "\xda\x00\x00\x00\x20\x00\xbb\xc7\xb9\x65\x76\x0b\xf0\x66\x2b"
               "\x93\xf4\xe5\xd6\x94\xb7\x65\xf0\xcd\x15\x9b\x28\x01\xa5",
    .payloadLen = 44,
    .maxPlaintextLen = 7,
    .err = ECE_ERROR_ZERO_PLAINTEXT,
  },
  {
    .desc = "Bad early padding delimiter",
    .ikm = "\x64\xc7\x0e\x64\xa7\x25\x55\x14\x51\xf2\x08\xdf\xba\xa0\xb9\x72",
    .payload = "\xaa\xd2\x05\x7d\x33\x53\xb7\xff\x37\xbd\xe4\x2a\xe1\xd5\x0f"
               "\xda\x00\x00\x00\x20\x00\xb9\xc7\xb9\x65\x76\x0b\xf0\x9e\x42"
               "\xb1\x08\x43\x38\x75\xa3\x06\xc9\x78\x06\x0a\xfc\x7c\x7d\xe9"
               "\x52\x85\x91\x8b\x58\x02\x60\xf3\x45\x38\x7a\x28\xe5\x25\x66"
               "\x2f\x48\xc1\xc3\x32\x04\xb1\x95\xb5\x4e\x9e\x70\xd4\x0e\x3c"
               "\xf3\xef\x0c\x67\x1b\xe0\x14\x49\x7e\xdc",
    .payloadLen = 85,
    .maxPlaintextLen = 16,
    .err = ECE_ERROR_DECRYPT_PADDING,
  },
  {
    .desc = "Bad final padding delimiter",
    .ikm = "\x64\xc7\x0e\x64\xa7\x25\x55\x14\x51\xf2\x08\xdf\xba\xa0\xb9\x72",
    .payload = "\xaa\xd2\x05\x7d\x33\x53\xb7\xff\x37\xbd\xe4\x2a\xe1\xd5\x0f"
               "\xda\x00\x00\x00\x20\x00\xba\xc7\xb9\x65\x76\x0b\xf0\x9e\x42"
               "\xb1\x08\x4a\x69\xe4\x50\x1b\x8d\x49\xdb\xc6\x79\x23\x4d\x47"
               "\xc2\x57\x16",
    .payloadLen = 48,
    .maxPlaintextLen = 11,
    .err = ECE_ERROR_DECRYPT_PADDING,
  },
  {
    .desc = "Invalid auth tag",
    .ikm = "\x64\xc7\x0e\x64\xa7\x25\x55\x14\x51\xf2\x08\xdf\xba\xa0\xb9\x72",
    .payload = "\xaa\xd2\x05\x7d\x33\x53\xb7\xff\x37\xbd\xe4\x2a\xe1\xd5\x0f"
               "\xda\x00\x00\x00\x20\x00\xbb\xc6\xb1\x1d\x46\x3a\x7e\x0f\x07"
               "\x2b\xbe\xaa\x44\xe0\xd6\x2e\x4b\xe5\xf9\x5d\x25\xe3\x86\x71"
               "\xe0\x7d",
    .payloadLen = 47,
    .maxPlaintextLen = 10,
    .err = ECE_ERROR_DECRYPT,
  },
  {
    // 2 records; last record is "\x00" without a delimiter.
    .desc = "rs = 21, truncated padding for last record",
    .ikm = "\x1a\x5c\x05\x64\x16\xdf\x83\x73\x87\x51\x01\xd1\x11\x98\x47\x83",
    .payload = "\x53\x06\xdc\x45\xdd\x8e\x51\x00\x16\x53\x3c\x1e\xba\xe5\x50"
               "\x53\x00\x00\x00\x15\x00\xa7\x0d\x92\x4e\xe6\x08\xd0\xc1\xc1"
               "\x00\x88\x5a\xe8\x78\x1d\xd1\x47\x67\x02\x12\x63\xf7\x9d\x22"
               "\xa9\x44\x8d\xb2\x33\x6e\xe0\xe5\x72\xe2\x3c\x38\x49\x70",
    .payloadLen = 59,
    .maxPlaintextLen = 6,
    .err = ECE_ERROR_ZERO_PLAINTEXT,
  },
  {
    // 2 records; last record is just the auth tag.
    .desc = "rs = 21, auth tag for last record",
    .ikm = "\xc1\xc9\xc0\x91\x9d\x81\x0a\xe7\xd9\xe8\x0c\x45\xbc\x21\xa9\xfa",
    .payload = "\xc1\xaf\x29\x07\x6f\x69\x25\x60\xde\x6d\x1f\xde\x02\x11\x69"
               "\x79\x00\x00\x00\x15\x00\x46\x9f\xde\x73\xa7\x8a\x2a\x66\x1d"
               "\xb0\xf1\xae\x55\xec\xec\x86\x6a\xaa\xe5\xf3\x04\xa3\x3e\xc3"
               "\xb0\xbb\x16\xe9\x0a\xab\xc4\xba\xe0\xed\xbb\x73\x46",
    .payloadLen = 58,
    .maxPlaintextLen = 5,
    .err = ECE_ERROR_SHORT_BLOCK,
  },
};

void
test_webpush_aes128gcm_decrypt_ok(void) {
  size_t tests = sizeof(webpush_aes128gcm_decrypt_ok_tests) /
                 sizeof(webpush_aes128gcm_decrypt_ok_test_t);
  for (size_t i = 0; i < tests; i++) {
    webpush_aes128gcm_decrypt_ok_test_t t =
      webpush_aes128gcm_decrypt_ok_tests[i];

    const void* recvPrivKey = t.recvPrivKey;
    const void* authSecret = t.authSecret;
    const void* payload = t.payload;

    size_t plaintextLen =
      ece_aes128gcm_plaintext_max_length(payload, t.payloadLen);
    ece_assert(plaintextLen == t.maxPlaintextLen,
               "Got plaintext max length %zu for `%s`; want %zu", plaintextLen,
               t.desc, t.maxPlaintextLen);

    uint8_t* plaintext = calloc(plaintextLen, sizeof(uint8_t));

    int err = ece_webpush_aes128gcm_decrypt(
      recvPrivKey, ECE_WEBPUSH_PRIVATE_KEY_LENGTH, authSecret,
      ECE_WEBPUSH_AUTH_SECRET_LENGTH, payload, t.payloadLen, plaintext,
      &plaintextLen);
    ece_assert(!err, "Got %d decrypting payload for `%s`", err, t.desc);

    ece_assert(plaintextLen == t.plaintextLen,
               "Got plaintext length %zu for `%s`; want %zu", plaintextLen,
               t.desc, t.plaintextLen);
    ece_assert(!memcmp(plaintext, t.plaintext, plaintextLen),
               "Wrong plaintext for `%s`", t.desc);

    free(plaintext);
  }
}

void
test_aes128gcm_decrypt_ok(void) {
  size_t tests =
    sizeof(aes128gcm_ok_decrypt_tests) / sizeof(aes128gcm_ok_decrypt_test_t);
  for (size_t i = 0; i < tests; i++) {
    aes128gcm_ok_decrypt_test_t t = aes128gcm_ok_decrypt_tests[i];

    size_t plaintextLen = ece_aes128gcm_plaintext_max_length(
      (const uint8_t*) t.payload, t.payloadLen);
    ece_assert(plaintextLen == t.maxPlaintextLen,
               "Got plaintext max length %zu for `%s`; want %zu", plaintextLen,
               t.desc, t.maxPlaintextLen);

    uint8_t* plaintext = calloc(plaintextLen, sizeof(uint8_t));

    int err = ece_aes128gcm_decrypt((const uint8_t*) t.ikm, 16,
                                    (const uint8_t*) t.payload, t.payloadLen,
                                    plaintext, &plaintextLen);
    ece_assert(!err, "Got %d decrypting payload for `%s`", err, t.desc);

    ece_assert(plaintextLen == t.plaintextLen,
               "Got plaintext length %zu for `%s`; want %zu", plaintextLen,
               t.desc, t.plaintextLen);
    ece_assert(!memcmp(plaintext, t.plaintext, plaintextLen),
               "Wrong plaintext for `%s`", t.desc);

    free(plaintext);
  }
}

void
test_aes128gcm_decrypt_err(void) {
  size_t tests =
    sizeof(aes128gcm_err_decrypt_tests) / sizeof(aes128gcm_err_decrypt_test_t);
  for (size_t i = 0; i < tests; i++) {
    aes128gcm_err_decrypt_test_t t = aes128gcm_err_decrypt_tests[i];

    const void* ikm = t.ikm;
    const void* payload = t.payload;

    size_t plaintextLen =
      ece_aes128gcm_plaintext_max_length(payload, t.payloadLen);
    ece_assert(plaintextLen == t.maxPlaintextLen,
               "Got plaintext max length %zu for `%s`; want %zu", plaintextLen,
               t.desc, t.maxPlaintextLen);

    uint8_t* plaintext = calloc(plaintextLen, sizeof(uint8_t));

    int err = ece_aes128gcm_decrypt(ikm, 16, payload, t.payloadLen, plaintext,
                                    &plaintextLen);
    ece_assert(err == t.err, "Got %d decrypting payload for `%s`; want %d", err,
               t.desc, t.err);

    free(plaintext);
  }
}
