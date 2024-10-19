# STM32 Bootloader with USB/UART Firmware Update

🚧 Under Development
Please note: This firmware is currently in the development stage. Features may change, and the code may contain bugs. Use with caution and report any issues you encounter.

This bootloader for the STM32 microcontroller provides a robust and feature-rich solution for firmware updates. The bootloader is designed to facilitate firmware updates via the internal USB or UART interfaces, utilizing an external SPI Flash to store firmware temporarily.

![Block_Diagram](block_diagram.png)

# Key Features

- Dual Slot Firmware Storage: The bootloader manages two separate slots in external SPI Flash to store different versions of firmware. This allows for safe firmware updates with rollback capability.
  
- Firmware Rollback: In case of an update failure, the bootloader can revert to the previous working firmware, ensuring the device remains operational.
  
- CRC32 Validation: Each firmware stored in the SPI Flash is validated using CRC32 checks to ensure data integrity and prevent corrupted firmware from being executed.

- Encrypted Storage: The bootloader keeps all firmware data stored in SPI Flash in an encrypted form, enhancing security against unauthorized access.

- Version Management: The bootloader can report the current firmware version, helping in maintaining and tracking the firmware versions deployed on the device.

- External Python Script: A Python script is provided to assist with loading new firmware onto the device. This script also facilitates version reporting and other bootloader-related operations.

# Software Blocks
 1. Lock Bin (Python Software for Encryption)
   - Purpose: This Python tool encrypts the firmware (`FW FILE`) and attaches crucial metadata like firmware version, firmware name, firmware ID, build time, and date to the output file (`BIN FILE`).
   - Process: 
     - It takes inputs including the firmware version, firmware name, firmware ID, build time, and date from the user.
     - The `Lock Bin` tool combines these inputs with the firmware file and encrypts the package to ensure its security during transmission.
     - The output is a binary file (`BIN FILE`) that securely contains the encrypted firmware along with the embedded metadata.
   - Output: A secure, encrypted `.fw` file that contains all the metadata, including build time and date.

 2. Host App (Firmware Upload Application)
   - Purpose: The `Host App` is responsible for transmitting the encrypted firmware file to the microcontroller.
   - Process: 
     - It reads the encrypted `.fw` file, extracts the necessary metadata for logging or validation purposes, and establishes a connection with the microcontroller via a COM port (e.g., USB/UART).
     - The app then transfers the encrypted firmware to the bootloader running on the microcontroller.
   - Functionality: Facilitates the communication between the microcontroller and the host, ensuring that the firmware is properly uploaded.

 3. Bootloader (Running on Microcontroller)
   - Purpose: The bootloader handles the firmware update process within the microcontroller.
   - Process: 
     - It receives the encrypted firmware and metadata from the `Host App` via USB/UART.
     - The bootloader verifies the integrity and authenticity of the firmware using the metadata (version, name, FW ID).
     - It then decrypts the firmware and installs it on the microcontroller, handling any necessary checks or updates.
     - It can also maintain logs or trigger rollback if an issue occurs during the update process.
   - Responsibilities: 
     - Safely receive, verify, and install the encrypted firmware.
     - Ensure integrity and correctness based on the metadata.

This pipeline securely handles firmware updates, ensuring the integrity of the process from encryption to installation. The addition of build time and date in the `.fw` file offers further traceability for each firmware version.

# Prerequisites

- Hardware: STM32 microcontroller with USB and UART interfaces.
- External Storage: SPI Flash module connected to the microcontroller.
- Software: Python installed on your host machine for running the external script.

# Building the Bootloader

1. Clone this repository:
   ```bash
   git clone https://github.com/mayankpatel97/STM32-SPIFLASH-UART-BOOTLOADER.git
   cd STM32-SPIFLASH-UART-BOOTLOADER
   ```
2. Configure the bootloader using your preferred IDE (e.g., STM32CubeIDE) or via command line.

3. Build the bootloader project and flash it onto your STM32 microcontroller.

# Using the Bootloader

1. Connect your STM32 device to your computer via USB or UART.

2. Run the provided Python script to load new firmware:
   ```bash
   python firmware_loader.py --port /dev/ttyUSB0 --file firmware.bin
   ```

3. The script will automatically handle the transfer, validation, and version management.

4. After the firmware is successfully loaded, the bootloader will perform a CRC16 check and either boot into the new firmware or revert to the previous version if an error is detected.

# Rollback Procedure

In case of an issue with the newly loaded firmware, the bootloader automatically reverts to the previous firmware stored in the other slot, ensuring the device remains operational.

# Checking Firmware Version

To check the currently running firmware version, use the following command:
```bash
python firmware_loader.py --port /dev/ttyUSB0 --version
```

# License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

# Contributing

Feel free to contribute to this project by submitting pull requests or opening issues for any bugs or feature requests.

# Acknowledgments

Special thanks to the open-source community for providing tools and libraries that helped in the development of this bootloader.
