# HeadTracker6DoF
Lets you track head's position and rotation through a webcam. 
It catches an image off your camera, finds your head, detects its rotation and sends the measured data to localhost via UDP.

The first minute of this video <https://www.youtube.com/watch?v=wbGGsp3OkSg> gives a good idea how does it works. 
Notice how game image moves to the sides when the guy turns his head. 
Although the guy uses different software and hardware.
*(that is a random video from youtube I found searching for "flight simulator head tracking". 
I am not affiliated with that person or channel)*

This program is supposed to be used with with either HeadTracker plugin or HeadAndHands plugin

### HeadTracker VaM plugin
This plugin only manages VaM camera: you turn your head to the left, VaM's camera turns to the left so 
the image on your display runs to the right and you see what was behind the left side of your monitor. Same with other directions.
You have to keep your eyes on the monitor. Eyes are not tracked, just head.
Be warned it is shaky [feb 2021]. I have some ideas how to fix this, but those are not implemented yet.

You can download this plugin either from a VaM Hub or from `VaM plugin` folder on this site.

### HeadAndHandsTracker VaM plugin
This will be published later.
<!---
It tracks your head exacly the same way, but instead of applying rotation to the VaM gameplay camera, 
it applies rotation to a selected atom (person). But not just rotation, it also uses detected translation moves 
, so you can move your head around a bit (within a view or your camera). 

It lets you control hands with special markers. You would need to hold that markers with your hands, so it is 
like controlling model's hands with your hands.
To make markers you need to print a .doc file, cut pieces with a pair of scissors and glues them together.
You can find instructions here: link here.

Also, HeadAndHandsTracker plugin tracks your facial metrics and lets you control eyes, brows and mouth. 
List of used morphs:

- Eyebrows inner up left, Eyebrows inner up right, Eyebrows outer up left, Eyebrows outer up right
- Eyes Closed Left, Eyes Closed Right
- Mouth Open, Mouth Smile, Mouth Width

You can download this plugin either from a VaM Hub or from `VaM plugin` folder on this site.
--->



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
