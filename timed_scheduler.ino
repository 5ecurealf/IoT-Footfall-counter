// |X| poll the state of the button every 5ms and count the number of low-to-high transitions 
// |X| Update the LCD every 1 second with the IP address assigned to the board and average footfall per minute. |X| calculated ffpm as total presses/amount of minutes
// |X| TRANSFER VALUES footfall, Average footfall per minute, sensors unique identifier
//       |X|Over the local network using HTTP -------------use mobile network to connect board and oli's laptop (200)
//       |X|Over the internet using MQTT.
// |X| Reset the total footfall counter using HTTP 
// |X| Reset the total footfall counter using MQTT
// |X| LED on if there is no WiFi connection  (line 91-93) WATCHDOG TIMER RESETS IF NOT CONNECTED SO LED WILL GO HIGH IF NOT CONNECTED EVEN IF IT LOOSES CONNECTION INSIDE LOOP
// |X| add the function calls (function prototype l4t2) to the top of the page like you've done for MQTTconnect 
//optimising design

// | | could you use floats to save memory space and power as need to call instruction and then call register locatiion

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

WiFiServer SERVER(80);

WiFiClient client; //4. Create a global instance of the WiFiClient class called client.


const char ssid[] = "aaa";
const char password[] = "Miamiamia";


#define TIMEOUTVAL 2 // how many ms we will wait for request data
#define LINES 8 // how many lines of the request we will store
#define LEN 150 // how many characters per line we will store

char inputbuf[LINES][LEN] = {}; //input buffer



// -------------------------------------------------------------------

#include <RFID.h>
#include <SPI.h>
#include <rgb_lcd.h>

#define SS_PIN 0
#define RST_PIN 2
#define LEDPIN 15
#define ANALOGUE_PIN A0

#define TASK1L 5 //every 5ms 
#define TASK2L 60000 //every 60s 
#define TASK3L 2000 //every 2s 
#define TASK4L 1000 //every 1s 
#define TASK5L 5000 //every 5s 

unsigned long current_millis = 0;
unsigned long task1LC = 0;
unsigned long task2LC = 0;
unsigned long task3LC = 0;
unsigned long task4LC = 0;
unsigned long task5LC = 0;



int timespressed = 0;
int minute_counter = 0;
float total_print = 0;

int cardpresent = 0, lastcardpresent = 0;
String tag= "";
RFID myrfid(SS_PIN,RST_PIN);

int red = 200;
int blue = 255;
int green = 0;

//Create an instance of the RGB_lcd class
rgb_lcd LCD;
/*
lcd.setCursor(0, 0); // top left
lcd.setCursor(15, 0); // top right
lcd.setCursor(0, 1); // bottom left
lcd.setCursor(15, 1); // bottom right
*/

/***********************************************************************
 *  
 ***************************************************************************************************/

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"


//#define NOPUBLISH      // comment this out once publishing at less than 10 second intervals

#define ADASERVER     "io.adafruit.com"     // do not change this
#define ADAPORT       1883                  // do not change this 
#define ADAUSERNAME   "LLOYDA1"               // ADD YOUR username here between the qoutation marks
#define ADAKEY        "aio_Nolk78pVbiSin0zimxM8n4ZS9eTp" // ADD YOUR Adafruit key here betwwen marks


WiFiClient client1 ;    // create a class instance for the MQTT server


// create an instance of the Adafruit MQTT class. This requires the client, server, portm username and
// the Adafruit key
Adafruit_MQTT_Client MQTT(&client1, ADASERVER, ADAPORT, ADAUSERNAME, ADAKEY);


/******************************** Feeds *************************************************************
 *  
 ***************************************************************************************************/
//Feeds we publish to

Adafruit_MQTT_Publish FFC = Adafruit_MQTT_Publish(&MQTT, ADAUSERNAME "/feeds/FootfallCount");
Adafruit_MQTT_Publish AFFC = Adafruit_MQTT_Publish(&MQTT, ADAUSERNAME "/feeds/AvgFootfallCount");
Adafruit_MQTT_Publish ID = Adafruit_MQTT_Publish(&MQTT, ADAUSERNAME "/feeds/sensorID");


// Setup a feed called LED to subscibe to HIGH/LOW changes
Adafruit_MQTT_Subscribe reset_mqtt = Adafruit_MQTT_Subscribe(&MQTT, ADAUSERNAME "/feeds/reset_footfall");

int startflag = 1;





void task1 (void);
void task2 (void);
void task3 (void);
void task4 (void);
void task5 (void);
void MQTTconnect ( void );
void servepage ( void );
int checkbutton ( void );



