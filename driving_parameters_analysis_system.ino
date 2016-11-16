// Instructables link :
// http://www.instructables.com/id/Driving-Parameters-Analysis-System/

// Driving Parameters Analysis System

//Header files required
//If not available, make sure to find them in Github repositories
#include <HttpClient.h>
#include <LTask.h>
#include <LWiFi.h>
#include <LWiFiClient.h>
#include <LDateTime.h>
#include <LGPRS.h>
#include <LGPRSClient.h>
#include <LGPRSServer.h>
#include <LGPS.h>

//Customize the data needed as per your specifications
#define WIFI_AP "your wifi name"
#define WIFI_PASSWORD "your wifi password"
#define DEVICEID "your device id"
#define DEVICEKEY "your device key"
#define SITE_URL "api.mediatek.com"

#define WIFI_AUTH LWIFI_WPA  
#define per 50
#define per1 3

gpsSentenceInfoStruct info;
char buff[256];
double latitude;
double longitude;
double spd;

char buffer_latitude[8];
char buffer_longitude[8];
char buffer_spd[8];
char buffer_sensor[8];

static unsigned char getComma(unsigned char num,const char *str)
{
  unsigned char i,j = 0;
  int len=strlen(str);
  for(i = 0;i < len;i ++)
  {
     if(str[i] == ',')
      j++;
     if(j == num)
      return i + 1; 
  }
  return 0; 
}

static double getDoubleNumber(const char *s)
{
  char buf[10];
  unsigned char i;
  double rev;
  
  i=getComma(1, s);
  i = i - 1;
  strncpy(buf, s, i);
  buf[i] = 0;
  rev=atof(buf);
  return rev; 
}

static double getIntNumber(const char *s)
{
  char buf[10];
  unsigned char i;
  double rev;
  
  i=getComma(1, s);
  i = i - 1;
  strncpy(buf, s, i);
  buf[i] = 0;
  rev=atoi(buf);
  return rev; 
}

//Esssential part of code
//GPS data processing
//Determining quantities latitude,longitude and speed
void parseDPAS(const char* DPASstr)
{
    int tmp, hour, minute, second, num ;
  if(DPASstr[0] == '$')
  {
    tmp = getComma(1, DPASstr);
    hour     = (DPASstr[tmp + 0] - '0') * 10 + (DPASstr[tmp + 1] - '0');
    minute   = (DPASstr[tmp + 2] - '0') * 10 + (DPASstr[tmp + 3] - '0');
    second    = (DPASstr[tmp + 4] - '0') * 10 + (DPASstr[tmp + 5] - '0');
    
    sprintf(buff, "UTC timer %2d-%2d-%2d", hour, minute, second);
        
    tmp = getComma(2, DPASstr);
    latitude = getDoubleNumber(&DPASstr[tmp])/100.0;
    int latitude_int=floor(latitude);
    double latitude_decimal=(latitude-latitude_int)*100.0/60.0;
    latitude=latitude_int+latitude_decimal;
    tmp = getComma(4, DPASstr);
    longitude = getDoubleNumber(&DPASstr[tmp])/100.0;
    int longitude_int=floor(longitude);
    double longitude_decimal=(longitude-longitude_int)*100.0/60.0;
    longitude=longitude_int+longitude_decimal;
    
    sprintf(buff, "latitude = %10.4f, longitude = %10.4f", latitude, longitude);
    tmp = getComma(7, DPASstr);
    spd = getDoubleNumber(&DPASstr[tmp]);    
    sprintf(buff, "speed = %F", spd);
    
  }
  else
  {
    Serial.println("Not get data"); 
  }
}


void recdat() 
{
  LGPS.getData(&info);
  //Serial.println((char*)info.DPAS); 
  parseDPAS((const char*)info.DPAS);
}

