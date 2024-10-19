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

import serial.tools
import serial.tools.list_ports

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


start_packet = {0x01, 0x02, 0x03, 0x04}

def serial_connect(port, baudrate):
    # Open the serial port
    ser = serial.Serial(port, baudrate, timeout=1)
    print(f"Connected to {port} at {baudrate} baud.")
    return ser

START_BYTE = 0xAA
END_BYTe = 0xBB

def serial_sendPacket(port,cmd,payload, length):
    head = bytearray([START_BYTE, cmd, length])

def firmware_file_valid(file_path):
    # Check if the file exists
    if not os.path.isfile(file_path):
        print(f"File '{file_path}' does not exist.")
        return False
    
    # Check the file extension
    file_extension = os.path.splitext(file_path)[1]  # Get the extension
    if file_extension == '.fw':
        print(f"File '{file_path}' exists and has the correct .fw extension.")
        return True
    else:
        print(f"File '{file_path}' exists but does not have a .fw extension.")
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
    
    print(Fore.GREEN + "Valid Firmware")
    byte_array = bytearray([0xDE, 0xAD, 0xBE, 0xEF, 0xBA, 0xAD, 0xF0, 0x0D])
    board.write(byte_array)
    print("packet sent")
    board.close()

