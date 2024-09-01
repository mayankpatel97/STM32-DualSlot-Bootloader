
import serial
import sys
import os
import time

FW_TYPE_APP = 0x01
FW_TYPE_BOOTLOADER = 0x02

FW_TYPE = FW_TYPE_APP
#FW_TYPE = FW_TYPE_BOOTLOADER
FW_VERSION = 0x3A67

def crc16(data: bytes, poly=0x2024):
    """Compute CRC16 using the given polynomial."""
    crc = 0xFFFF
    for byte in data:
        crc ^= (byte << 8)
        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ poly
            else:
                crc <<= 1
            crc &= 0xFFFF
    return crc

ACK = 0
NACK = 1
ETX_OTA_DATA_MAX_SIZE = 128
START_BYTE = 0x2A
END_BYTE = 0x23

CMD_START_PACKET = 0x01
CMD_INFO_PACKET = 0x02
CMD_START_PACKET_LENGTH = 0x01 # including cmd,2byte data length and 1 byte data
CMD_FWDATA_PACKET = 0x03
CMD_STOP_PACKET = 0x04
CMD_STOP_PACKET_LENGTH = 0x01



def ota_send_start_command(port):
    #port.write("Sending OTA START".encode("utf-8"))
    start_packet = []
    start_packet.append(START_BYTE)
    start_packet.append(CMD_START_PACKET)
    start_packet.append(CMD_START_PACKET_LENGTH)
    start_packet.append(0x00)
    start_packet.append(0x01)
    crc16 = calculate_crc16(start_packet[1:])
    crc_byte_array = crc16.to_bytes(2, byteorder='big')
    start_packet.append(crc_byte_array[1])
    start_packet.append(crc_byte_array[0])
    start_packet.append(END_BYTE)
    print("Start Packet : ", start_packet)
    #crc_byte_array = crc16.to_bytes(2, byteorder='big')
    port.write(bytes(start_packet))

def ota_send_header_command(port,fileSize,FW_TYPE,FW_CRC,Version):
    #port.write("Sending OTA Header".encode("utf-8"))
    #CMD_INFO_PACKET
    info_packet = []
    info_packet.append(START_BYTE)
    info_packet.append(CMD_INFO_PACKET)
    info_packet.append(0x09)
    info_packet.append(0x00) # packet length

    filesize_byte_array = fileSize.to_bytes(4, byteorder='big')
    info_packet.append(int(filesize_byte_array[3]))
    info_packet.append(int(filesize_byte_array[2]))
    info_packet.append(int(filesize_byte_array[1]))
    info_packet.append(int(filesize_byte_array[0]))

    info_packet.append(FW_TYPE)

    fwType_byte_array = FW_CRC.to_bytes(2, byteorder='big')
    info_packet.append(int(fwType_byte_array[1]))
    info_packet.append(int(fwType_byte_array[0]))

    version_byte_array = Version.to_bytes(2, byteorder='big')
    info_packet.append(int(version_byte_array[1]))
    info_packet.append(int(version_byte_array[0]))

    crc16 = calculate_crc16(info_packet[1:])
    crc_byte_array = crc16.to_bytes(2, byteorder='big')
    info_packet.append(crc_byte_array[1])
    info_packet.append(crc_byte_array[0])
    info_packet.append(END_BYTE)
    print("header Packet : ", info_packet)
    port.write(bytes(info_packet))
    

def ota_check_response(port,cmd):

    while True:
        data = port.readall();
        if data == b'': continue
        resp =[]
        for i in range(0,len(data)):
            resp.append(int(data[i]))

        print("Response : ", resp)
        # check CRC
        #time.sleep(2)
        if resp[1] == cmd:
            if resp[4] == 0:
                return ACK
            else:
                return NACK

def ota_send_data(port, data, datalen,log):
    #port.write("Sending OTA Header".encode("utf-8"))
    print("Sending OTA DATA : ", datalen, len(data))
    fw_packet = []
    fw_packet.append(START_BYTE)
    fw_packet.append(CMD_FWDATA_PACKET)
    
    filesize_byte_array = datalen.to_bytes(4, byteorder='big')
    fw_packet.append(int(datalen & 0x00FF))
    fw_packet.append(int((datalen & 0xFF00) >> 8))
    

    for x in range(0,datalen):
        fw_packet.append(int(data[x]))
        val = int(data[x])
        fmt_data = f"{val},"
        log.write(fmt_data.encode("utf-8"));
    #print("w data : %x",data)

    crc16 = calculate_crc16(fw_packet[1:])
    crc_byte_array = crc16.to_bytes(2, byteorder='big')
    fw_packet.append(crc_byte_array[1])
    fw_packet.append(crc_byte_array[0])
    fw_packet.append(END_BYTE)

    #print("fw packet : ", fw_packet)
    port.write(bytes(fw_packet))
    log.write(b'\n');


