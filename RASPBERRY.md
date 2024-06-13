# Setup of a Raspberry Pi

## Requirements

- Raspberry Pi 4 or 5, 4 Gb or 8 Gb
- RPi connected to the internet via WiFi
- RPi connected to a local Ethernet switch 
- Use the **Raspberry Pi Imager** tool to prepare a MicroSD Card with the image of a Rasbian OS (64 bits)
- During the image preparation, set the hostname to something meaningful (e.g. `mads-pi`), pick a user/password, and **enable SSH**
- Insert the MicroSD card into the RPi and power it up (with screen, keyboard, and mouse connected)

## Initial setup

Open a terminal and run the following commands:

```bash
sudo raspi-config
```

In the configuration tool, do the following:

* in `Interface options`, enable `SSH`, `SPI`, `I2C`, `Serial Port`, `1-Wire`
* in `System options` you can change the password and hostname, if needed
* Exit accepting changes and then reboot the RPi

In the top right of the screen, click on the network icon and connect to your WiFi network.

Then click on the same icon, select `Edit connections..`, and configure the IP address for the Ethernet wired connection (use DHCP is connected to the TCP infrastructure, or a static IP if connected to a local-only switch). 

Open a terminal and type this command:

```bash
sudo nano /etc/avahi/avahi-daemon.conf
```

Now we want to enable the Bonjour service for the RPi: this will enable to connect to the RPi using its hostname (e.g. `mads-pi.local`) instead of its IP address.

Using the editor, find the section `[publish]` and change the line `publish-workstation=no` to `publish-workstation=yes`, and the line `publish-hinfo=no` to `publish-hinfo=yes`. Save the file and exit the editor (CTRL-X, Y).

Now, from another computer connected to the same network, open a terminal and type:

```bash
ssh mads@mads-pi.local
```

If it works, you can optionally disable the grphical interface on the RPi to save resources: launch again the `raspi-config` tool and in `System options` select `Boot` and `Console Autologin`. From now on, you can use the RPi via SSH without any screen or keyboard attached.

## Programming environment

It is suggested that you use the Visual Studio Code IDE installed on your laptop to develop on the RPi. You must check that the VSCode plugins `Remote Explorer`, `Remote - SSH`, and `Remote Development` are installed.

Then click on the blue button marked `><` on the bottom left of the VSCode window and select `Connect to Host...`. Then type the RPi hostname (e.g. `mads@mads-pi.local`) and select `SSH`. You will be asked for the user and password, and then you will be connected to the RPi.

Now you can browse and edit files on the RPi, and open a terminal directly to the RPi from your laptop.

## Serial port connection

If you need to connect to an Arduino via USB UART serial port, the port address is usually `/dev/ttyACM0`. 


