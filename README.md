A WinUSB driver, project for real-time, low-latency, which extracts, parses and displays MJPEG streams from arbitrary UVC USB cameras on Windows 10 using WinUSB.sys driver and API. 
The main purpose of creating my own UVC USB camera driver was that I needed precise control of the data flow, since I have another project which requires real-time low-latency AI Yolov8 inference.

Requirements:

Windows OS Version >= 8 (Since WinUSB uses DMA and DMA is supported on or above version 8).
Might need to update tune the timings (read main.cpp).
Might also need to change the alternate settings in WinUSB (read main.cpp) for your specific UVC USB camera.
You camera needs to stream MJPEG.

Demostation of the custom WinUSB camera driver:

https://youtu.be/q9qqtrwG-LU?si=HZrZJ2-SY8HN16Vj