//AP connection
void AP_connect()
{
  Serial.print("Connecting to AP...");
  while (0 == LWiFi.connect(WIFI_AP, LWiFiLoginInfo(WIFI_AUTH, WIFI_PASSWORD)))
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Success!");
  
  Serial.print("Connecting site...");

  while (!drivepar.connect(SITE_URL, 80))
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Success!");
  delay(100);
}


void getconnectInfo()
{
    drivepar.print("GET /mcs/v2/devices/");
  drivepar.print(DEVICEID);
  drivepar.println("/connections.csv HTTP/1.1");
  drivepar.print("Host: ");
  drivepar.println(SITE_URL);
  drivepar.print("deviceKey: ");
  drivepar.println(DEVICEKEY);
  drivepar.println("Connection: close");
  drivepar.println();
  
  delay(500);

  int errorcount = 0;
  Serial.print("waiting for HTTP response...");
  while (!drivepar.available())
  {
    Serial.print(".");
    errorcount += 1;
    delay(150);
  }
  Serial.println();
  int err = http.skipResponseHeaders();

  int bodyLen = http.contentLength();
  char c;
  int ipcount = 0;
  int count = 0;
  int separater = 0;
  while (drivepar)
  {
    int v = (int)drivepar.read();
    if (v != -1)
    {
      c = v;
      //Serial.print(c);
      connection_info[ipcount]=c;
      if(c==',')
      separater=ipcount;
      ipcount++;    
    }
    else
    {
      Serial.println("no more content, disconnect");
      drivepar.stop();

    }
    
  }


  int i;
  for(i=0;i<separater;i++)
  {  ip[i]=connection_info[i];
  }
  int j=0;
  separater++;
  
  for(i=separater;i<21 && j<5 && i < ipcount;i++)
  {  port[j]=connection_info[i];
     j++;
  }
  
  
  portnum = atoi (port);

} 

void connectTCP()
{
  
  c.stop();
  Serial.print("Connecting to TCP...");
  while (0 == c.connect(ip, portnum))
  {
    Serial.println("Re-Connecting to TCP");    
    delay(1000);
  }
  c.println(tcpdata);
  c.println();
  Serial.println("Success!");
} 

void heartBeat()
{
  Serial.println("send TCP heartBeat");
  c.println(tcpdata);
  c.println();
    
}



void uploadstatus()
{
  while (!drivepar.connect(SITE_URL, 80))
  {
    Serial.print(".");
    delay(500);
  }
  
  delay(100);
  
  if(digitalRead(13)==1)
    upload_led = "ENGINE_MODE_DISPLAY,,1";
  else
    upload_led = "ENGINE_MODE_DISPLAY,,0";
  
  int thislength = upload_led.length();
  HttpClient http(drivepar);
  drivepar.print("POST /mcs/v2/devices/");
  drivepar.print(DEVICEID);
  drivepar.println("/datapoints.csv HTTP/1.1");
  drivepar.print("Host: ");
  drivepar.println(SITE_URL);
  drivepar.print("deviceKey: ");
  drivepar.println(DEVICEKEY);
  drivepar.print("Content-Length: ");
  drivepar.println(thislength);
  drivepar.println("Content-Type: text/csv");
  drivepar.println("Connection: close");
  drivepar.println();
  drivepar.println(upload_led);
  delay(500);

  int errorcount = 0;
  while (!drivepar.available())
  {
    Serial.print(".");
    delay(100);
  }
  int err = http.skipResponseHeaders();
  int bodyLen = http.contentLength();
  
  while (drivepar)
  {
    int v = drivepar.read();
    if (v != -1)
    {
      Serial.print(char(v));
    }
    else
    {
      Serial.println("no more content, disconnect");
      drivepar.stop();
    }
    
  }
  Serial.println();
}


