# HeadTracker6DoF
Lets you track head position and rotation through a webcam.

It catches an image off your camera, finds your head position, detects its rotation and sends the measured data to localhost via UDP.

## How to run

Make sure your web camera is connected.
Run file `_run.cmd`.
If you have more than one camera, run one of `"_run Camera 0.cmd"`, `"_run Camera 1.cmd"` or `"_run Camera 2.cmd"`.

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