void setup() {
  pinMode(LEDPIN,OUTPUT);
  pinMode(ANALOGUE_PIN,INPUT);
  
    //-------------------------- LCD Initialisation --------------------------
  LCD.begin(16,2);
  LCD.setRGB(red,green,blue);
  //-------------------------------------------------------------------------

  Serial.begin(115200);
  Serial.print("Attempting to connect to");
  Serial.print(ssid);

    //-------------------------- wifi Initialisation --------------------------

  WiFi.begin(ssid,password);

  while(WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    digitalWrite(LEDPIN,HIGH);
  }
    digitalWrite(LEDPIN,LOW);


  Serial.print("\n");
  Serial.println("Successfully connected");

  Serial.print("WiFimacAddress :");
  Serial.println(WiFi.macAddress());
  // print IP address.
  Serial.print("WiFi local IP :");
  Serial.println(WiFi.localIP());
  Serial.print("Subnet mask :");
  Serial.println(WiFi.subnetMask());
  Serial.print("gateway IP:");
  Serial.println(WiFi.gatewayIP());
  Serial.print("dnsIP:");
  Serial.println(WiFi.dnsIP()); 

  //--------------------------start a server--------------------------
  SERVER.begin();
  Serial.println("Server has been started");

  //--------------------------start SUBSCRIPTION TO A FEED --------------------------

  MQTT.subscribe(&reset_mqtt);                         // subscribe to the LED feed
  startflag = 1;
}


void loop() {
  current_millis = millis();

  
  if ( (current_millis - task1LC) >= TASK1L )
    {
      task1();
      task1LC = current_millis;
    }


  if ( (current_millis - task3LC) >= TASK3L )
    {
      task3();
      task3LC = current_millis;
    }


  if ( (current_millis - task5LC) >= TASK5L )
    {
      task5();
      task5LC = current_millis;
    }
    

  if ( (current_millis - task4LC) >= TASK4L )
    {
      task4();
      task4LC = current_millis;
    }


  if ( (current_millis - task2LC) >= TASK2L )
    {
      task2();
      task2LC = current_millis;
    }
}




void task1 (void){ //checking if the button is pressed and incrementing timespressed
    if (checkbutton()==1){
  	timespressed++;
  	}  
}

void task2 (void){ //calculates average footfall per minute
    minute_counter++;
    total_print = timespressed/minute_counter;
}

void task3 (void){ //retrieve client requests and respond to them

  int timecnt = 0; 
  int i;
//-------------------------------------------------------------------------
  client = SERVER.available();
  if (!client)                //wait for client to connect
    return;
//-------------------------------------------------------------------------

  Serial.println("There is a client, waiting for data");
  while(!client.available() ) //while there are no characters 
  { 
      delay(1);     //wait 1ms and try again
      timecnt++;
      if ( timecnt >= TIMEOUTVAL)
      {
          return; //if we have waited for the timeout time or more, return
      }
    
  }
//-------------------------------------------------------------------------

  Serial.println("Reading the request");
  //Now we have characters from the PORT 80 waiting in our buffer lets read them 
  for (i = 0; i < LINES; i++ )
  {
      // read each line as a string and convert into char arrays 
      client.readStringUntil('\r').toCharArray(&inputbuf[i][0],LEN);
      Serial.println(inputbuf[i]);
  }

  client.flush(); //flush remaining chars
  
  if(!strcmp(inputbuf[0],"GET /reset HTTP/1.1")){
  Serial.print("Resetting Footfall counter : ");
  timespressed = 0;
  total_print = 0;
  //task4();
  Serial.println(timespressed);
  }

      

//-------------------------------------------------------------------------
  servepage(); //respond to request with simple webpage
//-------------------------------------------------------------------------
  client.stop(); //stop processing with respect to client 
//-------------------------------------------------------------------------
//  Serial.print("5: ");
//  Serial.println(MQTT.connected());
}

void task4 (void){ //prints information to the LDC every second
//  digitalWrite(LEDPIN, !digitalRead(LEDPIN));   // Toggle led used to check if the task working at the right frequency
  LCD.clear();
  LCD.setCursor(0,0);
  LCD.print(WiFi.localIP());  // print IP address LCD.
  LCD.setCursor(0,1);
  LCD.print(total_print);
  Serial.print("timespressed: ");
  Serial.println(timespressed);
}

