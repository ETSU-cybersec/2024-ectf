#ifndef WOLFSSL_USER_SETTINGS_H
#define WOLFSSL_USER_SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/* RNG */
/* ------------------------------------------------------------------------- */

/* Seed Source */
/* Size of returned HW RNG value */

/* Choose RNG method */
#if 0 
    /* Use built-in P-RNG (SHA256 based) with HW RNG */
    /* P-RNG + HW RNG (P-RNG is ~8K) */
    #undef  HAVE_HASHDRBG
    #define HAVE_HASHDRBG
#else
	#undef CUSTOM_RAND_TYPE
	#define CUSTOM_RAND_TYPE int
	#undef CUSTOM_RAND_GENERATE
	#define CUSTOM_RAND_GENERATE rand_gen
	extern unsigned int rand_gen(void);
    /* Bypass P-RNG and use only HW RNG */
    #undef  CUSTOM_RAND_GENERATE_BLOCK
    #define CUSTOM_RAND_GENERATE_BLOCK  custom_rand_generate_block
    extern unsigned int custom_rand_generate_block(unsigned char* data, unsigned int len);

#endif

#define BLOCK_SIZE AES_BLOCK_SIZE
#define KEY_SIZE 32
#define HASH_SIZE SHA256_DIGEST_SIZE
#define HAVE_ECC_VERIFY
#define ECC_MIN_KEY_SZ 256
#define ECC_KEY_CURVE ECC_SECP384R1

#undef  NO_CRYPT_BENCHMARK
#define NO_CRYPT_BENCHMARK

#undef  NO_CRYPT_TEST
#define NO_CRYPT_TEST

#undef  NO_FILESYSTEM
#define NO_FILESYSTEM

#undef  NO_WRITEV
#define NO_WRITEV

#undef  NO_MAIN_DRIVER
#define NO_MAIN_DRIVER

#undef  NO_DSA
#define NO_DSA

#undef  NO_RC4
#define NO_RC4

#ifdef __cpluplus
}
#endif

#endif
