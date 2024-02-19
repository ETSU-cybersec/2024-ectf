/**
 * @file "simple_crypto.c"
 * @author Ben Janis
 * @brief Simplified Crypto API Implementation
 * @date 2024
 *
 * This source file is part of an 
*/

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

/** @brief Hashes arbitrary-length data
 *
 * @param data A pointer to a buffer of length len containing the data
 *          to be hashed
 * @param len The length of the plaintext to encrypt
 * @param hash_out A pointer to a buffer of length HASH_SIZE (16 bytes) where the resulting
 *          hash output will be written to
 *
 * @return 0 on success, non-zero for other error
 */
int hash(void *data, size_t len, uint8_t *hash_out) {
    // Pass values to hash
    return wc_Sha256Hash((uint8_t *)data, len, hash_out);
}

/*
unsigned int custom_rand_generate_block(byte* data, word32 len) {
	MXC_TRNG_Random(data, len);
	return 0;
}


unsigned int rand_gen(void) {
	return MXC_TRNG_RandomInt();
}

int asym_sign(uint8_t *ciphertext, size_t len) {
	ecc_key key;
	int makeKey, eccInit, rngInit, boardInit;
	WC_RNG rng;

	boardInit = MXC_TRNG_Init();
	if (boardInit != 0) {
		printf("BOARD RNG FAIL: %d\n", boardInit);
		return boardInit;
	}

	rngInit = wc_InitRng(&rng);
	if (rngInit != 0) {
		printf("RNG GEN FAIL: %d\n", rngInit);
	}

	eccInit = wc_ecc_init(&key);
	if (eccInit == 0) {
		
		printf("INIT KEY SUCCES\n");

		makeKey = wc_ecc_make_key(&rng, KEY_SIZE, &key);
	
		if(makeKey != 0) {
			printf("MAKE KEY FAIL: %d\n", makeKey);
			return makeKey;
		}
	}
	else {
		printf("KEY INIT FAIL: %d\n", eccInit);
		return eccInit;
	}

	
	word32 sig_len = 64;
	byte sig[sig_len];
	
	int result = wc_ecc_sign_hash(ciphertext, len, sig, &sig_len, &rng, &key);
	if (result != 0){
		printf("SIGN FAIL: %d\n", result); //Error reporting
	}
	

	return 0;
}*/
