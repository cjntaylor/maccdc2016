MACCDC 2016 Badge
=================

# Badge Firmware

The MACCDC 2016 badge uses an ESP8266 as its primary processor and uses custom firmware to provide its functionality. This firmware is based on the Espressif SDK and its port to the Arduino wiring environment, as well as a few libraries to support the specific peripherals on the badge.

To simplify development, this project uses [PlatformIO](http://platformio.org), which downloads and manages all required toolchains and libraries, including the Arduino ESP8266 framework, across all supported platforms (Linux, OSX and Windows). Follow the instructions below to get up and running.

All badges have been flashed with the current version of the firmware and do not require any additional modification. This information is only provided for information purposes and modification after the event.

## Setup

Follow the [setup instructions for PlatformIO](http://docs.platformio.org/en/latest/installation.html) to download the required tools and libraries. Running the commands below will require internet access the first time they are run, and the system will prompt you to install additional libraries and toolchains. In order to build correctly, all libraries and toolchains must be installed.

## Build

To build the software, open a terminal on your platform and run the following in the project directory:

    platformio run

This command will prompt downloads for the required toolchains and libraries as previously mentioned. It will then build all required dependencies of the firmware, followed by the firmware itself. On a reasonably modern computer, this whole process takes about 60 seconds. If there is a problem, the system will output the issue and indicate a failure.

## Uploading

Once the firmware is built, the firmware needs to be uploaded to the microcontroller. There are two methods for doing this, via the serial bootloader or for compatible firmware (including this firmware), via an Over-The-Air (OTA) network update. In either case, the power switch MUST be on in order to upload new firmware.

### Serial Bootloader

This method must be used for the initial flash, or if OTA functionality is not configured in the firmware (intentionally or otherwise). This requires a serial UART adapter that is compatible with the 6-pin programming header located on the top right of the badge. These cables are commonly referred to as FTDI cables, due to the most common chip used for creating a USB-based serial port, but they are made by many companies. The following cable is highly recommended as it is the one used to do the initial flash, and matches the pinout of the header, and runs at the required 3.3V logic level:

[FTDI Cable](https://www.sparkfun.com/products/9717)

The cable must be attached in a specific orientation. The outermost wires are typically green and black; labels indicating which color should go on which side of the connector are etched into the bottom of the badge. Connect the cable to the badge in the correct orientation and to an available USB port on your computer. Some systems may require drivers to work with the cable to make the serial port available; refer to the manufacturers instructions for more details. If you have purchased the cable above, there are instructions available [here](https://learn.sparkfun.com/tutorials/how-to-install-ftdi-drivers).

The serial bootloader is an in-built function of the ESP8266 but it must be manually triggered using the buttons on the board. If the LED on the board is glowing brightly red upon connecting the FTDI cable, the system is in an intermittent state and should be power cycled by removing and re-inserting the cable, and toggling the power switch. If the LED is already glowing dimly red, the following instructions MUST be followed anyways. 

Each time you want to flash the board, it must FIRST be put into bootloader mode by the following process:

1. Press and hold the RESET (right) button.
2. Press and hold the GPIO0 button (left) button. An onboard LED will glow brightly red.
3. Release the RESET button while still holding the GPIO0 button.
4. Release the GPIO0 button.

If bootloader mode has been entered successfully, the red LED will glow dimly until the next flash is complete. There is no harm in entering bootloader mode multiple times; if you are unsure, follow the process again. To exit bootloader mode without flashing new firmware, just press the RESET (right) button.

The stock firmware uses the SPIFFS filesystem feature of the ESP8266. This section of the flash storage must be programmed via bootloader mode. This process uploads the files contained within the [data](data) folder to the microcontroller via a SPIFFS image and takes significant time. It only needs be done for the initial flash and any time files in the [data](data) folder are added, deleted or modified. The firmware itself can be flashed by itself without this process if nothing has changed since the last time the process was run.

To upload the SPIFFS image, run the following in a terminal:

    platformio run -t uploadfs
    
While firmware is flashing, a blue LED on the board will flash quickly a few times, turn off, and then flash continuously while the image is written. When the led stops flashing, the board will automatically reset (exiting bootloader mode) and start up the new image.

Once the SPIFFS image has been flashed, the firmware itself must be flashed. Make sure the board is running in bootloader mode, and run the following in a terminal:

    platformio run -t upload
    
Once these two steps have been completed, the firmware has been fully flashed to the board.

### OTA Update

The Over-the-Air update functionality is a secondary method for uploading firmware without the need for connecting the system via an FTDI cable, but is not as reliable and requires specific support in the firmware. This functionality is provided by a Arduino framework library and must be embedded within the firmware AND any updates to that firmware, or the functionality no longer works. The stock firmware for the badge uses a custom extended OTA system to try to ensure that OTA functionality always available, even if networking is configured incorrectly.

OTA functionality can be intentionally or unintentionally disabled if the firmware that is flashed via this or the serial method does not enable it correctly. For this reason, please mind the comments in the source code to try to prevent this case. It is recommended to use the stock firmware as a template and to only modify specific sections of the code to add new or update functionality.

OTA updates push the same SPIFFS and firmware images used in the serial method by transmitting the data over the network directly to the ESP8266. This requires knowing the IP address of the badge (once booted, displayed in the upper left of the OLED), and having a network route on your development machine to the same network that the badge is on (being connected to the same network). For the following instructions, the badge IP address will be referred to as <BADGE_IP>.

If the badge configuration is incorrect and will not allow the badge to connect to its configured network, or would otherwise fail to startup correctly, the stock firmware boots into "Setup Mode," which should always allow the system to be flashed via OTA updating. The system will start in access point mode as an open network with the SSID "esp8266-<chip_id>", which is displayed on the OLED. Connect your development machine to this wireless network; an IP address will be assigned to your machine via DHCP. Use the displayed IP, the AP gateway as the <BADGE_IP> address. For more information about configuring the badge network, see the section below on [Configuring the badge Network](#network).

OTA updates **DO NOT** require and you **SHOULD NOT** put the ESP into bootloader mode. This method will not function in bootloader mode - the board must be up and running and fully connected to a network or running in "Setup Mode"; it must have a <BADGE_IP>.

To upload the SPIFFS image, run the following:

    platformio run -t uploadfs --upload-port <BADGE_IP>
    
To upload the firmware image, run the following:

    platformio run -t upload --upload-port <BADGE_IP>
    
Since this process can ONLY be used AFTER the first flash (true for all badges), the SPIFFS image need / should only be run if files inside of [data](data) have been added, deleted or modified. The firmware can be uploaded anytime the system has booted fully (has a correct configuration) and a <BADGE_IP> is set.

Uploading OTA is significantly faster than the serial method and is recommended if possible. Progress will be shown in the terminal and on the screen in the form of an OTA update message. When OTA updating is complete, the badge will automatically reboot and use the new firmware or SPIFFS image.

## Modifying the Firmware

The code has been marked with sections that should be retained or can be removed to allow modification without disrupting the OTA functionality. To prevent loss of OTA programming, please heed these section labels. This firmware includes libraries used to simplify programming of the two hardware devices connected to the board, the Neopixel Stick and the OLED. More information and documentation can be found here:

* [Arduino Framework (core library)](https://github.com/esp8266/Arduino)
* [NeoPixelBus](https://github.com/Makuna/NeoPixelBus)
* OLED
	* [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
	* [Adafruit SSD1306 Library](https://github.com/adafruit/Adafruit_SSD1306)

# Configuring the badge Network <a name="network"></a>

The [data/wifi.json](data/wifi.json) file controls which network the badge will attempt to connect to. The structure is as follows:

```json
{
  "ssid": "<SSID>",
  "pass": "<WPA/WPA2 PASSWORD>"
}
```

The only required key is `ssid`, which denotes the network to connect to. If `pass` is provided, the system will attempt to connect with authentication to the named `ssid` network. If it is omitted, the badge assumes `ssid` is an open network.

This configuration **MUST** be valid JSON. If JSON formatting is not followed exactly, the badge will not boot correctly and shift to the failsafe "Setup Mode".

The configuration can be updated by modifying the file, and uploading via one of the SPIFFS image flashing methods above. Please note, once the SPIFFS image is reflashed, the badge will attempt to connect to the updated network on the defined connection information. If this configuration is incorrect or fails to connect, the system will switch to "Setup Mode" to allow the configuration to be reflashed by directly connecting to the badge AP.