//Data streaming to Mediatek Cloud Sandbox
void transdat(){

  while (!drivepar.connect(SITE_URL, 80))
  {
    Serial.print(".");
    delay(500);
  }
  
  delay(100);


float sensor = analogRead(A0);
  Serial.printf("latitude=%.4f\tlongitude=%.4f\tspd=%.4f\tsensor=%.4f\n",latitude,longitude,spd,sensor);
  
  if(latitude>-90 && latitude<=90 && longitude>=0 && longitude<360)
  { 
    sprintf(buffer_latitude, "%.4f", latitude);
    sprintf(buffer_longitude, "%.4f", longitude);
    sprintf(buffer_spd, "%.4f", spd);
    sprintf(buffer_sensor, "%.4f", sensor);
    
  }
  String upload_GPS = "GPS,,"+String(buffer_latitude)+","+String(buffer_longitude)+","+String(buffer_spd)+","+String(buffer_sensor)+","+"0"+"\n"+"LATITUDE,,"+buffer_latitude+"\n"+"LONGITUDE,,"+buffer_longitude+"\n"+"SPEED,,"+buffer_spd+"\n"+"STEERING_ANGLE,,"+buffer_sensor;
  int GPS_length = upload_GPS.length();
  HttpClient http(drivepar);
  
  drivepar.print("POST /mcs/v2/devices/");
  drivepar.print(DEVICEID);
  drivepar.println("/datapoints.csv HTTP/1.1");
  drivepar.print("Host: ");
  drivepar.println(SITE_URL);
  drivepar.print("deviceKey: ");
  drivepar.println(DEVICEKEY);
  drivepar.print("Content-Length: ");
  drivepar.println(GPS_length);
  drivepar.println("Content-Type: text/csv");
  drivepar.println("Connection: close");
  drivepar.println();
  drivepar.println(upload_GPS);
  delay(500);

  int errorcount = 0;
  
  while (!drivepar.available())
  {
    Serial.print(".");
    delay(100);
  }
  int err = http.skipResponseHeaders();
  int bodyLen = http.contentLength();
  
  while (drivepar)
  {
    int v = drivepar.read();
    if (v != -1)
    {
      Serial.print(char(v));
    }
    else
    {
      Serial.println("no more content, disconnect");
      drivepar.stop();
    }
    
  }
  Serial.println();
}
LGPRSClient c;
unsigned int rtc;
unsigned int lrtc;
unsigned int rtc1;
unsigned int lrtc1;
char port[4]={0};
char connection_info[21]={0};
char ip[15]={0};             
int portnum;
int val = 0;
String tcpdata = String(DEVICEID) + "," + String(DEVICEKEY) + ",0";
String tcpcmd_led_on = "ENGINE_MODE_CONTROL,1";
String tcpcmd_led_off = "ENGINE_MODE_CONTROL,0";
String upload_led;

LGPRSClient drivepar;
HttpClient http(drivepar);


//Setup
void setup()
{
  pinMode(13, OUTPUT);
  
  LTask.begin();
  LWiFi.begin();
  
  Serial.begin(115200);
  LGPS.powerOn();
  AP_connect();
  getconnectInfo();
  connectTCP();
  
  Serial.println("...");
}


//Loop
void loop()
{
  String tcpcmd="";
  
  while (c.available())
   {
      int v = c.read();
      if (v != -1)
      {
        Serial.print((char)v);
        tcpcmd += (char)v;
        if (tcpcmd.substring(40).equals(tcpcmd_led_on)){
          digitalWrite(13, HIGH);
          Serial.print("Switch LED ON ");
          tcpcmd="";
        }
        else if(tcpcmd.substring(40).equals(tcpcmd_led_off)){  
          digitalWrite(13, LOW);
          Serial.print("Switch LED OFF");
          tcpcmd="";
        }
      }
   }

  LDateTime.getRtc(&rtc);
  if ((rtc - lrtc) >= per) {
    heartBeat();
    lrtc = rtc;
  }
  
  LDateTime.getRtc(&rtc1);
  if ((rtc1 - lrtc1) >= per1) {
    
    uploadstatus();
    recdat();
    transdat();
    lrtc1 = rtc1;
  }
 
}
