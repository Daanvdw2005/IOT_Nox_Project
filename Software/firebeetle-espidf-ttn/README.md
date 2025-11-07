# FireBeetle ESP-IDF LoRa TTN Project

This project demonstrates how to send data to The Things Network (TTN) using a FireBeetle board with LoRa capabilities. It utilizes the ESP-IDF framework for development.

## Project Structure

```
firebeetle-espidf-ttn
├── components
│   ├── lora_radio          # LoRa radio component
│   │   ├── include
│   │   │   └── lora_radio.h
│   │   ├── CMakeLists.txt
│   │   └── lora_radio.c
│   └── ttn                 # The Things Network component
│       ├── include
│       │   └── ttn.h
│       ├── CMakeLists.txt
│       └── ttn.c
├── main                    # Main application
│   ├── CMakeLists.txt
│   └── main.c
├── CMakeLists.txt          # Top-level build configuration
├── sdkconfig.defaults       # Default configuration options
├── partition_table.csv      # Flash memory partition table
├── idf_component.yml        # Component metadata
└── README.md                # Project documentation
```

## Setup Instructions

1. **Install ESP-IDF**: Follow the official ESP-IDF installation guide to set up the development environment.

2. **Clone the Repository**: Clone this repository to your local machine.

3. **Configure the Project**: Navigate to the project directory and run `idf.py menuconfig` to configure the project settings, including Wi-Fi credentials and TTN settings.

4. **Build the Project**: Use the command `idf.py build` to compile the project.

5. **Flash the Firmware**: Connect your FireBeetle board to your computer and run `idf.py -p (PORT) flash` to upload the firmware.

6. **Monitor Output**: Use `idf.py -p (PORT) monitor` to view the output from the device.

## Usage

Once the device is set up and running, it will initialize the LoRa radio and connect to The Things Network. You can modify the main application to send specific data as needed.

## Contributing

Feel free to submit issues or pull requests for improvements or bug fixes. 

## License

This project is licensed under the MIT License. See the LICENSE file for more details.