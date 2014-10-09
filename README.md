gaze_tracker_win
================
Hi everyone!

In this repository you can find the windows side server code for getting minimal fixated gaze data from the Tobii EyeX Controller, and sending through TCP/IP to an ubuntu pc.

##Setup
1. Make shure, that you have the [Tobii EyeX Controller](http://www.tobii.com/en/eye-experience/buy/) and you have installed the EyeX Engine
2. Install [Microsoft Visual Studio Express](http://www.visualstudio.com/en-us/products/visual-studio-express-vs.aspx)
3. Go to (http://developer.tobii.com/) and download the EyeX SDK for C++.

##build
~~~
1. Open TobiiEyeXSdk-cpp/samples/MinimalFixationDataStream/MinimalFixationDataStream.c
2. Copy & paste the [MinimalFixationDataStream.c](https://github.com/DaGizi/gaze_tracker_win/blob/master/MinimalFixationDataStream.c)
3. build & run directly from visual studio
