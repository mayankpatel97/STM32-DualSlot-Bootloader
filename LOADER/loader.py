"""
Host App -

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
import serial
from colorama import init, Fore, Back, Style
import time
import json

import serial.tools
import serial.tools.list_ports


START_BYTE = 0xAA
END_BYTE = 0xBB

PACKET_START        = 0x01
PACKET_ERASEMEM     = 0x02
PACKET_HEADER       = 0x03
PACKET_DATACHUNK    = 0x04
PACKET_END          = 0x05
PACKET_UPDATEFW     = 0x06

FW_DATA_CHUNKSIZE = 224


# Initialize colorama
init(autoreset=True)

def list_com_ports():
    print(Fore.YELLOW + "Available COM ports:")
    ports = serial.tools.list_ports.comports()
    if not ports:
        print(Fore.RED + "No COM ports found.")
    else:
        for port in ports:
            print(Fore.GREEN + f"{port.device} - {port.description}")

    return ports

def write_byte_array(port, byte_array):
    try:
        # Initialize the serial connection
        with serial.Serial(port, baudrate, timeout=1) as ser:
            print(f"Connected to {port} at {baudrate} baud rate.")

            # Send the byte array
            ser.write(byte_array)
            print(f"Sent byte array: {byte_array}")

            # Optional: wait to ensure the data is sent
            time.sleep(1)

    except serial.SerialException as e:
        print(Fore.RED + f"Error opening serial port: {e}")
    except Exception as e:
        print(Fore.RED + f"An error occurred: {e}")



def serial_connect(port, baudrate):
    # Open the serial port
    ser = serial.Serial(port, baudrate, timeout=1)
    print(f"Connected to {port} at {baudrate} baud.")
    return ser


def calculate_crc16(data, length):
    crc = 0xFFFF
    poly = 0x2024

    for i in range(length):
        crc ^= (data[i] << 8)
        for j in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ poly
            else:
                crc <<= 1

    return crc & 0xFFFF  # Ensure it stays within 16 bits


def serial_sendPacket(port,cmd,payload, length):
    head = bytearray([START_BYTE, cmd, length])
    crc16 = calculate_crc16(payload, length)
    crc16_byteArray = crc16.to_bytes(2, byteorder='big')
    packet = head + payload + crc16_byteArray + bytearray([END_BYTE])
    port.write(packet)
    print(f"sent({cmd}) : {packet}")


def firmware_file_valid(file_path):
    # Check if the file exists
    if not os.path.isfile(file_path):
        print(f"File '{file_path}' does not exist.")
        return False
    
    # Check the file extension
    file_extension = os.path.splitext(file_path)[1]  # Get the extension
    if file_extension == '.ebin':
        #print(f"File '{file_path}' exists and has the correct .fw extension.")
        return True
    else:
        #print(f"File '{file_path}' exists but does not have a .fw extension.")
        return False

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

def string_to_dict(json_string):
    """Convert a JSON string to a dictionary."""
    try:
        dictionary = json.loads(json_string)
        return dictionary
    except json.JSONDecodeError:
        raise ValueError("Invalid JSON string")

ACK = 0
NACK = 1
def ota_check_response(port,cmd):

    while True:
        data = port.readall()
        if data == b'': continue
        resp =[]
        for i in range(0,len(data)):
            resp.append(int(data[i]))

        print("Response : ", resp)
        # check CRC
        #time.sleep(2)
        if resp[1] == cmd:
            if resp[3] == 0:
                print(Fore.GREEN + f"Ack recvd for cmd {cmd}.")
                return ACK
            else:
                print(Fore.RED + f"Nack recvd for cmd {cmd}.")
                return NACK
            
def fetch(byte_array, start_char, end_char):
    # Convert characters to bytes
    start_byte = start_char.encode()  # Convert to byte
    end_byte = end_char.encode()  # Convert to byte

    # Find the starting index
    start_index = byte_array.find(start_byte)  # Find the index of the starting character
    # Find the ending index
    end_index = byte_array.find(end_byte, start_index + 1)  # Find the index of the ending character

    if start_index != -1 and end_index != -1:  # Check if both characters are found
        return byte_array[start_index:end_index + 1]  # Return the slice including both characters
    else:
        return None  # Return None if either character is not found
    
if __name__ == "__main__":
    print(Fore.GREEN + "Running host app.")
    serial_port_list = list_com_ports()
    
    if len(serial_port_list) == 0 : exit(1)
    # Specify the serial port and baud rate
    
    print("Select COM Port from list below")
    for i in range(0,len(serial_port_list)):
        print(f"{i}: {serial_port_list[i]}")
    
    print("")
    index = input("Enter Index: ")

    print(f"SELECTED: {serial_port_list[int(index)]}")

    port = serial_port_list[int(index)].device
#    serial_port = "COM19"  # Change this to your serial port (e.g., "/dev/ttyUSB0" for Linux)
    baud_rate = 115200
    # Send the packets
    
    timeout = 1  # Set a timeout for reading (in seconds)
    board = serial_connect(port,baud_rate)
    
    input_path_string = input("Drag and drop file to be loaded ..\n")
    if "'" in input_path_string:
        file_path = extract_string_between(input_path_string,"'","'")
    elif '"' in input_path_string:
        file_path = extract_string_between(input_path_string,'"','"')
    else:
        file_path = input_path_string

    if firmware_file_valid(file_path) == False: 
        print(Fore.RED + "Invalid firmware.") 
        exit(1)
    
    #remove below line 
    #file_path = 'D:/software projects/STM32-SPIFLashLoader/HOST_APP/output.ebin'
    
    print("Updating firmware...")
    fw_file = open(file_path,'rb')
    fw = fw_file.read()

    file_crc_stored = int.from_bytes(fw[0:4], byteorder='big', signed=False)
    file_crc_calculated = calculate_crc32(fw[4:])

    print(f'crc calculated: {file_crc_calculated}, stored: {file_crc_stored}')

    if file_crc_stored != file_crc_calculated:
        print(Fore.RED + "corrupt ebin file.")
        exit(1)

    print(Fore.GREEN + "Valid Firmware")
    print(Fore.RESET + "Fetching details")
    metadata_chunk = fw[4:256]
    metadata_byteArr = fetch(metadata_chunk,'{','}')
    metadata = json.loads(metadata_byteArr)
    print(metadata)

    print(Fore.BLUE + f'''
              fw         : {metadata["fw_name"]}
              owned by   : {metadata["owner"]}
              version    : {metadata["version"]}
              fw id      : {metadata["fw_id"]}
              fw size    : {metadata["size"]}
              build date : {metadata["build_date"]}
          ''')


    while True:
        decision = input("Press Y to update and N to abort. ")


        if decision == "n" or decision == "N":
            print(Fore.RED + "firmware update abort.")
            exit(1)
        
        if decision == "Y" or decision == "y":
            break

    # send FW update start packet
    print("Sending start packet")
    serial_sendPacket(board, PACKET_START, bytearray([0x01]), 1)
    if ota_check_response(board,PACKET_START) == NACK: exit(1)
    

    print("Sending memory erase packet")
    serial_sendPacket(board, PACKET_ERASEMEM, bytearray([0x01]), 1)
    if ota_check_response(board,PACKET_ERASEMEM) == NACK: exit(1)
    # wait for response 

    print("Memory Erased.")

    # send header 
    file_size = metadata["size"]
    file_sizeArr = file_size.to_bytes(4, byteorder='little')

    file_crc32 = metadata["crc32"]
    file_CrcArr = file_crc32.to_bytes(4, byteorder='little')

    file_version = metadata["version"]
    file_versionArr = file_version.to_bytes(4, byteorder='little')

    header = file_sizeArr + file_CrcArr + file_versionArr
    print(f"Sending header packet-> size: {file_size}, crc32: {file_crc32}, version: {file_version}")
    serial_sendPacket(board,PACKET_HEADER,header, len(header))
    # wait for response 
    if ota_check_response(board,PACKET_HEADER) == NACK: exit(1)
    print("Sending data packets")
    binary = fw[256:]
    sent_size =0
    #send firmware
    while True:
        if (sent_size+FW_DATA_CHUNKSIZE) < file_size:
            data = binary[sent_size:sent_size+FW_DATA_CHUNKSIZE]
            sent_size +=FW_DATA_CHUNKSIZE
        else:
            data = binary[sent_size:]
            sent_size = file_size

        serial_sendPacket(board,PACKET_DATACHUNK,data, len(data))
        if ota_check_response(board,PACKET_DATACHUNK) == NACK: exit(1)
        # wait for response 
        
        if sent_size == file_size: break

    print("Sending end packet")
    serial_sendPacket(board, PACKET_END, bytearray([0x01]), 1)
    # wait for response 
    if ota_check_response(board,PACKET_END) == NACK: exit(1)

    print("Sending update packet")
    serial_sendPacket(board, PACKET_UPDATEFW, bytearray([0x01]), 1)
    # wait for response 
    if ota_check_response(board,PACKET_UPDATEFW) == NACK: exit(1)

    board.close()
