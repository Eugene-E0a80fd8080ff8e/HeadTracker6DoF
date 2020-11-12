# HeadTracker6DoF
Lets you track head position and rotation through a webcam.

It catches an image off your camera, finds your head position, detects its rotation and sends the measured data to localhost via UDP.

## How to download and run

On releases page (<https://github.com/Eugene-E0a80fd8080ff8e/HeadTracker6DoF/releases>) select a latest release, open `Assets` section and download **HeadTracker6DoF.zip**. 
Unzip it.

Make sure your web camera is connected.
Run file `_run.cmd` or one of other *_run...* files, selecting your camera and resolution according to file name.

This will let you see yourself with head tracking information presented on the picture.

Now, return to Virt-a-Mate, open sessions plugings section and add the HeadTracker6DoF.cs plugin. It should run fine with default settings.

## Opentrack integration

As of v0.3 an integration to opentrack was added. To use it, run opentrack with its output set to "UDP over network" with ip 127.0.0.1 and port 62731. (The IP address is given with assumption you run opentrack on the computer you run VaM).

If you wondering what opentrack is, you may want to watch a video on how opentrack is used in flight simulators: <https://www.youtube.com/watch?v=LPlahUVPx4o> .

## How to compile

1. Make sure you have Visual Studio installed. 

   I used Visual Studio 2019 Community Edition (16.6.3)

2. Make sure your Visual Studio has C++ support in place together with Windows SDK.

   For this you need to open VS Installer and put few ticks here and there.

3. Download a copy of OpenCV. It would be an .exe file named like `opencv-4.4.0-vc14_vc15.exe`. Unzip it to `C:\lib\`.
   If you choose to unzip it elsewhere, you will need to change project setting to point to a new location.

   I used OpenCV version 4.4.0

4. Download a copy of Dlib. In my case it was a zip file `dlib-19.21.zip`. Similarly, unzip it to `C:\lib\`.

   I used Dlib version 19.21

5. Open the project in Visual studio, switch target to *Release*, and compile.

### Dependencies
- OpenCV <https://opencv.org/>
- Dlib <http://dlib.net/>
