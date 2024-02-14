import random

def generate_random_key(length):
    return [random.randint(0, 9) for _ in range(length)]

def write_key_to_file(value, name):
    with open('global_secrets.h', 'a') as file:
        file.write(f"#define {name} (uint8_t[]){{")
        for i in range(len(value) - 1):
            file.write(f"{value[i]}, ")
        file.write(f"{value[-1]}}}\n")

def write_counter_to_file(value, name):
    with open('global_secrets.h', 'a') as file:
        file.write(f'#define {name} {value}\n')

if __name__ == "__main__":
    key_len = 32
    max_counter = 1000
    
    # clear the file if it exists
    with open('global_secrets.h', 'w') as file:
        pass

    keys = ['VALIDATION_KEY', 'SECURE_MESSAGING_KEY']
    counters = ['VALIDATION_PADDING', 'SECURE_MESSAGING_PADDING']

    for key in keys:
        write_key_to_file(generate_random_key(key_len), key)

    for counter in counters:    
        write_counter_to_file(random.randint(1, max_counter), counter)