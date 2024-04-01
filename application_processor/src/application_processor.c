/**
 * @file application_processor.c
 * @author Jacob Doll
 * @brief eCTF AP Example Design Implementation
 * @date 2024
 *
 * This source file is part of an example system for MITRE's 2024 Embedded System CTF (eCTF).
 * This code is being provided only for educational purposes for the 2024 MITRE eCTF competition,
 * and may not meet MITRE standards for quality. Use this code at your own risk!
 *
 * @copyright Copyright (c) 2024 The MITRE Corporation
 */

#include "board.h"
#include "i2c.h"
#include "icc.h"
#include "led.h"
#include "mxc_delay.h"
#include "mxc_device.h"
#include "nvic_table.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "board_link.h"
#include "simple_flash.h"
#include "host_messaging.h"
#include "simple_crypto.h"
#include "timer.h"
#include <wolfssl/wolfcrypt/random.h>

#ifdef POST_BOOT
#include <stdint.h>
#include <stdio.h>
#endif

// Includes from containerized build
#include "ectf_params.h"
#include "global_secrets.h"

/********************************* CONSTANTS **********************************/
// Function Macro
#define timer get_timer_count()

// Flash Macros
#define FLASH_ADDR ((MXC_FLASH_MEM_BASE + MXC_FLASH_MEM_SIZE) - (2 * MXC_FLASH_PAGE_SIZE))

// Library call return types
#define SUCCESS_RETURN 0
#define ERROR_RETURN -1

/******************************** TYPE DEFINITIONS ********************************/
// Data structure for sending commands to component
// Params allows for up to MAX_I2C_MESSAGE_LEN - 1 bytes to be send
// along with the opcode through board_link. This is not utilized by the example
// design but can be utilized by your design.
typedef struct {
    uint8_t opcode;
    uint8_t params[MAX_I2C_MESSAGE_LEN-1];
} command_message;

// Data type for receiving a validate message
typedef struct {
    uint32_t component_id;
} validate_message;

// Data type for receiving a scan message
typedef struct {
    uint32_t component_id;
} scan_message;

// Datatype for information stored in flash
typedef struct {
    uint32_t flash_magic;
    uint32_t component_cnt;
    uint32_t component_ids[32];
} flash_entry;

// Datatype for commands sent to components
typedef enum {
    COMPONENT_CMD_NONE,
    COMPONENT_CMD_SCAN,
    COMPONENT_CMD_VALIDATE,
    COMPONENT_CMD_BOOT,
    COMPONENT_CMD_ATTEST,
    COMPONENT_CMD_COORDINATE
} component_cmd_t;

/********************************* GLOBAL VARIABLES **********************************/
// Variable for information stored in flash memory
flash_entry flash_status;
size_t secure_msg_size = BLOCK_SIZE * 15;
uint8_t symmetric_key[32];

/******************************* POST BOOT FUNCTIONALITY *********************************/
/**
 * @brief Secure Send 
 * 
 * @param address: i2c_addr_t, I2C address of recipient
 * @param buffer: uint8_t*, pointer to data to be send
 * @param len: uint8_t, size of data to be sent 
 * 
 * Securely send data over I2C. This function is utilized in POST_BOOT functionality.
 * This function must be implemented by your team to align with the security requirements.

*/
int secure_send(uint8_t address, uint8_t* buffer, uint8_t len) {
    uint8_t padded_buffer[secure_msg_size];
    uint8_t ciph[secure_msg_size];

    // Copy the original plaintext to the padded buffer
    memcpy(padded_buffer, buffer, len);

    // Add padding bytes with the chosen character
    for (int i = len; i < secure_msg_size; i++) {
        padded_buffer[i] = PADDING_CHAR;
    }
    
    uint8_t key[KEY_SIZE];
    memcpy(key, symmetric_key, KEY_SIZE * sizeof(uint8_t));
    encrypt_sym((uint8_t*)padded_buffer, secure_msg_size, key, ciph);

    // Send the encrypted data
    return send_packet(address, secure_msg_size, ciph);
}


/**
 * @brief Secure Key Send 
 * 
 * @param address: i2c_addr_t, I2C address of recipient
 * @param buffer: uint8_t*, pointer to data to be send
 * @param len: uint8_t, size of data to be sent 
 * 
 * Securely send key data over I2C.
*/
int secure_key_send(uint8_t address, uint8_t* buffer, uint8_t len) {
    uint8_t padded_buffer[secure_msg_size];
    uint8_t ciph[secure_msg_size];

    // Copy the original plaintext to the padded buffer
    memcpy(padded_buffer, buffer, len);

    // Add padding bytes with the chosen character
    for (int i = len; i < secure_msg_size; i++) {
        padded_buffer[i] = PADDING_CHAR;
    }
    
    uint8_t key[KEY_SIZE];
    memcpy(key, VALIDATION_KEY, KEY_SIZE * sizeof(uint8_t));
    encrypt_sym((uint8_t*)padded_buffer, secure_msg_size, key, ciph);

    // Send the encrypted data
    return send_packet(address, secure_msg_size, ciph);
}

