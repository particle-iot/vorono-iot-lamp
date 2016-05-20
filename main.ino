SYSTEM_THREAD(ENABLED);

#define OFFLINE_MODE

// This #include statement was automatically added by the Particle IDE.
#include "InternetButton/InternetButton.h"

#ifndef OFFLINE_MODE
// This #include statement was automatically added by the Particle IDE.
#include "WebServer/WebServer.h"

// This #include statement was automatically added by the Particle IDE.
#include "MDNS/MDNS.h"
#endif

#include "math.h"


// Uncomment to enable debugging to serial
// #define SERIAL_DEBUG


// #define SETTINGS_URL    "http://jordymoors.nl/lamp/webpage"
#define SETTINGS_URL    "http://spark.wgb.me/voroniot"

#ifndef OFFLINE_MODE
MDNS mdns;

// Web server settings
#define WEBDUINO_FAVICON_DATA ""
#define WEBDUINO_FAIL_MESSAGE ""
WebServer webserver("", 80);
#endif

#define POST_NAME_LENGTH    32
#define POST_VALUE_LENGTH   32


InternetButton b = InternetButton();

int baseX = 0;
int baseY = 0;
int baseZ = 0;

int calibrateTime = 2;
bool readMode = false;
int readLoop = 0;
int oldAngleDeg = 0;
int oldDegrees = 0;

float xAxis = 0;
float yAxis = 0;
float zAxis = 0;
float xAxis2 = 0;
float yAxis2 = 0;
float zAxis2 = 0;
float sumAxis = 0;
float angleRads = 0;
float angleDeg = 0;
// float intensity = 0;

float rads = 0;
float degrees = 0;

bool returnedHome = true;
int readReturnHomeLoop = 0;
bool white = false;

// float hue = 0;

// int rgb[3];

// Settings
struct structSettings {
    //bool fade = 0;
    float hue = 0;
    float intensity = 1.0;
    int rgb[3] = {0, 200, 255};
    int timerVal = 10000;
    char share_code[9] = "DEADBEEF";
};

structSettings Settings;


float sunIntensity = 0.0;
bool sun_dir = true;


// Function prototypes
void fade();
void eepromSave();
void sun();
void pubcolor();
void hsi2rgb(float H, float S, float I, int* rgb);
void measureMovement();
int colors(String command);
int handleParams(String command);
int fadeStart(String command);
void eepromLoad();
void subscribe_handler(const char *event, const char *data);
void tilt_a_whirl();


// Subscription management helpers
bool has_been_subscribed = false;
String last_share_code = "";
uint8_t EFFECT_MODE = 0;
uint8_t LAST_EFFECT_MODE = EFFECT_MODE;


#ifndef OFFLINE_MODE
// index.html
void web_index(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
    server.httpSuccess();

    server.print("<!DOCTYPE html><html><head><title>VoronIoT Lamp Settings</title><style type=\"text/css\">html,body,iframe{position:absolute;top:0;right:0;bottom:0;left:0;border:0;width:99%;height:99%;}</style></head><body><iframe src=\""+String(SETTINGS_URL)+"/index.html?ip="+String(WiFi.localIP())+"\"></iframe></body></html>");
}


// settings.json
void web_settings(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
    server.httpSuccess("application/json");

    server.print("\
{\
\"h\":"+String(Settings.hue)+",\
\"i\":"+String(Settings.intensity)+",\
\"t\":"+String(Settings.timerVal)+",\
\"rgbR\":"+String(Settings.rgb[0])+",\
\"rgbG\":"+String(Settings.rgb[1])+",\
\"rgbB\":"+String(Settings.rgb[2])+",\
\"share\":\""+String(Settings.share_code)+"\"\
}");
}


