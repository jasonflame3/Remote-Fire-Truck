# Remote-Fire-Truck
This project creates a prototype of a remote controlled vehicle that will assist Firefighters in accessing dangerous areas, to put out the source of fires, and to find civilians.


## Parts list
* 2 Lipo Batteries
* ESP32-WROOM-32D Microcontroller
* DC to DC Buck Converter
* TB6612FNG Dual DC Stepper Motor Driver Module
* RC Tank Frame (with motors included)
* Wireless Camera
* Laser Module
* Small Pressure Washer Pump
* Bluetooth Controller
* Water Container
* 5V Relay Switch 

## Software Description
The microprocessor is running an operating system with two tasks. One task receives a signal from a bluetooth controller as an object from the top of the protocol stack, and interprets the controls by changing global variables. The second task interprets the global variables and sends the appropriate signals to the motor controller and other I/O devices. 

## Challenges
There were two major challenges we encountered as we developed our prototype. The first was getting our bluetooth to connect. We tried a couple hardware configurations but all failed to properly control the external bluetooth chip. We then chose to use a micro controller with a built in wifi signal and found some standard drivers that allowed us to easily connect to the ps4 controller. The Second major issue we ran into was burning out our motor controller. We resolved this by keeping our wiring in the controlled environment of a breadboard. 

## Future improvements
* Pressurized/Improved water tank
* Sensors (Temperature, Flame, and Gas)
* PCB to connect the electrical components. 
* Waterproofing
* Heatproofing