/**
 * @brief Secure Receive
 * 
 * @param address: i2c_addr_t, I2C address of sender
 * @param buffer: uint8_t*, pointer to buffer to receive data to
 * 
 * @return int: number of bytes received, negative if error
 * 
 * Securely receive data over I2C. This function is utilized in POST_BOOT functionality.
 * This function must be implemented by your team to align with the security requirements.
*/
int secure_receive(i2c_addr_t address, uint8_t* buffer) {
    poll_and_receive_packet(address, buffer);
    
    // //Decrypt receive
    uint8_t key[KEY_SIZE];
    memcpy(key, symmetric_key, KEY_SIZE * sizeof(uint8_t));
    size_t plaintext_length;
    decrypt_sym(buffer, secure_msg_size, key, buffer, &plaintext_length);
    
    return plaintext_length;
}

/**
 * @brief Get Provisioned IDs
 * 
 * @param uint32_t* buffer
 * 
 * @return int: number of ids
 * 
 * Return the currently provisioned IDs and the number of provisioned IDs
 * for the current AP. This functionality is utilized in POST_BOOT functionality.
 * This function must be implemented by your team.
*/
int get_provisioned_ids(uint32_t* buffer) {
    memcpy(buffer, flash_status.component_ids, flash_status.component_cnt * sizeof(uint32_t));
    return flash_status.component_cnt;
}

/********************************* UTILITIES **********************************/

// Initialize the device
// This must be called on startup to initialize the flash and i2c interfaces
void init() {

    // Enable global interrupts    
    __enable_irq();

    // Setup Flash
    flash_simple_init();

    // Test application has been booted before
    flash_simple_read(FLASH_ADDR, (uint32_t*)&flash_status, sizeof(flash_entry));

    // Write Component IDs from flash if first boot e.g. flash unwritten
    if (flash_status.flash_magic != FLASH_MAGIC) {
        print_debug("First boot, setting flash!\n");

        flash_status.flash_magic = FLASH_MAGIC;
        flash_status.component_cnt = COMPONENT_CNT;
        uint32_t component_ids[COMPONENT_CNT] = {COMPONENT_IDS};
        memcpy(flash_status.component_ids, component_ids, 
            COMPONENT_CNT*sizeof(uint32_t));

        flash_simple_write(FLASH_ADDR, (uint32_t*)&flash_status, sizeof(flash_entry));
    }
    
    // Initialize board link interface
    board_link_init();

    // generate symmetric and asymmetric keys
    generate_keys(symmetric_key);

    // initialize hardware timer
    init_timer();
}

// Send a command to a component and receive the result
int issue_cmd(i2c_addr_t addr, uint8_t* transmit, uint8_t* receive) {
    // Encrypt and send message
    int result = send_packet(addr, sizeof(uint8_t), transmit);
    
    if (result == ERROR_RETURN) {
        return ERROR_RETURN;
    }

    // receive and decrypt response
    int len = secure_receive(addr, receive);

    if (len == ERROR_RETURN) {
        return ERROR_RETURN;
    }

    return len;
}

/******************************** COMPONENT COMMS ********************************/

int scan_components() {
    // Print out provisioned component IDs
    for (unsigned i = 0; i < flash_status.component_cnt; i++) {
        print_info("P>0x%08x\n", flash_status.component_ids[i]);
    }

    // Buffers for board link communication
    uint8_t receive_buffer[MAX_I2C_MESSAGE_LEN];
    uint8_t transmit_buffer[MAX_I2C_MESSAGE_LEN];

    // Scan scan command to each component 
    for (i2c_addr_t addr = 0x8; addr < 0x78; addr++) {
        // I2C Blacklist:
        // 0x18, 0x28, and 0x36 conflict with separate devices on MAX78000FTHR
        if (addr == 0x18 || addr == 0x28 || addr == 0x36) {
            continue;
        }

        // Create command message 
        command_message* command = (command_message*) transmit_buffer;
        command->opcode = COMPONENT_CMD_SCAN;

        // Send out command and receive result
        int result = send_packet(addr, sizeof(uint8_t), transmit_buffer);
        if (result == ERROR_RETURN) {
            continue;
        }
        
        // Receive message
        int len = poll_and_receive_packet(addr, receive_buffer);
        if (len == ERROR_RETURN) {
            continue;
        }

        // Success, device is present
        if (len > 0) {
            scan_message* scan = (scan_message*) receive_buffer;
            print_info("F>0x%08x\n", scan->component_id);
        }
    }
    print_success("List\n");
    return SUCCESS_RETURN;
}