// save.html
void web_save(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
    URLPARAM_RESULT rc;
    char name[POST_NAME_LENGTH];
    char value[POST_VALUE_LENGTH];


    server.httpSeeOther(String(SETTINGS_URL)+"/index.html?ip="+String(WiFi.localIP()));

    while(server.readPOSTparam(name, POST_NAME_LENGTH, value, POST_VALUE_LENGTH)) {
        String _name = String(name).toUpperCase();
        String _value = String(value);

        if(_name.equals("H"))
            Settings.hue = _value.toInt();

        else if(_name.equals("I"))
            Settings.intensity = _value.toInt();

        else if(_name.equals("T"))
            Settings.timerVal = _value.toInt();

        else if(_name.equals("RGBR"))
            Settings.rgb[0] = _value.toInt();

        else if(_name.equals("RGBG"))
            Settings.rgb[1] = _value.toInt();

        else if(_name.equals("RGBB"))
            Settings.rgb[2] = _value.toInt();

        else if(_name.equals("SHARE"))
            _value.toUpperCase().toCharArray(Settings.share_code, 9);

        else if(_name.equals("SAVE"))
            if (_value.toInt() == 1)
                // Save the updated settings
                eepromSave();
    }

    eepromSave();
    b.allLedsOn(Settings.rgb[0], Settings.rgb[1], Settings.rgb[2]);
}
#endif


// Timers
Timer timerFade(10000, fade);
Timer timerSunrise(18000, sun);
uint32_t periodFade = 10000;
uint32_t periodSunrise = 18000;


void setup()
{
    Particle.function("colors", colors);
    Particle.function("params", handleParams);

    // Use b.begin(1); if you have the original SparkButton, which does not have a buzzer or a plastic enclosure
    b.begin();

#ifdef SERIAL_DEBUG
    Serial.begin(9600);
#endif


    // See if this EEPROM has saved data
    if(EEPROM.read(0)==1) {
        eepromLoad();
    }
    // If data has not been saved, "initialize" the EEPROM
    else {
        eepromSave();
    }

    b.allLedsOff();
    b.allLedsOn(Settings.rgb[0], Settings.rgb[1], Settings.rgb[2]);

#ifndef OFFLINE_MODE
    // mDNS / Bonjour
    bool mdns_success = mdns.setHostname("lamp");

    if(mdns_success) {
        mdns.addService("tcp", "http", 80, "VoronoIoT");
        mdns.begin();
    }

    // Web server
    webserver.setDefaultCommand(&web_index);
    webserver.addCommand("save.html", &web_save);
    webserver.addCommand("settings.json", &web_settings);
    webserver.begin();
#endif
}


void loop()
{
    // If we're connected but haven't yet subscribed,
    // call "eepromLoad()" to begin the subscription
    if(Particle.connected() && !has_been_subscribed) {
        eepromLoad();
    }


    // Handle button presses
    // 12:00 = tilt_a_whirl
    if(b.buttonOn(1)) {
        b.ledOff(0); b.ledOff(3); b.ledOff(6); b.ledOff(9);
        delay(250); // Debounce
        b.allLedsOn(Settings.rgb[0], Settings.rgb[1], Settings.rgb[2]);

        EFFECT_MODE = 0;
    }

    // 3:00 = fade
    if(b.buttonOn(2)) {
        b.ledOff(0); b.ledOff(3); b.ledOff(6); b.ledOff(9);
        delay(250); // Debounce
        b.allLedsOn(Settings.rgb[0], Settings.rgb[1], Settings.rgb[2]);

        EFFECT_MODE = 1;

        // Faster!  Faster!
        if(LAST_EFFECT_MODE==EFFECT_MODE) {
            periodFade /= 2;

            if(periodFade<2)
                periodFade = 10000;

            timerFade.changePeriod(periodFade);
        }
    }

    // 6:00 = sun
    if(b.buttonOn(3)) {
        b.ledOff(0); b.ledOff(3); b.ledOff(6); b.ledOff(9);
        delay(250); // Debounce
        b.allLedsOn(Settings.rgb[0], Settings.rgb[1], Settings.rgb[2]);

        EFFECT_MODE = 2;

        // Faster!  Faster!
        if(LAST_EFFECT_MODE==EFFECT_MODE) {
            periodSunrise /= 2;

            if(periodSunrise<2)
                periodSunrise = 18000;

            timerSunrise.changePeriod(periodSunrise);
        }
    }

    // 9:00 = off
    if(b.buttonOn(4))
        EFFECT_MODE = 3;


    // Do something if the effect mode changed
    if(EFFECT_MODE!=LAST_EFFECT_MODE) {
        switch(EFFECT_MODE) {
            case 1: // Fade
                timerSunrise.stop();
                timerFade.start();
                timerFade.changePeriod(Settings.timerVal);
                break;

            case 2: // Sunrise
                timerFade.stop();
                timerSunrise.start();
                sunIntensity = 0;
                sun_dir = true;
                timerSunrise.changePeriod(Settings.timerVal);
                break;

            case 3: // Turn everything off
                b.allLedsOff();

            default: // Disable timers for "tilt-a-whirl" and "off" modes
                timerFade.stop();
                timerSunrise.stop();
                //b.allLedsOn(255, 50, 0);
                break;
        }

        LAST_EFFECT_MODE = EFFECT_MODE;
    }


    // This needs to be called every loop to detect movement
    if(EFFECT_MODE==0)
        tilt_a_whirl();

#ifndef OFFLINE_MODE
    mdns.processQueries();

    char web_buff[64];
    int web_len = 64;
    webserver.processConnection(web_buff, &web_len);
#endif

    Particle.process();
    delay(10);
}


