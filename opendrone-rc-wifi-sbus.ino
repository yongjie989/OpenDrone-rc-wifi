/* @file               OpenDrone WiFi to SBUS for OpenDrone RC
 * @technical support  Ethan Huang <ethan@robotlab.tw>
 * @version            V1.0.1
 * @date               2018/05/08
 * 
 *)
*/
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <WiFiUdp.h>
#include <EEPROM.h>

WiFiUDP Udp;
unsigned int localUdpPort = 6666;
char eRead[25];
byte len;
int change_ssid_flag = 0;
int switch_ap_mode_flag = 0;
String factory_ssid = "OD1234";
char* ssid_password = "12345678";
const char* ssid;
String tempR;

void returnFail(String msg);
void handleSubmit(void);
void handleRoot(void);
void SaveString(int startAt, const char* id);
void ReadString(byte startAt, byte bufor);

ESP8266WebServer server_AP(8080);

const char index_html[] =
  "<html>"
  "    <head>"
  "        <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
  "        <title>OpenDrone-RC configuration</title>"
  "    </head>"
  "    <body>"
  "        <form action=\"/\" method=\"post\">"
  "            <h1>輸入飛控板的WiFi SSID</h1>"
  "            SSID <input type=\"text\" name=\"ssid\"/><br/>"
  "            <input type=\"submit\" value=\"Save\" />"
  "        </form>"
  "    </body>"
  "</html>";

const char save_html[] =
  "<html>"
  "    <head>"
  "        <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
  "        <title>OpenDrone-RC configuration</title>"
  "    </head>"
  "    <body>"
  "     <div>已儲存! 請重新啟動電源。</div>"
  "    </body>"
  "</html>";

void returnFail(String msg)
{
  server_AP.sendHeader("Connection", "close");
  server_AP.sendHeader("Access-Control-Allow-Origin", "*");
  server_AP.send(500, "text/plain", msg + "\r\n");
}

void handleSubmit(void)
{
  char *new_ssid = "";
  if (!server_AP.hasArg("ssid")) 
    return returnFail("請輸入SSID.");

  String data1 = server_AP.arg("ssid");
  new_ssid = const_cast<char*>(data1.c_str());

  SaveString(5, new_ssid); 
  Serial.write("UPDATE SSID\n");
  server_AP.send(200, "text/html", save_html);

}

void handleRoot(void)
{
  if (server_AP.hasArg("ssid")) {
    handleSubmit();
  }else{
    server_AP.send(200, "text/html", index_html);
  }
}
  
void SaveString(int startAt, const char* id)
{
  for (byte i = 0; i <= strlen(id); i++)
  {
    EEPROM.write(i + startAt, (uint8_t) id[i]);
  }
  EEPROM.commit();
}

void ReadString(byte startAt, byte bufor)
{
  for (byte i = 0; i <= bufor; i++)
  {
    eRead[i] = (char)EEPROM.read(i + startAt);
  }
  len = bufor;
}

void setup()
{  
  delay(1000);
  int timeout = 5; 
  EEPROM.begin(512);
  Serial.begin(115200);
  Serial.setTimeout(0);
  delay(5000);

  //SaveString(5, "OD1111"); 
  
  ReadString(5, 10);
  
  tempR = "";
  for (byte i = 0; i < len; i++)
  {
    tempR += eRead[i];
  }
  ssid = tempR.c_str();
  if(ssid == "")
  {
    Serial.println(">ssid==null");
    SaveString(5, factory_ssid.c_str());
    ReadString(5, 10);
    tempR = "";
    for (byte i = 0; i < len; i++)
    {
      tempR += eRead[i];
    }
    ssid = tempR.c_str();
  }
  Serial.println("ssid=");
  Serial.println(ssid);  
    
  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, "12345678");

  while (WiFi.status() != WL_CONNECTED and timeout > 0)
  {
    delay(500);
    timeout--;
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED)
  { 
    Udp.begin(6667);
  }
}
void loop()
{ 
  int i = 0;
  int result;
  char *pch;
  char *packets[10];   
  server_AP.handleClient();
  while(Serial.available() > 0 )
  {
    char packetBuffer[45]; 
    String joystick_data = Serial.readString();
    Serial.flush();
    //Serial.println(joystick_data);
    joystick_data.toCharArray(packetBuffer, sizeof(packetBuffer));
    
    pch = strtok (packetBuffer,"|");
    while (pch != NULL)
    {
      packets[i] = pch;      
      pch = strtok (NULL, "|");
      i++;
    }
    Serial.printf("packets[5] = %s\n",packets[5]);
    Serial.printf("packets[6] = %s\n",packets[6]);
    Serial.printf("packets[7] = %s\n",packets[7]);
    Serial.printf("packets[8] = %s\n",packets[8]);

  
    //if (WiFi.status() == WL_CONNECTED)
    //{ 
      Udp.beginPacket("192.168.4.1", 6666);
      Udp.print(joystick_data);
      Udp.endPacket();
    //}
     
    // into RC matching mode.
    if( strcmp(packets[5], "AA") == 0 && strcmp(packets[6], "AA") == 0 && change_ssid_flag == 0 )
    {
        change_ssid_flag = 1;
        //save ssid into EEPROM address at 5.
        delay(2000);
        String new_ssid = packets[8];
        SaveString(5, new_ssid.c_str());
        WiFi.disconnect();
        WiFi.begin( new_ssid.c_str(), "12345678");
        delay(1000);
        Serial.println("changed SSID.");
        
    }
    
    if( strcmp(packets[7], "AA") == 0 && switch_ap_mode_flag == 0)
    {
      switch_ap_mode_flag = 1;
      WiFi.disconnect();
      WiFi.mode(WIFI_AP); 
      WiFi.softAP("OpenDrone-RC", "12345678");
      delay(500);
      server_AP.on("/", handleRoot);
      server_AP.begin();
    }

    
    
  }  
  //delay(10);
  
  yield();
  
}
