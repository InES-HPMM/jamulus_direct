# Jamulus UnMUTE
Jamulus is an open-source software for online music sessions. Especially with the pandemic hiting the world, Jamulus is a great solution for bands and choirs to rehearse together from home over the internet. The website of Jamulus can be found at https://jamulus.io/.

We are big fans of Jamulus and wanted to add peer-to-peer audio structure. Jamulus UnMUTE is the result of this effort.

# About UnMute
UnMute is a modified version of Jamulus with a peer-to-peer audio function. The peer-to-peer structure can lower audio latencies between clients. UnMute is inteded for musical groups that want to rehearse together. The person who opens a session, shares the session name with its peers, so they can connect to the opened session. Opened sessions are not displayed in a list.

UnMute is designed to be run on a Raspberry Pi. To test it out, you can either use our prepared image or install UnMute on a already running Raspberry Pi. Both options are explained in more details below.   

- [Setup UnMUTE Image on a RaspberryPi](#Setup-UnMUTE-Image-on-a-RaspberryPi)
- [Install UnMUTE on an already running RaspberryPi](#Install-UnMUTE-on-an-already-running-RaspberryPi)

# Setup UnMUTE Image on a RaspberryPi
- [1 Write SD Card](#Write-SD-card)    
- [2 Connect to RaspberryPi with VNC Viewer](#Connect-to-RaspberryPi-with-VNC-Viewer)    

------------------------------------------------

## Write SD card
Download the UnMUTE Image:

Write the image on an SD Card. For this you can use the RaspberryPi Imager: https://www.raspberrypi.org/downloads/

After installing the RaspberryPi Imager, start the application. In "CHOOSE OS" select "Use custom" and then find the downloaded image.

When the imager is done, remove the SD card from the computer and put it into the RaspberryPi.

Boot the RaspberryPi.

## Connect to RaspberryPi with VNC Viewer
To use the RaspberryPi, one can simply connect a display, keyboard and mouse to the Pi. But since this setup is not very handy, we suggest connecting to the RaspberryPi with VNC Viewer. VNC Viewer can be installed on a Laptop/Computer with Windows, MacOS and Linux (https://www.realvnc.com/de/connect/download/viewer/). Or install it on your phone over the App Store on Android or iOS.

VNC Viewer needs the IP address of the RaspberryPi to connect. If you don't know the IP of the RaspberryPi you can find it out with:
- On Windows: Install and run Bonjour Print Services https://support.apple.com/kb/DL999?locale=en_US
- Android Phone: Install and run BonjourBrowser Application from the Play Store
- Apple: TODO
- Linux: In a terminal run: `ping raspberrypi.local`


### Enable SSH direclty on the SD before booting the Raspi
The boot partition on a Pi should be accessible from any machine with an SD card reader, on Windows, Mac, or Linux. If you want to enable SSH, all you need to do is to put a file called `ssh` in the `/boot/` directory. The contents of the file don’t matter: it can contain any text you like, or even nothing at all. When the Pi boots, it looks for this file; if it finds it, it enables SSH and then deletes the file. SSH can still be turned on or off from the Raspberry Pi Configuration application or `raspi-config`.


# Install UnMUTE on an already running RaspberryPi




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