int validate_components() {
    // Buffers for board link communication
    uint8_t receive_buffer[MAX_I2C_MESSAGE_LEN];
    uint8_t transmit_buffer[MAX_I2C_MESSAGE_LEN];

    // Send validate command to each component
    for (unsigned i = 0; i < flash_status.component_cnt; i++) {
        // Set the I2C address of the component
        i2c_addr_t addr = component_id_to_i2c_addr(flash_status.component_ids[i]);

        // Create command message
        command_message* command = (command_message*) transmit_buffer;
        command->opcode = COMPONENT_CMD_VALIDATE;
        
        // Send out command and receive result
        int len = issue_cmd(addr, transmit_buffer, receive_buffer);
        if (len == ERROR_RETURN) {
            print_error("Could not validate component\n");
            return ERROR_RETURN;
        }

        validate_message* validate = (validate_message*) receive_buffer;
        // Check that the result is correct
        if (validate->component_id != flash_status.component_ids[i]) {
            print_error("Component ID: 0x%08x invalid\n", flash_status.component_ids[i]);
            return ERROR_RETURN;
        }
    }
    return SUCCESS_RETURN;
}

int boot_components() {
    // Buffers for board link communication
    uint8_t receive_buffer[MAX_I2C_MESSAGE_LEN];
    uint8_t transmit_buffer[MAX_I2C_MESSAGE_LEN];

    // Send boot command to each component
    for (unsigned i = 0; i < flash_status.component_cnt; i++) {
        // Set the I2C address of the component
        i2c_addr_t addr = component_id_to_i2c_addr(flash_status.component_ids[i]);
        
        // Create command message
        command_message* command = (command_message*) transmit_buffer;
        command->opcode = COMPONENT_CMD_BOOT;


        // Send out command and receive result
        int len = issue_cmd(addr, transmit_buffer, receive_buffer);
        if (len == ERROR_RETURN) {
            print_error("Could not boot component\n");
            return ERROR_RETURN;
        }

        // Print boot message from component
        print_info("0x%08x>%s\n", flash_status.component_ids[i], receive_buffer);
    }
    return SUCCESS_RETURN;
}

int attest_component(uint32_t component_id) {
    // Buffers for board link communication
    uint8_t receive_buffer[MAX_I2C_MESSAGE_LEN];
    uint8_t transmit_buffer[MAX_I2C_MESSAGE_LEN];

    // Set the I2C address of the component
    i2c_addr_t addr = component_id_to_i2c_addr(component_id);

    // Create command message
    command_message* command = (command_message*) transmit_buffer;
    command->opcode = COMPONENT_CMD_ATTEST;

    // Send out command and receive result
    int len = issue_cmd(addr, transmit_buffer, receive_buffer);
    if (len == ERROR_RETURN) {
        print_error("Could not attest component\n");
        return ERROR_RETURN;
    }

    // Print out attestation data 
    print_info("C>0x%08x\n", component_id);
    print_info("%s", receive_buffer);
    return SUCCESS_RETURN;
}

/********************************* AP LOGIC ***********************************/

// Boot sequence
// YOUR DESIGN MUST NOT CHANGE THIS FUNCTION
// Boot message is customized through the AP_BOOT_MSG macro
void boot() {
    // POST BOOT FUNCTIONALITY
    // DO NOT REMOVE IN YOUR DESIGN
    #ifdef POST_BOOT
        POST_BOOT
    #else
    // Everything after this point is modifiable in your design
    // LED loop to show that boot occurred
    while (1) {
        LED_On(LED1);
        MXC_Delay(500000);
        LED_On(LED2);
        MXC_Delay(500000);
        LED_On(LED3);
        MXC_Delay(500000);
        LED_Off(LED1);
        MXC_Delay(500000);
        LED_Off(LED2);
        MXC_Delay(500000);
        LED_Off(LED3);
        MXC_Delay(500000);
    }
    #endif
}

// Compare the entered PIN to the correct PIN
int validate_pin() {
    size_t size = 50;
    char buf[size];
    recv_input("Enter pin: ", buf, size);
    if (!strcmp(buf, AP_PIN)) {
        print_debug("Pin Accepted!\n");
        return SUCCESS_RETURN;
    }
    print_error("Invalid PIN!\n");
    return ERROR_RETURN;
}

// Function to validate the replacement token
int validate_token() {
    size_t size = 50;
    char buf[size];
    recv_input("Enter token: ", buf, size);
    if (!strcmp(buf, AP_TOKEN)) {
        print_debug("Token Accepted!\n");
        return SUCCESS_RETURN;
    }
    print_error("Invalid Token!\n");
    return ERROR_RETURN;
}

