#include <Adafruit_ASFcore.h>
//#include <clock.h>
//#include <clock_feature.h>
//#include <compiler.h>
//#include <gclk.h>
//#include <i2s.h>
//#include <interrupt.h>
//#include <interrupt_sam_nvic.h>
//#include <parts.h>
//#include <pinmux.h>
//#include <power.h>
//#include <reset.h>
//#include <status_codes.h>
//#include <system.h>
//#include <system_interrupt.h>
//#include <system_interrupt_features.h>
//#include <Wire.h> 
#include <Adafruit_L3GD20.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
//#include "Adafruit_TMP007.h"
#include <avdweb_SAMDtimer.h>
#include <Adafruit_SleepyDog.h>



#define VBATPIN A7
#define LED 13
#define TIMER_PERIOD 100000
#define MIN_PERIOD 10
#define __SAMD21G18A__
#define DEBUG false
#define BUFF_SIZE 100
#define CSV F(",")
#define BLINK_PER 10



typedef struct sensorReadings_s
    {
      int16_t ax; 
      int16_t ay;
      int16_t az;
      int16_t gx;
      int16_t gy;
      int16_t gz;
      unsigned long ts;
    } sensorReadings;


Adafruit_LSM303 lsm;
Adafruit_L3GD20 lsg;
//Adafruit_TMP007 tmp007;






//sensors_event_t event;
sensorReadings buf[BUFF_SIZE];



volatile int readPtr, sendPtr, mincounter, blinkcounter;
boolean doMeasure, measured, doBlink;

unsigned long t, measuretime; 

String msg, filename, serbuff, str, msgtosend;;

void  timerInterupt(struct tc_module *const module_inst){

 Watchdog.reset();
 blinkcounter++;
 mincounter++;

 if ((blinkcounter > BLINK_PER) && doBlink) {
  blinkcounter = 0;
  digitalWrite(LED, !digitalRead(LED));
 }
 
  if (doMeasure){
    lsm.read();
    lsg.read();
    if (readPtr < BUFF_SIZE){
    buf[readPtr].ax=lsm.accelData.x;
    buf[readPtr].ay=lsm.accelData.y;
    buf[readPtr].az=lsm.accelData.z;
    buf[readPtr].gx=int(lsg.data.x*10000);
    buf[readPtr].gy=int(lsg.data.y*10000);
    buf[readPtr].gz=int(lsg.data.z*10000);
    buf[readPtr].ts=millis()-measuretime;
    readPtr++;
    measured = true;
    
    
   } else {
    readPtr = 0;
    
   }
 }
 
}



SAMDtimer mytimer2(4, timerInterupt, TIMER_PERIOD, false );





void setup() {

  // Watchdog is optional!
  
  doMeasure = false;
  doBlink=true;
  blinkcounter = 0;
  pinMode(LED, OUTPUT);
  
  Watchdog.enable(8000);
  mytimer2.enableInterrupt(true);

  Serial.begin(115200);
  delay(3000);

  Serial1.begin(115200);
  delay(3000);


  
  
  

  //initialyzing buffer for sensor readings

  Serial.print(F("initialise variables...."));
  msg.reserve(64);
  msg="";
  filename.reserve(64);
  filename = "";
  
  serbuff="";
  
  str.reserve(64);
  
  msgtosend.reserve(2048);
  msgtosend = "";
  
  readPtr = 0;
  sendPtr = 0;
 

  measured = false;
  mincounter=0;
  
  Serial.println("Ok"); 
  Serial.print(F("initialise sensors...."));
  lsm.begin();
  lsg.begin();
  Serial.println(F("Ok")); 
  Serial.print(F("initialise WiFi...."));
  initESP();
  Serial.println(F("Ok")); 
  
 
  doBlink = false;
  digitalWrite(LED,LOW);

  


}



 


