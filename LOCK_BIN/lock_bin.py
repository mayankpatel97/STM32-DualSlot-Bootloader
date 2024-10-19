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

# Initialize Colorama
init()

# Print text with different colors and styles
#print(Fore.RED + 'This text is red')

# Constants
ORG_NAME = "MAYANK"  # Owner name
BLOCK_SIZE = 16  # AES block size
METADATA_SIZE = 128  # Updated metadata size in bytes
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

def calculate_crc32(data):
    """Calculate CRC32 of the data."""
    return zlib.crc32(data) & 0xFFFFFFFF

def create_metadata(crc32, version, fw_id, fw_name):
    """Create 128-byte metadata."""
    build_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    metadata = struct.pack('<I', crc32)  # CRC32 is 4 bytes

    # Add version, fw_id, and fw_name (adjusted sizes for metadata packing)
    version = version[:8].ljust(8, '\x00')  # 8 bytes for version
    fw_id = fw_id[:8].ljust(8, '\x00')  # 8 bytes for firmware ID
    fw_name = fw_name[:FW_NAME_SIZE].ljust(FW_NAME_SIZE, '\x00')  # 32 bytes for firmware name
    build_time = build_time[:19].ljust(19, '\x00')  # 19 bytes for date and time

    metadata += version.encode('utf-8')
    metadata += fw_id.encode('utf-8')
    metadata += fw_name.encode('utf-8')
    metadata += build_time.encode('utf-8')
    metadata += ORG_NAME[:ORG_NAME_SIZE].ljust(ORG_NAME_SIZE, '\x00').encode('utf-8')  # 32 bytes for organization name

    return metadata.ljust(METADATA_SIZE, b'\x00')

def encrypt_firmware(data, key):
    """Encrypt the firmware using AES-128-CBC."""
    iv = get_random_bytes(AES.block_size)
    cipher = AES.new(key, AES.MODE_CBC, iv)
    encrypted_data = cipher.encrypt(pad(data))
    return iv + encrypted_data  # Prepend IV for decryption

def lock_bin(input_file, version, fw_id, fw_name, output_file):
    """Main function to generate encrypted .fw file."""
    # Print starting message in green
    print(Fore.GREEN + "Starting the firmware encryption process..." + Style.RESET_ALL)

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

    # Create metadata
    metadata = create_metadata(crc32, version, fw_id, fw_name)
    print(Fore.CYAN + "Generated firmware metadata:" + Style.RESET_ALL)
    print(Fore.CYAN + metadata.hex() + Style.RESET_ALL)

    # Generate AES-128 encryption key (fixed or derived from a secret)
    #key = get_random_bytes(16)
    #print(Fore.MAGENTA + "Generated AES-128 encryption key." + Style.RESET_ALL)

    # Encrypt the firmware
    encrypted_firmware = encrypt_firmware(bin_data, bytearray(KEY))
    print(Fore.GREEN + "Firmware successfully encrypted." + Style.RESET_ALL)

    # Write metadata + encrypted firmware to output .fw file
    with open(output_file, 'wb') as f:
        f.write(metadata)  # Write metadata (not encrypted)
        f.write(encrypted_firmware)  # Write encrypted firmware

    print(Fore.GREEN + f"Generated {output_file} successfully." + Style.RESET_ALL)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Lock Bin - Encrypt firmware with metadata")
    parser.add_argument('input_file', help="Input .bin file")
    parser.add_argument('version', help="Firmware version")
    parser.add_argument('fw_id', help="Firmware ID")
    parser.add_argument('fw_name', help="Firmware name")
    parser.add_argument('output_file', help="Output .fw file")

    args = parser.parse_args()

    lock_bin(args.input_file, args.version, args.fw_id, args.fw_name, args.output_file)
