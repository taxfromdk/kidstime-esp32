

//#include <NTPClient.h>
#include <ArduinoOTA.h>
#include <PxMatrix.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "Adafruit_GFX.h"
#include "daynight.h"
#include "ezTime.h"

//const long utcOffsetInSeconds = 3600*2;


#ifndef STASSID
#define STASSID "linksys1338"
#define STAPSK  "rokketand"
#endif

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


const char * ssid = STASSID; // your network SSID (name)
const char * pass = STAPSK;  // your network password

char disp[100];

WiFiUDP ntpUDP;

//NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);



#include <Ticker.h>
Ticker display_ticker;
#define P_LAT 16
#define P_A 5
#define P_B 4
#define P_C 15
#define P_D 12
#define P_E 0
#define P_OE 2

#define matrix_width 32
#define matrix_height 32
#define DON 1

uint8_t display_draw_time=7; //10-50 is usually fine


PxMATRIX display(matrix_width,matrix_height,P_LAT, P_OE,P_A,P_B,P_C);

// Some standard colors
uint16_t myRED = display.color565(255, 0, 0);
uint16_t myGREEN = display.color565(0, 255, 0);
uint16_t myBLUE = display.color565(0, 0, 255);
uint16_t myWHITE = display.color565(255, 255, 255);
uint16_t myYELLOW = display.color565(255, 255, 0);
uint16_t myCYAN = display.color565(0, 255, 255);
uint16_t myMAGENTA = display.color565(255, 0, 255);
uint16_t myBLACK = display.color565(0, 0, 0);

// ISR for display refresh
void display_updater()
{
  display.display(display_draw_time);
}

void display_update_enable(bool is_enable)
{

  if (is_enable)
    display_ticker.attach(0.002, display_updater);
  else
    display_ticker.detach();
}




/**************************************
 * 
 * Game of life
 * 
 *************************************/


#define worldHeight 32
#define worldWidth 32
int ix = 0;
unsigned char world[32][32][2];
int iterations = 0;
int stalls = 0;

int wrap(int i, int v)
{
  while(i >= v)
  {
    i -= v;
  }
  while(i < 0)
  {
    i += v;
  }
  return i;
}

bool updateWorld()
{
  iterations++;
  bool r = false;
  int next = (ix + 1) % 2;
  
  bool me;
  int neighbours;
  int x, xx, xxx;
  int y, yy, yyy;

  for (y = 0; y < worldHeight; y++)
  {
     for (x = 0; x < worldWidth; x++)
     {
        me = world[y][x][ix] > 0 ;
        
        //Count neighboure
        neighbours = 0;
        for(xx = -1; xx <= 1; xx++)
        {
          for(yy = -1; yy <= 1; yy++)
          {
            xxx = wrap(x+xx, worldWidth);
            yyy = wrap(y+yy, worldHeight);
            
            if(xxx != xx || yyy != yy)
            {
              if(xx != 0 && yy != 0)
              {
                if(world[yyy][xxx][ix] > 0)neighbours++;    
              }
            }
          } 
        }
        
        //Any live cell with two or three neighbors survives.
        //Any dead cell with three live neighbors becomes a live cell.
        //All other live cells die in the next generation. Similarly, all other dead cells stay dead.        

        //Apply rules
        if(me)
        {
          if(neighbours == 2 || neighbours == 3)
          {
            world[y][x][next] = 1;   
          }
          else
          {
            world[y][x][next] = 0;   
            r = true;
          }
        }
        else
        {
          if(neighbours == 3)
          {
            world[y][x][next] = 1;
            r = true;
          }
          else
          {
            world[y][x][next] = 0;   
          }
        }
     }
  }
  //Flip state
  ix = next;
  return r;
}

void initWorld()
{
  
  iterations = 0;
  stalls = 0;
  int v = 0;
  for (int y = 0; y < worldHeight; y++)
   {
     for (int x = 0; x < worldWidth; x++)
     {
        v = (random(1024) < 21)?1:0;
        
        world[y][x][ix] = 255*v;
        world[y][x][ix] = ((v+1)%2)*255;
     }
   } 
   //while(!updateWorld())
   //{}
}

void drawWorld()
{
  unsigned char v;
  for (int y = 0; y < worldHeight; y++)
   {
     for (int x = 0; x < worldWidth; x++)
     {
        if(world[y][x][ix] != world[y][x][(ix+1)%2])
        {
          v = (world[y][x][ix] > 0)?255:0;
          display.drawPixelRGB888(x , y, v, v, v);  
        }
     }
   }
}


