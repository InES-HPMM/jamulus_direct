# Setup RaspberryPi and Jamulus
## Overview
- [Write SD Card](#Write-SD-card)    
- [First Raspberry Pi Boot](#First-Raspberry-Pi-Boot)    
- [Option A Teamviewer](#Option-A-Teamviewer)    
- [Option B SSH](#Option-B-SSH)    
- [Install and Run Jamulus](#Install-and-Run-Jamulus)    
- [Additions](#Additions)     
  - [Set static IP](#Set-static-IP)    
  - [Enable SSH direclty on the SD before booting the Raspi](#Enable-SSH-direclty-on-the-SD-before-booting-the-Raspi)    

------------------------------------------------

## Write SD card
Install RaspberryPi Imager: https://www.raspberrypi.org/downloads/

Write SD card with Imager. As OS choose recommended Raspberry Pi OS (32-bit).

## First Raspberry Pi Boot
- Connect micro HDMI, mouse and keyboard to the raspi. Then boot it by connecting it to power.
- Usually the raspberry will login automatically. Else the default login is:
```
User: pi     
Password: raspberry
```
Run the following line to update the packages:
```
sudo apt update
```

For the future we want to be able to use the RaspberryPi from remote. You can either choose option A: Teamviewer or option B: ssh.

## Option A Teamviewer
Download, install and setup Teamviewer.
```
wget https://download.teamviewer.com/download/linux/teamviewer-host_armhf.deb
sudo apt install ./teamviewer*.deb
teamviewer setup
```
Write down the TeamviewerID and set a password. Or add the device to your teamviewer account.   
Usually the raspberry does not boot into the GUI if no display is connected. But Teamviewer needs the GUI to be startet:
```
sudo nano /boot/config.txt
```
Add the lines:
```
hdmi_force_hotplug=1
hdmi_group=2
hdmi_mode=82
hdmi_drive=2
```
With this changes, the raspi boots its display even though nothing is connected to the HDMI port.   
Reboot the raspi and connect over teamviewer.    
If you would like to change the resolution you can execute the following line at run time:
```
xrandr --size 1920x1080
```

## Option B SSH
Enable SSH in the raspi-config:
```
sudo raspi-config 
-> interfacing options 
-> SSH 
-> enable
```
Write down the IP address of the Raspberry Pi (`ifconfig` shows ip information) and reboot.
Now you can connect to the raspberry:

```
ssh -X pi@<ip-address>
```
The `-X` option makes it possible to run applications with GUI's. This is necessary to run Jamulus.    
Jamulus is run with sudo because it needs root permissions. But the x11 needs to be enabled for sudo user:
```
xauth list | grep unix`echo $DISPLAY | cut -c10-12` > /tmp/xauth
sudo su
xauth add `cat /tmp/xauth`
exit
```


## Install and Run Jamulus
Get the jamulus source code, checkout the correct branch and then run the raspijamulus.sh script, which will install and run Jamulus:
```
git clone https://github.com/corrados/jamulus.git
cd ~/jamulus/distributions
git checkout --track origin/dev-p2p
git pull
sudo raspijamulus.sh
```
In the future to run jamulus just execute the raspijamulus.sh:
```
cd ~/jamulus/distributions
sudo ./raspijamulus.sh
```
Optionally:
You could put this two lines in a bash script on your desktop to faster start jamulus in the future.
```
nano ~/Desktop/startjamulus.sh

cd ~/jamulus/distributions
sudo raspijamulus.sh

Ctrl+X

sudo chmod 777 ~/Desktop/startjamulus.sh
```
Double click the script on the desktop and choose "Execute in Terminal".

## Additions
### Set static IP
If the raspi needs a static IP, follow the instructions on: [Set-StaticIP-RaspberryPi](https://thepihut.com/blogs/raspberry-pi-tutorials/how-to-give-your-raspberry-pi-a-static-ip-address-update)


### Listen to recordings over the USB Interface
Jamulus lets you record your music session. To listen to this recordings you can use `aplay`. But the audio will be played on the default sound card which is usually the HDMI sound output. You can check all available sound cards with:
```
cat /proc/asound/cards
```
The output will look something like this:
```
 0 [b1             ]: bcm2835_hdmi - bcm2835 HDMI 1
                      bcm2835 HDMI 1
 1 [Headphones     ]: bcm2835_headphonbcm2835 Headphones - bcm2835 Headphones
                      bcm2835 Headphones
 2 [CODEC          ]: USB-Audio - USB Audio CODEC
                      Burr-Brown from TI USB Audio CODEC at usb-0000:01:00.0-1.2, full speed
```
To change the default soundcard to the headphone jack (1) or to the USB interface (2) you need to create the file `/etc/asound.conf`:
```
sudo nano /etc/asound.conf
```
And add the following lines:  
```
defaults.pcm.card 2
defaults.ctl.card 2
```
Now `aplay` will play the recordings on the USB interface. Replace both `2`s with `1`s to change it to the headphone jack.

### Enable SSH direclty on the SD before booting the Raspi
The boot partition on a Pi should be accessible from any machine with an SD card reader, on Windows, Mac, or Linux. If you want to enable SSH, all you need to do is to put a file called `ssh` in the `/boot/` directory. The contents of the file don’t matter: it can contain any text you like, or even nothing at all. When the Pi boots, it looks for this file; if it finds it, it enables SSH and then deletes the file. SSH can still be turned on or off from the Raspberry Pi Configuration application or `raspi-config`.
