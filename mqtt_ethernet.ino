#include <SPI.h>
#include <Dns.h>
#include <Dhcp.h>
#include <Arduino.h>
#include <Ethernet.h>
#include <Thermistor.h>
#include <LiquidCrystal.h>
#include <EthernetClient.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/************************* Adafruit.io Setup *********************************/
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "carlosrodriguesit"
#define AIO_KEY         "4db0020c64894a6fa2f81c87fbe89d08"					   
/*****************************************************************************/

/************************Variables analog or digital *************************/
//Analog A0 value
Thermistor temp(0);
LiquidCrystal lcd(9, 8, 7, 6, 5, 4);
/*****************************************************************************/

/************************* Ethernet Client Setup *****************************/
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
//Uncomment the following, and set to a valid ip if you don't have dhcp available.
IPAddress iotIP (192, 168, 0, 200);
//Uncomment the following, the subnet.
IPAddress subIP(255, 255, 255, 0);
//Uncomment the following, the router's gateway.
IPAddress gatIP(192, 168, 0, 1);
//Uncomment the following, and set to your preference if you don't have automatic dns.
IPAddress dnsIP (8, 8, 8, 8);
//If you uncommented either of the above lines, make sure to change "Ethernet.begin(mac)" to "Ethernet.begin(mac, iotIP)" or "Ethernet.begin(mac, iotIP, dnsIP)"
/*****************************************************************************/

/************************* Ethernet Server HTTP 80 *****************************/
EthernetServer server(80);
/*****************************************************************************/

/************ Global State (you don't need to change this!) ******************/
//Set up the ethernet client with DHCP if ready!
EthernetClient client;

// Store the MQTT server, client ID, username, and password in flash memory.
// This is required for using the Adafruit MQTT library.
const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;
// Set a unique MQTT client ID using the AIO key + the date and time the sketch
// was compiled (so this should be unique across multiple devices for a user,
// alternatively you can manually set this to a GUID or other random value).
const char MQTT_CLIENTID[] PROGMEM  = __TIME__ AIO_USERNAME;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;

Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD);

// You don't need to change anything below this line!
#define halt(s) { Serial.println(F( s )); while(1);  }
/*****************************************************************************/

/****************************** Feeds ***************************************/

// Setup a feed called 'temperature' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
const char TEMPERATURE_FEED[] PROGMEM = AIO_USERNAME "/feeds/temperature";
Adafruit_MQTT_Publish temperature = Adafruit_MQTT_Publish(&mqtt, TEMPERATURE_FEED, MQTT_QOS_1);
/*****************************************************************************/

/*************************** Sketch Code ************************************/
void setup()
{
    Serial.begin(115200);
    // Setup LCD 16*2 and clear display
    lcd.begin(16, 2);
    //function set IP address and display in serial and LCD
    tcpIP();    
    delay(1000); //give the ethernet a second to initialize
}    

// read the input on analog pin 0:
uint32_t x=0;

void loop()
{
    //Ensure the connection to the MQTT server is alive (this will make the first
    //connection and automatically reconnect when disconnected).  See the MQTT_connect
    //function definition further below.
    MQTT_connect();


    //Now we can publish stuff!
    Serial.print(-temp.getTemp());
  
    if (! temperature.publish(-temp.getTemp()))
    {
        Serial.println(F("Failed"));
    } 
    else
    {
        Serial.println(F("OK!"));
    }

    //function to call Google Chart
    googleChart();
    
    //Tempo de espera até nova leitura de temperatura
    delay(5000);


    
  /*ESTA FUNCAO DESLIGA O SERVIDOR - NAO UTILIZAR
    // ping the server to keep the mqtt connection alive
    if(! mqtt.ping())
    {
        mqtt.disconnect();
    }
  */    
}
/*****************************************************************************/

/******************************Functions**************************************/
// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect()
{
	int8_t ret;
	
	// Stop if already connected.
	if (mqtt.connected())
	{
		return;
	}
	
	Serial.print("Connecting to MQTT... ");
	
	// connect will return 0 for connected
	while ((ret = mqtt.connect()) != 0)
	{
		Serial.println(mqtt.connectErrorString(ret));
		Serial.println("Retrying MQTT connection in 5 seconds...");
		mqtt.disconnect();
		delay(5000);  // wait 5 seconds
	}
	
	Serial.println("MQTT Connected!");
}

