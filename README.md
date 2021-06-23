# UnMUTE based on Jamulus
Jamulus is an open-source software for online music sessions. Especially with the pandemic hiting the world, Jamulus is a great solution for bands and choirs to rehearse together from home over the internet. The website of Jamulus can be found at https://jamulus.io/.

We are big fans of Jamulus and wanted to add peer-to-peer audio structure. UnMUTE is the result of this effort.

## About UnMute
UnMute is a modified version of Jamulus with a peer-to-peer audio function. The peer-to-peer structure can lower audio latencies between clients. UnMute is inteded for musical groups that want to rehearse together. The person who opens a session, shares the session name with his/her peers, so they can connect to the opened session. Opened sessions are not displayed in a public list.

UnMute is designed to be run on a Raspberry Pi with a USB audio interface that is compatible with Linux.    
Our setup is a RaspberryPi 4 with a U-Phoria UM2 Behringer USB Interface.    
To test it out, you can either use our prepared image or install UnMute on a already running Raspberry Pi. Both options are explained in more details below.   

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

Once you have found out the IP of the Raspberrypi. Enter the IP in the VNC Viewer application. To login use:
- User: pi
- Password: unmute2021

When you are logged in, you can change the password of the RaspberryPi     
(Click Raspberry Symbol on top left -> Preferences -> Raspberry Pi Configuration -> Change Password)

## Run UnMUTE
Tor run UnMUTE, double-click the file called UNMUTE on the desktop. Then you can choose either "Execute" or "Execute in Terminal". Running it in terminal can be useful to get the application debug output, if something is not working.

The first window that opens up lets you choose your name and the name of the session. If you are opening the session, press "Open". If your peer has already opened a session, than enter his/her session name and press "Connect to Session". 

# Install UnMUTE on an already running RaspberryPi

## Install and Run UnMUTE
Download the UnMUTE source code and run the raspijamulus script to build UnMUTE and its dependencies:
```
git clone https://github.com/InES-HPMM/Jamulus_UnMUTE.git
cd ~/Jamulus_UnMUTE/distributions
./raspijamulus.sh
```
In the future to run jamulus just execute the unmute.sh with sudo privileges:
```
cd ~/Jamulus_UnMUTE/distributions
sudo ./unmute.sh
```