def ota_send_stop_command(port):
    #port.write("Sending OTA START".encode("utf-8"))
    stop_packet = []
    stop_packet.append(START_BYTE)
    stop_packet.append(CMD_STOP_PACKET)
    stop_packet.append(CMD_STOP_PACKET_LENGTH)
    stop_packet.append(0x00)
    stop_packet.append(0x01)
    crc16 = calculate_crc16(stop_packet[1:])
    crc_byte_array = crc16.to_bytes(2, byteorder='big')
    stop_packet.append(crc_byte_array[1])
    stop_packet.append(crc_byte_array[0])
    stop_packet.append(END_BYTE)
    print("Stop Packet : ", stop_packet)
    #crc_byte_array = crc16.to_bytes(2, byteorder='big')
    port.write(bytes(stop_packet))



def main():

    ser = serial.Serial('COM13', 115200, timeout=0.1)
    while True:
        ser.write("This is a message from computer\n");
    
        break
    ser.close()

    return
    # total arguments
    n = len(sys.argv)
    if(n < 3) : 
        print("Please enter filename and port")
    else:
            
        binfilePath = sys.argv[1]
        port = sys.argv[2]

        baud_rate = 115200  # Adjust this to match your device's baud rate

        try:
            # Open the serial port
            ser = serial.Serial(port, baud_rate, timeout=0.1)

            wfile = open("wfile.bin","wb")

            #open binary file
            #binfile_size = os.path.getsize(binfilePath)
            
            binfile = open(binfilePath,"rb")
            binfile_content = binfile.read()
            binfile_size = len(binfile_content)
            print("Binary file size : ",binfile_size)
            print(f"Serial port {port} is open.")

            # calculate crc of the fw
            fw_crc = calculate_crc16(binfile_content)
            #fw_crc = crc16(binfile_content)
            print("FW CRC : ", fw_crc)

            # Read and print data from the serial port
            #ser.flush()
            # start ota update
            ota_send_start_command(ser)
            resp = ota_check_response(ser,CMD_START_PACKET)
            if resp != ACK:
                print(ERROR_CODES[resp])
                return -1

            # send header command 
            ota_send_header_command(ser,binfile_size,FW_TYPE,fw_crc,FW_VERSION)
            resp = ota_check_response(ser,CMD_INFO_PACKET)
            if resp != ACK:
                print(ERROR_CODES[resp])
                return -1
            
            i=0
            #send firmware 
            print("updating firmware : ", 0 , "%" )
            while True:
                if binfile_size - i >= ETX_OTA_DATA_MAX_SIZE:
                    tobesend = binfile_content[i:i + ETX_OTA_DATA_MAX_SIZE]
                    ota_send_data(ser,tobesend,ETX_OTA_DATA_MAX_SIZE,wfile)
                    

                    resp = ota_check_response(ser,CMD_FWDATA_PACKET)
                    if resp == ACK:
                        i += ETX_OTA_DATA_MAX_SIZE
                    else:
                        print(ERROR_CODES[resp])
                        return -1

                    print("updating firmware : ", int(i/ binfile_size * 100) , "%" )
                elif binfile_size - i > 0:
                    tobesend = binfile_content[i:binfile_size]
                    ota_send_data(ser,tobesend,binfile_size - i,wfile)
                 

                    resp = ota_check_response(ser,CMD_FWDATA_PACKET)
                    if resp == ACK:
                        i = binfile_size
                        print("updating firmware : ", int(i/ binfile_size * 100)  , "%" )
                        
                    else:
                        print(ERROR_CODES[resp])
                        return -1

                if i == binfile_size:
                    print("Firmware update successfull!")
                    # send stop command
                    ota_send_stop_command(ser)
                    break
        
        except serial.SerialException as e:
            print(f"Error: {e}")
        finally:
            if ser.is_open:
                ser.close()
                binfile.close()
                wfile.close()
                print(f"Serial port {port} is closed.")



if __name__=="__main__":
    main()