/**************************************
 * 
 * Still graphics
 * 
 *************************************/


void drawImg(int offset)
{
   int x =0;
   int y =0;
   int counter = 0;
   char* data = header_data;
   char pixel[4];
   for(int i = 0; i < offset * 32; i++)
   {
       HEADER_PIXEL(data, pixel)
       
   }
   
   for (int yy = 0; yy < 32; yy++)
   {
     for (int xx = 0; xx < 32; xx++)
     {
       HEADER_PIXEL(data, pixel)
       display.drawPixelRGB888(xx + x , yy + y, pixel[0]*DON, pixel[1]*DON, pixel[2]*DON);
     }
   }
}

/**************************************
 * 
 * Draw Clock
 * 
 *************************************/

//last color at that range
int last_clock[20][2];

void drawArm(float angle, float cx, float cy, float l, uint16_t c)
{
    angle -= 1.0/4; //Offset degree so it starts in north
    angle *= M_PI*2;
    
    /*display.drawLine(
      cx + cos(angle) * from , 
      cy + sin(angle) * from, 
      cx + cos(angle) * to , 
      cy + sin(angle) * to, 
      c);*/

    float dx = cos(angle); 
    float dy = sin(angle); 
    float ll = 0.5;
    while(abs(ll*dx) < l && abs(ll*dy) < l)
    {
      ll += 0.5;
    }

    int x = round(cx+dx*ll);
    int y = round(cy+dy*ll);


    //Remember last pixel we drew on    
    display.drawPixel(last_clock[int(round(l))][0], last_clock[int(round(l))][1], 0);
    last_clock[int(round(l))][0] = x;
    last_clock[int(round(l))][1] = y;
    
    display.drawPixel(x, y, c);
}


/**************************************
 * 
 * Setup
 * 
 *************************************/

Timezone myTZ;


void setup() {

  display.begin(8);
  
  display.setFastUpdate(true);
  display_update_enable(true);

  display.setBrightness(180);

  drawImg(0);

  
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  //initWorld();
  
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  waitForSync();
  
  myTZ.setLocation(F("Europe/Copenhagen"));
  Serial.print(F("Copenhagen:         "));
  Serial.println(myTZ.dateTime());

  
  
  //OTA

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("KidsTime");

  // No authentication by default
  ArduinoOTA.setPassword((const char *)"update");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //timeClient.begin();
}


/**************************************
 * 
 * Main loop
 * 
 *************************************/



int64_t last_gol_update = 0;
int64_t last_ntp_update = 0;

unsigned long lastepoch = 0;
unsigned long lastepoch_millis;


int last_i = -999;

void loop() {
  int64_t n = millis();
  
  //reboot once daily
  if(n > 1000*24*60*60)
  {
    Serial.print("resetting\r\n");
    ESP.restart();
  }


  unsigned long e = myTZ.now();
  //timeClient.getEpochTime();
  
  if(e/60 != lastepoch)
  {
    lastepoch = e/60;
    lastepoch_millis = n;
  }

  
  if(n - last_gol_update > 1000)
  {
    /*
    last_gol_update = n;
    iterations ++;
    if(!updateWorld())
    {
      stalls++;
    }
    drawWorld();
    if(stalls > 0 || iterations > 60)
    {
      initWorld();  
    }
    */

    int dayseconds = 86400;
    int halfdayseconds = dayseconds / 2;
    
    float sleeptime = (e % dayseconds + (12-1)*3600)/ (1.0*dayseconds);
    int image_height = round(
        constrain(
          cos(2*M_PI*sleeptime)*64+16
        , 0, 32)
        );


    
    if(image_height != last_i)
    {
      drawImg(image_height);
      last_i = image_height;
    }
    
    drawArm((e%halfdayseconds)/(1.0*halfdayseconds), 15.5, 15.5, 13, myRED);
    drawArm((e%3600)/(1.0*3600), 15.5, 15.5, 14, myGREEN);
    drawArm(((n-lastepoch_millis) % 60000)/(1.0*60000), 15.5, 15.5, 15, myBLUE);

    

    
    
    
    
    

    
  }


  ArduinoOTA.handle();
  //timeClient.update();

  /*
  static int lasttimestr = 0;
  if(n - lasttimestr > 1000)
  {
    Serial.print(daysOfTheWeek[timeClient.getDay()]);
    Serial.print(", ");
    Serial.print(timeClient.getHours());
    Serial.print(":");
    Serial.print(timeClient.getMinutes());
    Serial.print(":");
    Serial.println(timeClient.getSeconds());
    lasttimestr = n;
  }
  */

 
}