void tilt_a_whirl()
{
    measureMovement();

    if (angleDeg < 5 && !readMode && !returnedHome) {
        readReturnHomeLoop++;
        if (readReturnHomeLoop > 14){
            returnedHome = true;
            readReturnHomeLoop = 0;

            b.playNote("G3",8);
            eepromSave();
            pubcolor();
        }

    }
    else {
        readReturnHomeLoop = 0;
    }

    if (angleDeg > 15 && !readMode && returnedHome) {
        readMode = true;
        oldAngleDeg = 0;
        oldDegrees = 0;
        readLoop = 0;
    }

    if (readMode) {
        hsi2rgb(degrees, 1, Settings.intensity, Settings.rgb);
        b.allLedsOn(Settings.rgb[0], Settings.rgb[1], Settings.rgb[2]);

        if ((abs(angleDeg - oldAngleDeg) < 5 && abs(degrees - oldDegrees) < 5) || angleDeg < 5 ) {
            readLoop++;

            if (readLoop == 14) {
                readMode = false;
                returnedHome = false;

                if (angleDeg < 5) {
                    white = !white;
                    Settings.rgb[0] = white ? 255 : 0;
                    Settings.rgb[1] = white ? 255 : 0;
                    Settings.rgb[2] = white ? 255 : 0;
                    Settings.intensity = 1;
                }
                else {
                    hsi2rgb(degrees, 1, Settings.intensity, Settings.rgb);
                    white = true;
                }

                b.playNote("G3",8);

                b.allLedsOn(Settings.rgb[0],Settings.rgb[1],Settings.rgb[2]);

                readLoop = 0;
            }
        }
        else
        {
            oldDegrees = degrees;
            oldAngleDeg = angleDeg;
            readLoop = 0;
        }
    }

    delay(40);
}


void sun()
{
    if(sun_dir)
        sunIntensity += 0.01;
    else
        sunIntensity -= 0.01;

    hsi2rgb(30.0, 1.0, sunIntensity, Settings.rgb);
    b.allLedsOn(Settings.rgb[0], Settings.rgb[1], Settings.rgb[2]);

    // If we've maxed out (or minned out?), change the direction
    if(sunIntensity>=0.99)
        sun_dir = false;
    else if(sunIntensity<=0.01)
        sun_dir = true;
}


void fade()
{
    Settings.hue++;

    if (Settings.hue == 360) {
        Settings.hue = 0;
    }

    hsi2rgb(Settings.hue, 1, 1, Settings.rgb);
    b.allLedsOn(Settings.rgb[0], Settings.rgb[1], Settings.rgb[2]);
}


void calibrate()
{
    for (int i = 0; i < 40 * calibrateTime; i++) {
        baseX += b.readX16();
        baseY += b.readY16();
        baseZ += b.readZ16();
        delay(25);
    }

    baseX /= 40 * calibrateTime;
    baseY /= 40 * calibrateTime;
    baseZ /= 40 * calibrateTime;
}


