import random

def generate_random_key(length):
    return [random.randint(0, 9) for _ in range(length)]

def write_key_to_file(key, name):
    with open('global_secrets.h', 'a') as file:
        file.write(f"#define {name} (uint8_t[]){{")
        for i in range(len(key) - 1):
            file.write(f"{key[i]}, ")
        file.write(f"{key[-1]}}}\n")

if __name__ == "__main__":
    key_len = 32

    # clear the file if it exists
    with open('global_secrets.h', 'w') as file:
        pass

    keys = ['AP_VALIDATION_KEY', 'COMP1_VALIDATION_KEY', 'COMP2_VALIDATION_KEY']

    for key in keys:
        write_key_to_file(generate_random_key(key_len), key)