void loop() {

  int fromBuff;
 
  int datalen;
  int connectionId;

 if(mincounter > 600){
  mincounter = 0;
  //  Serial.println("Batt voltage: "+String(getBatLvl(),DEC));
 }

  if(measured){
    measured = false;
    str = String(buf[readPtr-1].ax,DEC)+CSV+String(buf[readPtr-1].ay,DEC)+CSV+String(buf[readPtr-1].az,DEC)+CSV+String(buf[readPtr-1].gx,DEC)+CSV+String(buf[readPtr-1].gy,DEC)+CSV+String(buf[readPtr-1].gz,DEC)+CSV+String(buf[readPtr-1].ts,DEC)+"\n";
    if (msgtosend.length()+str.length() < 2048) {
     msgtosend += str; 
    }else{
     sendESP(msgtosend);
     msgtosend = str; 
    }
    
  }
  

  if(Serial1.available()) // check if the esp is sending a message 
  {
     while (Serial1.available()){ serbuff.concat(char(Serial1.read()));}
  }   
     
        
     if (serbuff.indexOf("CONNECT") > 0 ) {
      sendESP("I am alive! enter command>");
      serbuff="";
     }

   if(serbuff.indexOf("\n") >0 ){
      Serial.print("Msg: ");Serial.println(serbuff);
      
      int z = serbuff.indexOf(":");
     
      if (z>0 && serbuff.indexOf("IPD") > 0)   {
       msg = serbuff.substring(z+1);      
       Serial.print("data :");Serial.println(msg);

       if (msg.startsWith("b ")){
        filename = msg.substring(msg.indexOf(" ")+1);
        delay(200);
        measured = false;
        sendESP("\n\r\Starting measure, name: "+filename); 
        delay(100);
        sendESP("\n\rax;ay;az;gx;gy;gz;ts\n\r");
        delay(100);
        readPtr = 0;
        memset(buf,0,sizeof(buf));    
        measuretime = millis(); 
        msgtosend = "";
        digitalWrite(LED,HIGH);
        doMeasure = true;
      }

      if (msg.startsWith("s")){
       doMeasure = false;
       sendESP(msgtosend);
       delay(200);
       t = millis() - measuretime;
       Serial.println("Stoping measure....");
       sendESP("\n\r\Stopped\r\nEnter command>");
       digitalWrite(LED,LOW);
       
      }


      if (msg.startsWith("v")){
        str = "Batt voltage: "+String(getBatLvl(),DEC)+"\n";
        sendESP(str);
        delay(100);
        sendESP("Enter command>");
       
      }

       if (msg.startsWith("q")){
        Serial1.println("AT+CIPCLOSE=0");
       
      }

    
    
    }
   serbuff = "";   
       
  }
    
 }




float getBatLvl() {
  float measuredvbat;
  measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  return measuredvbat;
}





void initESP(){

   sendData("AT+RST\r\n",500,DEBUG);
   sendData("AT+CWSAP=\"loger\",\"\",5,0\r\n",500,DEBUG);
   sendData("AT+CIPMUX=1\r\n",500,DEBUG);
   sendData("AT+CIPSERVER=1,23\r\n",2000,DEBUG);
}


void sendESP(String msg){
  
  String cmd,newmsg;
  //newmsg = "\r\n";
  //newmsg += msg;
  cmd="AT+CIPSEND=";
  cmd += "0";
  cmd += ",";
  cmd += msg.length();
  cmd += "\r\n";
  sendData(cmd,25,DEBUG);
  sendData(msg,25,DEBUG);
   
}





String sendData(String command, const int timeout, boolean debug)
{
    
    
    String response = "";
        
    Serial1.print(command); // send the read character to the esp8266
    
   long int time = millis();
    
    while( (time+timeout) > millis())
    {
      //while(Serial1.available())
      //{
        
        // The esp has data so display its output to the serial window 
     //   char c = Serial1.read(); // read the next character.
     //   response += c;
     //  }  
    }
    
    if(debug)
    {
     // Serial.print(response);
    }
    
    return response;
 
  


}