void measureMovement()
{
    xAxis = b.readX16();
    yAxis = b.readY16();
    zAxis = b.readZ16();

    xAxis2 = xAxis * xAxis;
    yAxis2 = yAxis * yAxis;
    zAxis2 = zAxis * zAxis;
    sumAxis = sqrt(xAxis2+yAxis2+zAxis2);

    angleRads = acos(zAxis/sumAxis);
    angleDeg = angleRads*180/M_PI;

    Settings.intensity = map(angleDeg, 5,50,0,1000);
    Settings.intensity = constrain(Settings.intensity, 0, 1000)/1000;

    rads = atan2(b.readY16(),b.readX16());
    degrees = rads*180/M_PI;
    // Serial.println(degrees);

    if (degrees < 0) {
        degrees += 360;
    }

    //Serial.println(angleDeg);
    //Serial.println(degrees);
}


void hsi2rgb(float H, float S, float I, int* rgb)
{
  int r, g, b;
  H = fmod(H,360); // cycle H around to 0-360 degrees
  H = 3.14159*H/(float)180; // Convert to radians.
  S = S>0?(S<1?S:1):0; // clamp S and I to interval [0,1]
  I = I>0?(I<1?I:1):0;


  // Math! Thanks in part to Kyle Miller.
  if(H < 2.09439) {
    r = 255*I/3*(1+S*cos(H)/cos(1.047196667-H));
    g = 255*I/3*(1+S*(1-cos(H)/cos(1.047196667-H)));
    b = 255*I/3*(1-S);
  } else if(H < 4.188787) {
    H = H - 2.09439;
    g = 255*I/3*(1+S*cos(H)/cos(1.047196667-H));
    b = 255*I/3*(1+S*(1-cos(H)/cos(1.047196667-H)));
    r = 255*I/3*(1-S);
  } else {
    H = H - 4.188787;
    b = 255*I/3*(1+S*cos(H)/cos(1.047196667-H));
    r = 255*I/3*(1+S*(1-cos(H)/cos(1.047196667-H)));
    g = 255*I/3*(1-S);
  }

  rgb[0]=r;
  rgb[1]=g;
  rgb[2]=b;
}


int colors(String command)
{
    char charBuf[20];
    command.toCharArray(charBuf, 20);

    // Begin black magic supplied by @mdma at:
    // https://community.spark.io/t/gpio-control-via-gui/6310/7
    const int value_count = 8;  // the maximum number of values
    int values[value_count];    // store the values here

    char string[20];
    strcpy(string, charBuf);  // the string to split
    int idx = 0;
    for (char* pt=strtok(string, ","); pt && idx<value_count; pt=strtok(NULL, ",")) {
       values[idx++] = atoi(pt);
    }
    // End black magic.

    Settings.rgb[0] = values[0];
    Settings.rgb[1] = values[1];
    Settings.rgb[2] = values[2];


    b.allLedsOn(Settings.rgb[0],Settings.rgb[1],Settings.rgb[2]);
    white = true;

    EFFECT_MODE = 0;

    return 0;
}


void eepromLoad()
{
    EEPROM.get(1, Settings);
    // String("DEADBEEF").toCharArray(Settings.share_code, 9);


    if(Particle.connected() && !String(Settings.share_code).equalsIgnoreCase(last_share_code)) {
        Particle.unsubscribe();
        Particle.publish("ip_address", String(WiFi.localIP()), 60, PRIVATE);
        Particle.subscribe(String(Settings.share_code), subscribe_handler);

#ifdef SERIAL_DEBUG
        Serial.println("SUBSCRIBED!");
#endif

        has_been_subscribed = true;
        last_share_code = Settings.share_code;
    }
}


void eepromSave()
{
    EEPROM.update(0, 1);
    EEPROM.put(1, Settings);
    eepromLoad();
}


