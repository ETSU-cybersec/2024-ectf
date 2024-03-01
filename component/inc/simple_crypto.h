/**
 * @file "simple_crypto.h"
 * @author Ben Janis
 * @brief Simplified Crypto API Header 
 * @date 2024
 *
 * This source file is part of an example system for MITRE's 2024 Embedded System CTF (eCTF).
 * This code is being provided only for educational purposes for the 2024 MITRE eCTF competition,
 * and may not meet MITRE standards for quality. Use this code at your own risk!
 *
 * @copyright Copyright (c) 2024 The MITRE Corporation
 */

#ifndef ECTF_CRYPTO_H
#define ECTF_CRYPTO_H

#include <wolfssl/wolfcrypt/settings.h>
#include "wolfssl/wolfcrypt/aes.h"
#include "wolfssl/wolfcrypt/random.h"
#include "wolfssl/wolfcrypt/ecc.h"
#include "wolfssl/wolfcrypt/hash.h"
#include "trng.h"


/******************************** MACRO DEFINITIONS ********************************/
#define BLOCK_SIZE AES_BLOCK_SIZE
#define KEY_SIZE 32
#define HASH_SIZE SHA256_DIGEST_SIZE
#define PADDING_CHAR '\0'


/******************************** FUNCTION PROTOTYPES ********************************/
/** @brief Encrypts plaintext using a symmetric cipher
 *
 * @param plaintext     A pointer to a buffer of length len containing the
 *                              plaintext to encrypt
 *
 * @param len                   The length of the plaintext to encrypt. Must be a multiple of
 *                              BLOCK_SIZE (16 bytes)
 *
 * @param key                   A pointer to a buffer of length KEY_SIZE (16 bytes) containing
 *                              the key to use for encryption
 *
 * @param ciphertext    A pointer to a buffer of length len where the resulting
 *                              ciphertext will be written to
 *
 * @return 0 on success, -1 on bad length, other non-zero for other error
 */
int encrypt_sym(uint8_t *plaintext, size_t len, uint8_t *key, uint8_t *ciphertext);

/** @brief Decrypts ciphertext using a symmetric cipher
 *
 * @param ciphertext    A pointer to a buffer of length len containing the
 *                              ciphertext to decrypt
 *
 * @param len                   The length of the ciphertext to decrypt. Must be a multiple of
 *                              BLOCK_SIZE (16 bytes)
 *
 * @param key                   A pointer to a buffer of length KEY_SIZE (16 bytes) containing
 *                              the key to use for decryption
 *
 * @param plaintext     A pointer to a buffer of length len where the resulting
 *                              plaintext will be written to
 *
 * @return 0 on success, -1 on bad length, other non-zero for other error
 */
int decrypt_sym(uint8_t *ciphertext, size_t len, uint8_t *key, uint8_t *plaintext, size_t *plaintext_length);

/** @brief Hashes arbitrary-length data (Edited to use SHA256 for strong security)
 *
 * @param data          A pointer to a buffer of length len containing the data
 *                      to be hashed
 *
 * @param len           The length of the plaintext to encrypt
 *
 * @param hash_out      A pointer to a buffer of length len where the resulting
 *                              hash output will be written to
 *
 * @return 0 on success, non-zero for other error
 */
int hash(const byte *data, size_t len, uint8_t *hash_out);

/** @brief      Initializes then generate and store key values in the ECC_Key struct
 *
 * @param ecc_key       ECC Key struct to store generated key values
 *
 * @param rng           WC_RNG struct for key generation
 *
 * @return 0 on success, -1 on bad length, other non-zero for other error
 */
int ecc_keygen(ecc_key *key, WC_RNG *rng, ecc_key *publicKey, byte *privateKey);

/** @brief Cryptographically signs an encrypted ciphertext
 *                 uses WolfSSL's WolfCrypt library for Hashing/Signing
 *                 and MAX78000fthr's on board hardware for RNG
 *
 * @param ciphertext    A pointer to a buffer of length len containing the data
 *                              to be signed
 *
 * @param signature     A pointer to a buffer of length len where the resulting
 *                              signature output will be written to
 *
 * @param key                   A pointer to an ECC_Key object for signing
 *
 * @param sig_len               A pointer to a buffer to store the size of the signature
 *
 * @param rng                   A pointer to a WC_RNG object for signing
 *
 * @param digest                A pointer to a buffer to store the hashed ciphertext
 *
 * @return 0 on success, non-zero for other error
 */
int asym_sign(uint8_t *ciphertext, byte *signature, ecc_key *key, word32 *sig_len, WC_RNG *rng, uint8_t *digest);

/* @brief Verifies an ECC signature of a hashed ciphertext for authentication purposes
 *
 * @param signature             A pointer to a buffer storing the signature need validation
 *
 * @param sig_len               Length of the signature
 *
 * @param hash                  A pointer to a buffer containing the hashed message
 *
 * @param hash_len              Length of the hashed message
 *
 * @param status                Pointer to an int that represents the result of the signature verification
 *
 * @param key                   Pointer to the ECC_Key object to verify signature with
 *
 * @return 0 indicating successful authentication, non-zero for other errors
 *      NOTE: Does not indicate if signature is only that the validation was completed successfully
 */
int asym_validate(const byte *signature, word32 sig_len, const byte *hash, word32 hash_len, int *status, ecc_key *key);
#endif // ECTF_CRYPTO_H
