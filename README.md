# VL53L1X_Oil_level_Sender
DIY Home Heating Oil Level Sender

## Warnings
This device probably voids some warrantees, will make your heating oil companies mad, and nullify your home insurance. Maybe, I'm not a laywer. Just know that installing a DIY device on your heating oil tank probably will get some eyebrows raised if anything goes wrong. Get your tank inspected regularly, do not install if you are not comfortable doing it yourself. I am not responcible for anything that goes wrong on your end. 

There are comercial devices you can buy to perform the same function as this device that are certified and approved. If you have any hesitation to a DIY solution, go buy one of those instead. 

#### Safety
Heating oil tanks undergo some large pressures during filling and there are some safety items that you do not want to interfere with. 
1. Most comercial indoor basement tanks in North America use a whistle to tell the truck driver when to stop filling with oil. This device relies on there being no open air vents other than the single vent pipe that exists next to the fill pipe. This means your sensor needs to be sealed and air tight. This requires filling the plug that is used with potting or other appropriate expoxy. 
2. Leaks. Heating oil leaks are expensive to clean up and dangerous. Your sensor needs to be built into the correct plug, and then installed with the correct thread sealant and tightened appropriately. If you do not know how to do this, consult someone that does. 
3. Pressure. Heating oil tanks are rated to 5-7psi, which sounds low, but results in a huge force on the tank walls. Loop up hoop stress if you want to know more. In a tank the size of your heating oil tank 5psi can exert hundreds of pounds of force on the shell of the tank. However at the cap we're using, because the surface area is small, the forces on the actual device are small. If you keep the hole you drill in the plug small then potting comound or expoy will have no problem with the forces that exist during filling. 
4. Electrical shorts and sparks. Home heating oil is thankfully not as volitile as other fuels, but you still absolutely do not want to be putting any kind of spark inside your tank. Any electrical device is a spark risk. Even if the sensor is running at 3.3v there are always surge risks from outside factors such as lightning strikes. This is why the design used here isolates and embedds all electronics in potting material. There should be no exposed path for any circuitry to be exposed to the atmosphere inside the tank. 

#### Device
With all those warnings out of the way, onto the device. Very simple this is nothing more than a Time of Flight distance sensor and a wifi connected microcontroller. The time of flight sensor measures the distance to the top of the oil level, and the micro controller does something with that data. In my case I'm simply pushing the distance off to a Node Red flow in it's raw reading format. However you can do all levels of data analysis on the microcontroller itself. ESP devices are more than capable of hosting a simple web page to display data. 

The oil level sender is effectively two parts. Part 1 is the sensor embedded in a standard off the shelf Black Iron plug. Part 2 is the microcontroller and housing. 

##### Sensor and Plug
You should use a real metal plug bought from your local hardware store. Do not use plastic, do not 3d print something, go buy the right part, they only cost a couple dollars. In this plug drill a hole through the top that is roughly 1/4inch diameter. 

Prepping the sensor. First print out the spacer. This is used to make the carrier board sit flat on our isolation plate. Install the lens that hopefully came with your sensor, then test fit everything into the printed spacer. 

Next wire up the sensor. These can be run with or without the Interupt and Shutdown pins. If you're trying to run multiple sensors these pins are not optional, but this device uses only one. It doesn't hurt to wire all pins and run them just in case, since you can't get at the sensor after potting. 

Cut out the isolation plate. This really should be made from metal for it's chemical resistance. The outer shape does not need to be perfect, but the cutout around the sensor should be a close fit. Take your time with a small file and get it close. 

When your parts are ready, first glue the sensor into the spacer. I used a little CCA glue just to hold it in place until the potting compound sets. 

Next apply a very small amount of potting compound around the sensor opening and press the sensor into place. Use something to hold everything together and let this set. 

Now we need to fully pot the sensor into the cap. Pass the wiring through the cap, and populate the connectors. I strongly suggest testing this now before you pot everything and lose the ability to figure out you wired the connectors wrong. Use a drop or two of CCA glue to hold the plate in place, then apply some low tac tape (i used painter tape) to seal up around the outside to hold back any potting compound that may try to leak through. 

Now it's time to inject the potting compound into the back of the sensor through the hole we drilled for wires. Take your time and inject the compound allowing it to flow and settle a little bit at a time. You want this cavity completely filled. Allow this to completely cure. 

Once everything is cured remove the tape we applied earlier. We need to fill the remaining seam around the isolation plate. Here I recomend an epoxy that is chemical resistant. Apply enough to fillet the seam between the isolation plate and plug and allow to cure completely. 

You should now have your completed sensor plug assembly. 


##### Microcontroller and Enclosure
This part is flexable. You can follow my version, or you can really attach any micrcontroller you prefer and enclose in any way you want. If you are using my design I have a few options. I've designed this around an ESP8266 D1 Mini. The ESP32 is newer and fancier, but we really don't need the increased power of the newer device. You can hand wire everything, or you can order the cheap connection PCB I have designed and linked on oshpark. All this board does is make wiring the device easier, it's not nessesary if you're comfortable hand wiring things. 

For enclosures I have three versions. Two are based around using my connector board, the other is just a bare shell that you can use for fitting your own wiring and microcontroller into. The two based around my design allow you to either expose the USB connector on the D1 mini, or install a DC power jack.

For any of the enclosures I recomend printing in either PETG or ABS/ASA. I suggest minimum 3 shells, 4 is better to make sure there is enough material around the threaded inserts. I use brass heat set inserts for all fasteners. 

Wiring is straight forward. Whether you use my carrier board or wire it all by hand the connections are the same. If you do use my carrier board you can get away with only wiring up 3.3V, ground, SCL, and SDA, though adding the Shutdown and interupt wires may let you write more complicated programs. If you do use my carrier board you have the option to wire power to a 2mm pitch JST connector, or just solder wires directly to the board at JP1. If you're exposing the USB connector then you can skip either of those as power will come from the USB. 


#### Installation
Installing the device is straightforward. You will first need to remove one of the extra plugs in the top of your tank. I recommend picking the one furthest from the fill pipe to avoid splashing when the tank is being filled. Before threading in the device, make sure to apply the appropriate pipe dope compound to the threads to make sure it seals and prevent the threads coroding. 

Thread the plug in by hand to make sure it does not cross thread, then tighten down fully. 

Make the connections to the microontroller, then place the enclosure over the plug and secure with the two set screws to hold it onto the plug. 

Lastly attach the cover and plug in. 


#### Firmware
I have provided my arduino sketch and some basic node red flows if you wish to copy my setup exactly. I won't go in depth on how to setup node red and MQTT, there are plenty of better tutorials out there than any I could write. The basic overview of my firmware is as follows. 
The microcontroller samples the distance to the oil level 5 times, then averages those readings in an attempt to rule out some measurement error. This reading is then published over MQTT. Then the device waits a set amount of time and repeats. That's basically it. I'm doing all the data analysis off device in the node red flow. The sparkfun VL53X1L library has a lot of features that could be used to calibrate offsets and then do all the math right on the microcontroller skipping the node red portion all together. I choose to do it off device because I already ahve a node red setup that is doing weather fetching and other data analysis, so it made sense to crunch numbers there. 

I've included a very basic node red flow that will capture the incoming readings, store them, and do some basic plotting and usage calculations. 