// this function automagically gets called upon a matching POST request
int handleParams(String command)
{
  //look for the matching argument <-- max of 64 characters long
  int p = 0;

  bool setNewColor = false;

  while (p<(int)command.length()) {
    int i = command.indexOf(',',p);

    if (i<0)
        i = command.length();

    int j = command.indexOf('=',p);

    if (j<0)
        break;

    String key = command.substring(p,j).toUpperCase();
    String value = command.substring(j+1,i);
    int val = value.toInt();


    if (key=="FADE") {
        EFFECT_MODE = 1;
        val == 1 ? timerFade.changePeriod(Settings.timerVal) : timerFade.stop();
    }

    // Red
    else if (key=="R") {
      Settings.rgb[0] = val;
      setNewColor = true;
    }

    // Green
    else if (key=="G") {
      Settings.rgb[1] = val;
      setNewColor = true;
    }

    // Blue
    else if (key=="B") {
      Settings.rgb[2] = val;
      setNewColor = true;
    }


    else if (key=="TIMER") {
      Settings.timerVal = val;
      timerFade.changePeriod(Settings.timerVal);
    }


    else if (key=="SAVE") {
      eepromSave();
    }


    else if (key=="RESET") {
       b.allLedsOff();
       System.reset();
    }

    else if (key=="SUN") {
        EFFECT_MODE = 2;
        sunIntensity = 0;
        int interval = (val * 600);
        val > 0 ? timerSunrise.changePeriod(interval) : timerSunrise.stop();
    }

    p = i+1;
  }

  if (setNewColor) {
      EFFECT_MODE = 0;
      b.allLedsOn(Settings.rgb[0], Settings.rgb[1], Settings.rgb[2]);
      setNewColor = false;
      pubcolor();
  }

  return 1;
}


//Convert color values to JSON string and publish them.
void pubcolor()
{
    // Don't publish all off
    if(Settings.rgb[0]==0 && Settings.rgb[1]==0 && Settings.rgb[2]==0)
        return;

    // Don't publish all on
    if(Settings.rgb[0]==255 && Settings.rgb[1]==255 && Settings.rgb[2]==255)
        return;


    // sprintf(pubstring, "{\"r\": %u, \"g\": %u, \"b\": %u}", Settings.rgb[0], Settings.rgb[1], Settings.rgb[2]);
    String pubstring = "VORONIOT:"+String(Settings.rgb[0])+"|"+String(Settings.rgb[1])+"|"+String(Settings.rgb[2]);
    Particle.publish(String(Settings.share_code), pubstring, PRIVATE);

#ifdef SERIAL_DEBUG
    Serial.print("PUBLISH: ");
    Serial.print(String(Settings.share_code));
    Serial.print(" ");
    Serial.println(String(pubstring));
#endif
}


void subscribe_handler(const char *event, const char *data)
{
#ifdef SERIAL_DEBUG
    Serial.print("RECEIVED: ");
    Serial.print(event);
    Serial.print(" ");
    Serial.println(data);
#endif

    // This makes it much easier to parse
    String d = String(data);


    // Check to make sure we have a sting of the correct length
    if(d.length()<14 || d.length()>20) {
#ifdef SERIAL_DEBUG
        Serial.print(" x Invalid length (");
        Serial.print(d.length());
        Serial.println(")");
#endif
        return;
    }

    // Check that it begins with the "VORONIOT:" preamble
    if(!d.startsWith("VORONIOT:")) {
#ifdef SERIAL_DEBUG
        Serial.println(" x Invalid preamble");
#endif
        return;
    }


    // If we've made it this far, begin parsing!
    // Remove the preamble
    d = d.replace("VORONIOT:", "");

    // Get red
    String red = d.substring(0, d.indexOf("|"));
    d = d.substring(d.indexOf("|")+1);

    // Get green
    String green = d.substring(0, d.indexOf("|"));
    d = d.substring(d.indexOf("|")+1);

    // Get blue
    String blue = d;


    // Don't act on all off
    if(red.toInt()==0 && green.toInt()==0 && blue.toInt()==0) {
#ifdef SERIAL_DEBUG
        Serial.println(" x Not turning to all off");
#endif
        return;
    }

    // Don't act on all on
    if(red.toInt()==255 && green.toInt()==255 && blue.toInt()==255) {
#ifdef SERIAL_DEBUG
        Serial.println(" x Not turning to all on");
#endif
        return;
    }


    // Assign the string values to their integer settings
    Settings.rgb[0] = red.toInt();
    Settings.rgb[1] = green.toInt();
    Settings.rgb[2] = blue.toInt();

#ifdef SERIAL_DEBUG
    Serial.print("NEW VALUES: ");
    Serial.print(Settings.rgb[0]);
    Serial.print(", ");
    Serial.print(Settings.rgb[1]);
    Serial.print(", ");
    Serial.println(Settings.rgb[2]);
#endif

    b.allLedsOn(Settings.rgb[0], Settings.rgb[1], Settings.rgb[2]);
}