void task5 (void){ //publish and reads information to/from the Adafruit MQTT server
//   Serial.print("1: ");
//  Serial.println(MQTT.connected());

  // This function call ensures that we have a connection to the Adafruit MQTT server. This will make
  // the first connection and automatically tries to reconnect if disconnected. However, if there are
  // three consecutive failled attempts, the code will deliberately reset the microcontroller via the 
  // watch dog timer via a forever loop.
  MQTTconnect();

  // an example of subscription code
  Adafruit_MQTT_Subscribe *subscription;                    // create a subscriber object instance
  while ( subscription = MQTT.readSubscription(10) )      // Read a subscription and wait for max of 10 milli seconds.
  {                                                         // will return 1 on a subscription being read.
    if (subscription == &reset_mqtt)                               // if the subscription we have receieved matches the one we are after
    {
      Serial.print("Recieved from the LED subscription:");  // print the subscription out
      Serial.println((char *)reset_mqtt.lastread);                 // we have to cast the array of 8 bit values back to chars to print

      // ADD code here to compare (char *)LED.lastread against "HIGH" or "LOW". Refer to previous labs 
      // for how to use STRCMP. You should use two ifs rather than an if then else structure.
      // we could also convert characters received into integer variables via the use of atoi for passing values from the 
      // the broker to the ESP8266
      if (strcmp( (char *)reset_mqtt.lastread,"1") == 0){
        timespressed =0; 
        total_print = 0;
      }
//      if (strcmp( (char *)reset_mqtt.lastread,"0") == 0){
//      digitalWrite(LEDPIN,LOW);
//      }
     
    }
  }
//  Serial.print("2: ");
//  Serial.println(MQTT.connected());

  // Add publishing code here
  if( FFC.publish(timespressed) == 1){
    Serial.println("publishing FFC was successful");
  }

  if( AFFC.publish(total_print) == 1){
    Serial.println("publishing AFFC was successful");
  }
  
  if(startflag == 1)
  {
    if( ID.publish("Front door") == 1){
      Serial.println("publishing ID was successful");
    }
    startflag = 0;
  }

//Serial.print("3: ");
//Serial.println(MQTT.connected());

}


//function that sends a HTML webpage serving the information  
void servepage ( void )
{
  
  client.println("HTTP/1.1 200 OK");                                  // header for response to GET
  client.println("Content-Type: text/html");                  
  client.println(""); 


  // add a line here to control refresh from the browser
 // client.println("<meta http-equiv=""refresh"" content=""5"">");


  client.println("");
          
  client.println("<!DOCTYPE HTML>");                                  // start of actual HTML file
  client.println("<html>");                                           // start of html
  client.println("<head>");                                           // the HTML header
  client.println("<title>Alfie's sensor</title>");                  // page title as shown in the tab
  client.println("</head>");    
  client.println("<body>");                                           // start of the body
  client.println("<h1>Front door sensor node</h1>");  // heading text
 
 // add content here 

  client.print("Total Footfall :");
  client.println(timespressed); 
  client.println("<br>");
  client.print("Average footfall :");
  client.println( total_print); 


 
  client.println("<br>");
  client.println("</body>");                                          // end of body
  client.println("</html>");                                          // end of HTML
}

// DEBOUNCING THE BUTTON FUNCTION, THEN THE LOOP WILL CHECK IF THIS INPUT IS A 1 OR 0
 int checkbutton ( void )
 {
    static int last = 0;  
    int current = 0;
    pinMode(LEDPIN,INPUT);
    current = digitalRead(LEDPIN);
// if condition checks if there is a rising edge 
    if (current == 1 && !last == 1)
    {
      
        delay(5); //delay checks 5ms later to see if that input is a real input by checking to see if the button is pressed
        
        if (digitalRead(LEDPIN)== HIGH)
        {
          pinMode(LEDPIN,OUTPUT);
          last = current;
          return 1;
        }
        else
        {
          pinMode(LEDPIN,OUTPUT);
          last = current;
          return 0;
        }
    }
    else{
        pinMode(LEDPIN,OUTPUT);
        last = current;
        return 0;
    }
 
 }


 /******************************* MQTT connect *******************************************************
 *  
 ***************************************************************************************************/
void MQTTconnect ( void ) 
{
  unsigned char tries = 0;

  // Stop if already connected.
  if ( MQTT.connected() ) 
  {
    return;
  }
  startflag = 1;
  Serial.print("Connecting to MQTT... ");

  while ( MQTT.connect() != 0 )                                        // while we are 
  {     
       Serial.println("Will try to connect again in five seconds");   // inform user
       MQTT.disconnect();                                             // disconnect
       delay(5000);                                                   // wait 5 seconds
       tries++;
       if (tries == 3) 
       {
          Serial.println("problem with communication, forcing WDT for reset");
          while (1)
          {
            ;   // forever do nothing
          }
       }
  }
  
  Serial.println("MQTT succesfully connected!");
}
