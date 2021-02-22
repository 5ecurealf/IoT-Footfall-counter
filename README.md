# IoT-Footfall-counter
Software component of the IoT device designed for a local shopping centre. The device is a footfall counter which is to be placed in major entry/exit points and pathways within the shopping centre, it also connects to the existing staff WiFi network onsite.

At a high level of abstraction, the design can be split up into 5 subtasks working in tandem to meet the specification. These tasks include: 

1. Polling the state of the button every 5ms and count the number of low-to-high transitions. Where each low-high transition is used to simulate the detector sensing a visitor passing the sensor. 

2. Transfer the values of: 

    a. total footfall (TF). 

    b. Average footfall per minute.(AFM) 

    c. The sensors unique identifier (for localisation purposes). 

  Over the local network using HTTP and over the internet using MQTT. 

3. Resetting the total footfall counter using HTTP and MQTT. 

4. Updating the LCD every 1 second with the IP address assigned to the board and average footfall per minute. 

5. Turn the LED on if there is no WiFi connection, otherwise turn it off. 
