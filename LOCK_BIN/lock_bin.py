"""
Lock Bin - A console application for generating AES-128 CBC encrypted firmware files with metadata.
It takes a .bin file, firmware version, firmware ID, and firmware name as input.
The application then adds 128 bytes of unencrypted metadata, including CRC32 checksum, 
firmware details, build date and time, and organization name. 
The firmware is encrypted using AES-128 CBC encryption.
The output is a .fw file.

Usage:
    python lock_bin.py <input_file.bin> <version> <fw_id> <fw_name> <output_file.fw>

This application provides terminal feedback with colored text to indicate progress and success.

Dependencies
    pip install colorama
    pip install pycryptodome

command line format : python lock_bin.py <input_file.bin> <version> <fw_id> <fw_name> <output_file.fw>
command line example: python lock_bin.py firmware.bin 1.0.3 1234ABC "MyFirmwareName" encrypted_output.fw

"""
import os
import struct
import hashlib
import datetime
import argparse
from Crypto.Cipher import AES
from Crypto.Random import get_random_bytes
import zlib
# import colorma module
from colorama import init, Fore, Back, Style
import json

# Initialize Colorama
init()

# Print text with different colors and styles
#print(Fore.RED + 'This text is red')

# Constants

BLOCK_SIZE = 16  # AES block size
METADATA_SIZE = 256  # Updated metadata size in bytes
FW_NAME_SIZE = 32  # Firmware name size in metadata
ORG_NAME_SIZE = 32  # Organization name size in metadata
KEY = 'encryptionkey123'.encode('utf-8')



def pad(data):
    """Pad data to be a multiple of BLOCK_SIZE."""
    pad_len = BLOCK_SIZE - len(data) % BLOCK_SIZE
    return data + bytes([pad_len]) * pad_len

def unpad(data):
    """Unpad the data."""
    return data[:-data[-1]]

def encrypt_firmware(data, key):
    """Encrypt the firmware using AES-128-CBC."""
    iv = get_random_bytes(AES.block_size)
    cipher = AES.new(key, AES.MODE_CBC, iv)
    encrypted_data = cipher.encrypt(pad(data))
    return iv + encrypted_data  # Prepend IV for decryption

def extract_string_between(input_string, start_char, end_char):
    # Find the positions of the start and end characters
    start_index = input_string.find(start_char)
    end_index = input_string.find(end_char, start_index + 1)
    
    # Check if both characters are found
    if start_index != -1 and end_index != -1:
        # Extract the string between the two characters
        return input_string[start_index + 1:end_index]
    else:
        return "Characters not found in the string."

def firmware_file_valid(input_path_string):

    return_stat = True
    if "'" in input_path_string:
        file_path = extract_string_between(input_path_string,"'","'")
    elif '"' in input_path_string:
        file_path = extract_string_between(input_path_string,'"','"')
    else:
        file_path = input_path_string
    # Check if the file exists
    if not os.path.isfile(file_path):
        print(f"File '{file_path}' does not exist.")
        return_stat = False

    return return_stat, file_path

def calculate_crc32(data):
    """Calculate CRC32 checksum of the given data."""
    crc = 0xFFFFFFFF  # Initialize CRC to all bits set to 1

    for byte in data:
        crc ^= byte  # XOR byte into the least-significant byte of crc
        for _ in range(8):  # Process each bit
            # Check if the least significant bit is set
            if crc & 1:
                crc = (crc >> 1) ^ 0xEDB88320  # Polynomial used in CRC32
            else:
                crc >>= 1  # Just shift right

    # Finalize the CRC value
    return crc ^ 0xFFFFFFFF  # Invert all bits to get the final CRC32 value

    
def lock_bin():
    """Main function to generate encrypted .fw file."""
    input_file = ''
    '''
    metadata_dict = {"build_time": datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), 
                    "version"   : 1.0,
                    "fw_id"     : 30,
                    "fw_name"   : led blink,
                    "owner"     : "Mayank"
                    "crc"       : 2345656
                    }
    '''
    metadata_dict = {}
    state = 0
    while True:

        if  state == 0:
            input_path_string = input("Drag and drop input file below.\n\n")
            valid, input_file = firmware_file_valid(input_path_string) 
            if valid == False:  print(Fore.RED + "Invalid bin file.") 
            else:
                print(Fore.GREEN + "Valid binary file")
                state += 1

            print(Fore.RESET)

        elif    state == 1:
            version_input = input("\nEnter Version Number\n")
            try:
                metadata_dict["version"] = int(version_input)
                state += 1
            except:
                print(Fore.RED + "Invalid version number") 

        
        elif    state == 2:
            fw_id = input("\nEnter firmware ID number\n")

            try:
                metadata_dict["fw_id"] = int(fw_id)
                state += 1
            except:
                print(Fore.RED + "Invalid firmware ID number") 

        elif    state == 3:
            fw_name = input("\nEnter firmware name\n")
            if len(fw_name) > 64: 
                print(Fore.RED + "firmware name out of range") 
                continue

            metadata_dict["fw_name"] = fw_name
            state += 1

        elif    state == 4:
            owner = input("\nEnter owner info\n")
            if len(owner) > 64: 
                print(Fore.RED + "ownner info out of range") 
                continue

            metadata_dict["owner"] = owner
            state += 1
        else:
            break

    
    # Read binary input file
    try:
        with open(input_file, 'rb') as f:
            bin_data = f.read()
        print(Fore.BLUE + f"Loaded input file: {input_file}" + Style.RESET_ALL)
    except FileNotFoundError:
        print(Fore.RED + f"Error: Input file {input_file} not found!" + Style.RESET_ALL)
        return

    
    # Calculate CRC32 for the binary file
    crc32 = calculate_crc32(bin_data)
    print(Fore.YELLOW + f"Calculated CRC32: {crc32:#010x}" + Style.RESET_ALL)
    metadata_dict["crc32"] = crc32
    metadata_dict["build_date"] = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    metadata_dict["size"] = len(bin_data)
    metadata_str = json.dumps(metadata_dict)
    metadata_byteArray = bytearray(metadata_str, 'utf-8')
    print("metadata len: ", len(metadata_byteArray))
    print("meta data: ", metadata_dict)
    
    # make the meta data 256 byte
    metadata = metadata_byteArray + bytearray(METADATA_SIZE - len(metadata_byteArray))
    # Print starting message in green
    print(Fore.GREEN + "Starting the firmware encryption process..." + Style.RESET_ALL)

    # Encrypt the firmware
    encrypted_firmware = encrypt_firmware(bin_data, bytearray(KEY))
    output_content = metadata + encrypted_firmware
    crc32 = calculate_crc32(output_content)
    crc32_byteArray = crc32.to_bytes(4, byteorder='big', signed=False)
    print(Fore.GREEN + "Firmware successfully encrypted." + Style.RESET_ALL)
    output_file = "output.ebin"
    # Write metadata + encrypted firmware to output .fw file
    with open(output_file, 'wb') as f:
        f.write(crc32_byteArray + output_content)  # Write metadata (not encrypted)

    print(Fore.GREEN + f"Generated {output_file} successfully." + Style.RESET_ALL)

if __name__ == '__main__':

    lock_bin()


