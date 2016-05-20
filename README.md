# VoronoIoT Lamp

### Introduction:

This is a simple enclosure that turns your Particle Internet Button into a beautiful, reactive desk lamp. The enclosure is laser cut out of a black acyrlic sheet and requires some assembly.

![Step9](https://github.com/spark/vorono-iot-lamp/blob/master/images/iso-on.jpg)

### You will need:
 - The VoronoIoT Kit
 - Particle internet button with a Photon
 - Glue: hot glue/scotch tape
 - Hobby knife
 - Letter size printer paper x2
 - Patience

### Assembly Guide:

#### Step 1:

The parts come with a protective layer of paper.

![Step1](https://github.com/spark/vorono-iot-lamp/blob/master/images/top-01.JPG)

#### Step 2:

Peel off the protective paper. You'll need some patience. Don't try this on an empty stomach.

![Step2](https://github.com/spark/vorono-iot-lamp/blob/master/images/peeled.JPG)

#### Step 3:

You can use hot glue, or scotch tape to put together the lamp. Try not to use super glue since it will stain the surface very easily.

![Step3](https://github.com/spark/vorono-iot-lamp/blob/master/images/step3.jpg)

#### Step 4:

Place the Internet button on the bottom plate. You can fix it to the base using a M3 screw and nut or simply hot glue it.

![Step4](https://github.com/spark/vorono-iot-lamp/blob/master/images/step4.jpg)

#### Step 5:

In order to defuse the light, you can use a standard printing paper (letter size). Simply fold it into half and cut 0.25" from the edge to make a nice fit. You'll need to glue it place. We have also used tracing paper and acetate paper successfully.

![Step5](https://github.com/spark/vorono-iot-lamp/blob/master/images/step5.jpg)

#### Step 6:
![Step6](https://github.com/spark/vorono-iot-lamp/blob/master/images/step6.jpg)

#### Step 7:

Pass the USB cable through the opening as shown.

![Step7](https://github.com/spark/vorono-iot-lamp/blob/master/images/top-open.jpg)

#### Step 8:

Place the top plate on (don't glue it in!). Leave the inside open - it will project some cool patterns on the ceiling when the lamp turns on.

![Step8](https://github.com/spark/vorono-iot-lamp/blob/master/images/step7.jpg)

#### Step 9:

Follow the steps to program the internet button and turn it ON!

![Step9](https://github.com/spark/vorono-iot-lamp/blob/master/images/iso-on.jpg)


### Setup (programming guide):

Setting up the software side if things isn't too hard. You'll first need to get your Photon up and running, for which the "getting started" guide in the Particle documentation is great. Head over, set it up, and come back once you're Photon's breathing cyan and controllable over the web (changing the D7 LED with the Tinker phone application is a good indication). If you're facing issues, don't hesitate to ask the community, since they're extremely helpful.
Once you're Photon works and has access to the web, head over to the web IDE. The current demo does not yet take advantage of web connectivity, but this will be added soon. For now, go ahead and use the library system to add the "internetbutton" library to a new app you've created. Then, copy over the code from the main.ino file in this github repository and paste it over the contents of the new app you've made. Flash that to your device, and you should be good to go.

Pressing the buttons will give you different modes:
Button 1: "swirl mode", will allow you to tilt/rotate the lamp through the colorcircle, also changing the intensity.
Button 2: Fade trough the color circle. Pressing this button again will double the speed.
Button 3: Sunrise effect. Gently increasing brightness of orange to wake you up. pressing again will double the speed.
Button 4: Off. For those times you'd rather not have light.

### Troubleshooting:

Link to the community thread.