// Boot the components and board if the components validate
void attempt_boot() {
    if (validate_components()) {
        print_error("Components could not be validated\n");
        return;
    }
    print_debug("All Components validated\n");
    if (boot_components()) {
        print_error("Failed to boot all components\n");
        return;
    }

    // Print boot message
    // This always needs to be printed when booting
    print_info("AP>%s\n", AP_BOOT_MSG);
    print_success("Boot\n");
    // Boot
    boot();
}

// Replace a component if the PIN is correct
void attempt_replace() {
    size_t size = 50;
    char buf[size];

    if (validate_token()) {
        return;
    }

    uint32_t component_id_in = 0;
    uint32_t component_id_out = 0;

    recv_input("Component ID In: ", buf, size);
    sscanf(buf, "%x", &component_id_in);
    recv_input("Component ID Out: ", buf, size);
    sscanf(buf, "%x", &component_id_out);

    // Find the component to swap out
    for (unsigned i = 0; i < flash_status.component_cnt; i++) {
        if (flash_status.component_ids[i] == component_id_out) {
            flash_status.component_ids[i] = component_id_in;

            // write updated component_ids to flash
            flash_simple_erase_page(FLASH_ADDR);
            flash_simple_write(FLASH_ADDR, (uint32_t*)&flash_status, sizeof(flash_entry));

            print_debug("Replaced 0x%08x with 0x%08x\n", component_id_out,
                    component_id_in);
            print_success("Replace\n");
            
            // send current key
            secure_key_send(flash_status.component_ids[i], symmetric_key, 32);
            // block program until ACK received
            uint8_t receive_buffer[secure_msg_size];
            secure_receive(flash_status.component_ids[i], receive_buffer);

            // send coordinate msg
            uint8_t transmit_buffer[MAX_I2C_MESSAGE_LEN];
            command_message* command = (command_message*) transmit_buffer;
            command->opcode = COMPONENT_CMD_COORDINATE;
            command->params[0] = timer;
            int len = issue_cmd(flash_status.component_ids[i], transmit_buffer, receive_buffer);

            return;
        }
    }

    // Component Out was not found
    print_error("Component 0x%08x is not provisioned for the system\r\n",
            component_id_out);
}

// Attest a component if the PIN is correct
void attempt_attest() {
    size_t size = 50;
    char buf[size];

    if (validate_pin()) {
        return;
    }
    uint32_t component_id;
    recv_input("Component ID: ", buf, size);
    sscanf(buf, "%x", &component_id);
    if (attest_component(component_id) == SUCCESS_RETURN) {
        print_success("Attest\n");
    }
}

// generate symmetric key for I2C communication
void generate_keys(uint8_t* symKey) {
    // Clear the memory allocated for the symmetric key
    memset(symKey, 0, sizeof(symKey));
    // Initialize the True Random Number Generator (TRNG) peripheral
    MXC_TRNG_Init();
    // Generate random bytes for the symmetric key using TRNG
    MXC_TRNG_Random(symKey, 32);
    // Shutdown the TRNG peripheral to conserve power
    MXC_TRNG_Shutdown();

}

/*********************************** MAIN *************************************/

int main() {
    // Initialize board
    init();

    print_info("Application Processor Started\n");

    // Handle commands forever
    size_t size = 50;
    char buf[size];
    while (1) {
        recv_input("Enter Command: ", buf, size);

        // send symmetric key to component(s)
        if ( timer % 5 == 0) {
	    generate_keys(symmetric_key);

        for (i2c_addr_t addr = 0x8; addr < 0x78; addr++) {
            // I2C Blacklist:
            // 0x18, 0x28, and 0x36 conflict with separate devices on MAX78000FTHR
            if (addr == 0x18 || addr == 0x28 || addr == 0x36) {
                continue;
            }

            int result = secure_key_send(addr, symmetric_key, 32);
            if (result == ERROR_RETURN) {
                continue;
            }
        
            // Receive message
            uint8_t receive_buffer[secure_msg_size];
            int len = secure_receive(addr, receive_buffer);
            if (len == ERROR_RETURN) {
                continue;
            }   
        }
            reset_timer();
        }

        increment_timer_count();

        // Execute requested command
        if (!strcmp(buf, "list")) {
            scan_components();
        } else if (!strcmp(buf, "boot")) {
            attempt_boot();
        } else if (!strcmp(buf, "replace")) {
            attempt_replace();
        } else if (!strcmp(buf, "attest")) {
            attempt_attest();
		} else {
            print_error("Unrecognized command '%s'\n", buf);
        }
    }

    // Code never reaches here
    return 0;
}
