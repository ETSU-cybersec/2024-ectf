/**
 * @file "simple_crypto.c"
 * @author Ben Janis
 * @brief Simplified Crypto API Implementation
 * @date 2024
 *
 * This source file is part of an example system for MITRE's 2024 Embedded System CTF (eCTF).
 * This code is being provided only for educational purposes for the 2024 MITRE eCTF competition,
 * and may not meet MITRE standards for quality. Use this code at your own risk!
 *
 * @copyright Copyright (c) 2024 The MITRE Corporation
 */

#include <wolfssl/wolfcrypt/settings.h>
#include "simple_crypto.h"
#include <stdint.h>
#include <string.h>

/******************************** FUNCTION PROTOTYPES ********************************/
/** @brief Encrypts plaintext using a symmetric cipher
 *
 * @param plaintext A pointer to a buffer of length len containing the
 *          plaintext to encrypt
 * @param len The length of the plaintext to encrypt. Must be a multiple of
 *          BLOCK_SIZE (16 bytes)
 * @param key A pointer to a buffer of length KEY_SIZE (16 bytes) containing
 *          the key to use for encryption
 * @param ciphertext A pointer to a buffer of length len where the resulting
 *          ciphertext will be written to
 *
 * @return 0 on success, -1 on bad length, other non-zero for other error
 */
int encrypt_sym(uint8_t *plaintext, size_t len, uint8_t *key, uint8_t *ciphertext) {
    Aes ctx; // Context for encryption
    int result; // Library result

    // Ensure valid length
    if (len <= 0 || len % BLOCK_SIZE)
        return -1;

    // Set the key for encryption
    result = wc_AesSetKey(&ctx, key, 16, NULL, AES_ENCRYPTION);
    if (result != 0)
        return result; // Report error


    // Encrypt each block
    for (int i = 0; i < len - 1; i += BLOCK_SIZE) {
        result = wc_AesEncryptDirect(&ctx, ciphertext + i, plaintext + i);
        if (result != 0)
            return result; // Report error
    }
    return 0;
}

/** @brief Decrypts ciphertext using a symmetric cipher
 *
 * @param ciphertext A pointer to a buffer of length len containing the
 *          ciphertext to decrypt
 * @param len The length of the ciphertext to decrypt. Must be a multiple of
 *          BLOCK_SIZE (16 bytes)
 * @param key A pointer to a buffer of length KEY_SIZE (16 bytes) containing
 *          the key to use for decryption
 * @param plaintext A pointer to a buffer of length len where the resulting
 *          plaintext will be written to
 *
 * @return 0 on success, -1 on bad length, other non-zero for other error
 */
int decrypt_sym(uint8_t *ciphertext, size_t len, uint8_t *key, uint8_t *plaintext) {
    Aes ctx; // Context for decryption
    int result; // Library result

    // Ensure valid length
    if (len <= 0 || len % BLOCK_SIZE)
        return -1;

    // Set the key for decryption
    result = wc_AesSetKey(&ctx, key, 16, NULL, AES_DECRYPTION);
    if (result != 0)
        return result; // Report error

    // Decrypt each block
    for (int i = 0; i < len - 1; i += BLOCK_SIZE) {
        result = wc_AesDecryptDirect(&ctx, plaintext + i, ciphertext + i);
        if (result != 0)
            return result; // Report error
    }
    return 0;
}

/** @brief Hashes arbitrary-length data (Edited to use SHA256 for strong security)
 *
 * @param data A pointer to a buffer of length len containing the data
 *          to be hashed
 * @param len The length of the plaintext to encrypt
 * @param hash_out A pointer to a buffer of length len where the resulting
 *          hash output will be written to
 *
 * @return 0 on success, non-zero for other error
 */
int hash(void *data, size_t len, uint8_t *hash_out) {
    // Pass values to hash
    return wc_Sha3_256Hash((uint8_t *)data, len, hash_out);
}

/**
 * @file "simple_crypto.c"
 * @author Adam Hooven/ETSU CyberBucs
 * @brief Extension of MITRE-provided Simplified Crypto API Implementation
 * @date 2024
 *
 * Extending the MITRE-provided Cryptography API Implementation to 
 * include the use of Elliptic Curve Cryptography which will allow the MISC
 * to digitally sign and verify encrypted communications
 *
 * @copyright Copyright (c) 2024 The MITRE Corporation
 */



/** @brief   Get a random number of length len
 *
 * @param   data    Pointer to a location to store the number
 * @param   len     Length of random number in bytes
 *
 * @MXC_TRNG return  Success/Fail, see \ref MXC_Error_Codes for a list of return codes.*
 * 
 * @custom_rand_generate_block return 0 for Sucess 
 * 
 * MXC_TRNG_Random - Copyright (C) 2023 Maxim Integrated Products, Inc., All Rights Reserved.
 */
unsigned int custom_rand_generate_block(byte* data, word32 len) {
	int ret = MXC_TRNG_Random(data, len);
	if (ret != 0) {
		return ret;
	}
	return 0;
}

/**@brief   Get a random number from MXC TRNG to seed WolfCrypt RNG
 *
 * @return  A random 32-bit number
 *
 * MXC_TRNG_RandomInt - Copyright (C) 2023 Maxim Integrated Products, Inc., All Rights Reserved.
 */
unsigned int rand_gen(void) {
	return MXC_TRNG_RandomInt();
}

/** @brief 	Initializes then generate and store key values in the ECC_Key struct
 *
 * @param ecc_key	ECC Key struct to store generated key values	
 *
 * @param rng		WC_RNG struct for key generation
 *
 * @return 0 on success, -1 on bad length, other non-zero for other error
 */
int ecc_keygen(ecc_key *key, WC_RNG *rng) {
	//Initialize the Hardware RNG
	int boardInit = MXC_TRNG_Init();
	if (boardInit != 0) {
		return boardInit; //Report error
	}

	//Initialize WolfSSL WC_RNG struct
	int rngInit = wc_InitRng(rng);
	if (rngInit != 0) {
		return rngInit; //Report error
	}

	//Initialize ECC Key struct
	int eccInit = wc_ecc_init(key);
	if (eccInit != 0) {
		return eccInit; //Report error
	}

	//Generate Key
	int makeKey = wc_ecc_make_key(rng, KEY_SIZE, key);
	if(makeKey != 0) {
		return makeKey; //Report error
	}
	
	return 0;
}

/** @brief Cryptographically signs an encrypted ciphertext 
 * 		   uses WolfSSL's WolfCrypt library for Hashing/Signing
 * 		   and MAX78000fthr's on board hardware for RNG
 *
 * @param ciphertext 	A pointer to a buffer of length len containing the data
 *          			to be signed
 * @param len 			The length of the plaintext to encrypt
 * @param sig_out 		A pointer to a buffer of length len where the resulting
 *          			signature output will be written to
 * @param pub_key		A pointer to the ecc_key to export
 *
 * @return 0 on success, non-zero for other error
 */
int asym_sign(uint8_t *ciphertext, byte *sig_out, ecc_key *key, WC_RNG *rng) {
	int result; //Result of signing
	uint8_t digest; //Buffer to store hash

	//Hash the encrypted command
	hash(ciphertext, BLOCK_SIZE, &digest);

	word32 sig_len = wc_ecc_sig_size(key);

	//Sign the hashed message
	result = wc_ecc_sign_hash(&digest, HASH_SIZE, sig_out, &sig_len, rng, key);
	if (result != 0){
		return result; //Report error
	}

	return 0;
}