//function to start TCP/IP communication 1-DHCP and 2-Static
void tcpIP()
{
	// start the Ethernet connection:
	Serial.println("\nGet IP by DHCP");
	// Initiate the LCD
	lcd.setCursor(0, 0);
	lcd.print("Get IP by DHCP");

	
	if(Ethernet.begin(mac) == 0)
	{
		Serial.println("Failed to configure Ethernet using DHCP");
		lcd.clear();
		delay(1000);
		lcd.print("Failed Get DHCP");
		lcd.clear();
		
		// initialize the Ethernet device with static IP:
		Ethernet.begin(mac, iotIP, dnsIP, gatIP, subIP);
		delay(1000);
		
		Serial.print("Set Static IP - ");
		lcd.print("Set Static IP");
		delay(2000);
		lcd.clear();
	}
	
	iotIP = Ethernet.localIP();
	
	// print your local IP address:
	lcd.clear();
	lcd.setCursor(0,0);
	lcd.print("IP-");

	for(byte thisByte = 0; thisByte < 4; thisByte++)
	{
		// print the value of each byte of the IP address:
		Serial.print(iotIP[thisByte], DEC);
		Serial.print(".");
		lcd.print(iotIP[thisByte]);
		lcd.print(".");
	}
	Serial.print("\n");

	lcd.setCursor(0,1);
	lcd.print("Temperatura-");
	lcd.print(-temp.getTemp());
}
/*****************************************************************************/

//function to Start WebServer and display results of analog value with Google Charts 
void googleChart()
{
   // listen for incoming clients
   EthernetClient client = server.available();
   
   if(client)
   {
       Serial.println("new client");
       // an http request ends with a blank line
       boolean currentLineIsBlank = true;
       
       while (client.connected())
       {
           if (client.available())
           {
               char c = client.read();
               Serial.write(c);
             
               // if you've gotten to the end of the line (received a newline
              // character) and the line is blank, the http request has ended,
             // so you can send a reply
             
             if (c == '\n' && currentLineIsBlank)
             {
                 // send a standard http response header
                 client.println("HTTP/1.1 200 OK");
                 client.println("Content-Type: text/html");
                 client.println("Connection: close");  // the connection will be closed after completion of the response
                 client.println("Refresh: 5");  // refresh the page automatically every 5 sec
                 client.println();
                 client.println("<!DOCTYPE HTML>");
                 client.println("<html>");
                 client.println("<head>");
                 client.println("<script type='text/javascript' src='https://www.gstatic.com/charts/loader.js'></script>");
                 client.println("<script type='text/javascript'>");
                 client.println("google.charts.load('current', {'packages':['corechart']});");
                 client.println("google.charts.setOnLoadCallback(drawChart);");
                 client.println("function drawChart() {");
                 client.println("var data = google.visualization.arrayToDataTable([");
                 client.println("['Effort', 'Amount given'],");
                 client.println("['Temperature',");
                 client.print(-temp.getTemp());
                 client.print("],");
                 client.println("]);");
                 client.println("var options = {");
                 client.println("pieHole: 0.5,");
                 client.println("pieSliceTextStyle: {");
                 client.println("color: 'black',");
                 client.println("},");
                 client.println("legend: 'Internet of Things'");
                 client.println("};");
                 client.println("var chart = new google.visualization.PieChart(document.getElementById('donut_single'));");
                 client.println("chart.draw(data, options);");
                 client.println("}");
                 client.println("</script>");
                 client.println("</head>");
                 client.println("<body>");
                 client.println("<div id='donut_single' style='width: 900px; height: 500px; display: block; margin: 0 auto;'></div>");
                 client.println("</body>");         
                 client.println("</html>");
                break;
            }
        
            if (c == '\n')
            {
                // you're starting a new line
                currentLineIsBlank = true;
            }
            else if (c != '\r')
            {
                // you've gotten a character on the current line
                currentLineIsBlank = false;
            }
         }
      }      
      // give the web browser time to receive the data
      delay(1);
      // close the connection:
      client.stop();
      Serial.println("client disconnected");
   }
}
