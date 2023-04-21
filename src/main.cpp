#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <ESP32Servo.h>
#include <iostream>
#include <sstream>

#include "Timer.h"
#include "elapsedMillis.h"
#include "expo.h"


// Smartphone: 192.168.4.1
// Prototypes

#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

#define BATT_PIN 33

int tonfolge[3] = {554, 329, 440};
void sendCurrentRobotArmState();
void writeServoValues(int servoIndex, int value);

// uint16_t          servomittearray[NUM_SERVOS] = {}; // Werte fuer Mitte
// von RC22
#define NUM_SERVOS 4
uint16_t          servomittearray[NUM_SERVOS] = {}; // Werte fuer Mitte

uint16_t maxwinkel = 180;


uint8_t buttonstatus = 0;
uint8_t tonindex = 0;
void playTon(int ton);
#define START_TON 0
#define LICHT_ON 1

uint8_t expolevel = 3;

uint16_t ubatt = 0;

int ledintervall = 1000;
Timer timer;
elapsedMillis ledmillis;


struct ServoPins
{
  Servo servo;
  int servoPin;
  String servoName;
  int initialPosition;  
};
std::vector<ServoPins> servoPins = 
{
  { Servo(), 14 , "Rechts", 90},
  { Servo(), 32 , "Links", 90},
  { Servo(), 15 , "Elbow", 90},
  { Servo(), 27 , "Gripper", 90},
};

struct RecordedStep
{
  int servoIndex;
  int value;
  int delayInStep;
};
std::vector<RecordedStep> recordedSteps;

bool recordSteps = false;
bool playRecordedSteps = false;

unsigned long previousTimeInMilli = millis();

const char* ssid     = "AP EPS";
const char* password = "eiramsor44wl";

AsyncWebServer server(80);
AsyncWebSocket wsRobotArmInput("/RobotArmInput");

const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <style>

    input[type=button]
    {
      background-color:red;color:white;border-radius:20px;width:100%;height:40px;font-size:20px;text-align:center;
    }
        
    .noselect {
      -webkit-touch-callout: none; /* iOS Safari */
        -webkit-user-select: none; /* Safari */
         -khtml-user-select: none; /* Konqueror HTML */
           -moz-user-select: none; /* Firefox */
            -ms-user-select: none; /* Internet Explorer/Edge */
                user-select: none; /* Non-prefixed version, currently
                                      supported by Chrome and Opera */
    }

    .slidecontainer {
      width: 100%; 
      padding: 10px;
    }

    .slidecontainerV {
      width: 40px;
      height:300px;
      
      border: 1px solid black;

    }

    .slider {
      -webkit-appearance: none;
      width: 100%;
      height: 20px;
      border-radius: 5px;
      background: #d3d3d3;
      outline: 2px;
      opacity: 0.7;
      -webkit-transition: .2s;
      transition: opacity .2s;
    }
    
    .sliderV {
      -webkit-appearance:none;
      
      width: 250px;
      height: 20px;
      data-height: 40px;
      data-width: 40px;

      margin-left: 10px;

      border: 1px solid black;
      border-radius: 5px;
      background: #d3d3d3;
      outline: 2px;
      opacity: 0.7;
      -webkit-transition: .2s;
      transition: opacity .2s;
      transform: rotate(90deg);
      transform-origin: bottom left;
    }

    .slider:hover {
      opacity: 1;
    }
  
    .slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;

      width: 40px;
      height: 40px;
      border-radius: 50%;
      background: blue;
      cursor: pointer;
    }
    .sliderV::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      
      width: 60px;
      height: 60px;
      border-radius: 50%;
      background: red;
      cursor: pointer;

    }

    .slider::-moz-range-thumb {
      width: 40px;
      height: 40px;
      border-radius: 50%;
      background: green;
      cursor: pointer;
    }
    
table,td
{
border: 1px solid black;
  border-collapse: collapse;
}
  
</style>
  
  </head>
  <body class="noselect" align="center" style="background-color:white">
     
    <h1 style="color: teal;text-align:center;">ESP32 Auto</h1>
    
    <table id="mainTable" style="width:400px;left:200px;margin:auto ; table-layout:fixed">

      <tr/><tr/>
      <tr/>
      <td colspan=2 style="text-align:center;font-size:25px"><b>LINKS</b></td>
      
       <td colspan=2 style="text-align:center;font-size:25px"><b>RECHTS</b></td>
      <tr/>
      
      
      <tr height="150px" >
        
        <td colspan=2 style="padding-left:75px">
         <div class="slidecontainerV">
            <input type="range" min="0" max="180" value="90" class="sliderV" id="Links" oninput='sendButtonInput("Links",value)' >
          </div>
        </td>
      
       
        <td colspan=2 style="padding-left:75px">
         <div class="slidecontainerV">
            <input type="range" min="0" max="180" value="90" class="sliderV" id="Rechts" oninput='sendButtonInput("Rechts",value)'>
          </div>
        </td>
      <tr/>

       <tr>
        <td  style="text-align:left;font-size:25px"><b>Signal</b></td>

       <td><input type="button" id="Signal" value="Signal" ontouchend='onclickButton(this)' onclick='onclickButton(this)'></td>
   
      <td><input type="button" id="Licht" value="Licht"  ontouchend='onclickButton(this)' onclick='onclickButton(this)'></td>
     <tr>
          <td  style="text-align:left;font-size:25px"><b>Batt:</b></td>
          <td> <style="text-align:left;font-size:45px" id="ubatt" value="0" > -- </td>
          
     </tr>

     
      <!--
      </tr>  
      <tr>
        <td style="text-align:left;font-size:25px"><b>Shoulder:</b></td>
        <td colspan=2>
         <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Shoulder" oninput='sendButtonInput("Shoulder",value)'>
          </div>
        </td>
      </tr>  
      <tr/><tr/>      
      <tr>
        <td style="text-align:left;font-size:25px"><b>Base:</b></td>
        <td colspan=2>
         <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Base" oninput='sendButtonInput("Base",value)'>
          </div>
        </td>
      </tr> 
      
      
     

      <tr>
        <td style="text-align:left;font-size:25px"><b>Record:</b></td>
        <td><input type="button" id="Record" value="OFF" ontouchend='onclickButton(this)'></td>
        <td></td>
      </tr>
      <tr/><tr/> 
      <tr>
        <td style="text-align:left;font-size:25px"><b>Play:</b></td>

       
       <td><input type="button" id="Play" value="iOFF" ontouchend='onclickButton(this)'></td>
   
        <td><input type="button" id="Test" value="mOFF"  onclick='onclickButton(this)'></td>
     
      
      </tr>  -->    
    </table>
  
    <script>
    
      var webSocketRobotArmInputUrl = "ws:\/\/" + window.location.hostname + "/RobotArmInput";      
      var websocketRobotArmInput;

      function initRobotArmInputWebSocket() 
      {
        websocketRobotArmInput = new WebSocket(webSocketRobotArmInputUrl);
        websocketRobotArmInput.onopen    = onOpen; <!--function(event){};-->
        websocketRobotArmInput.onclose   = function(event){setTimeout(initRobotArmInputWebSocket, 2000);};
        websocketRobotArmInput.onmessage    = function(event)
        {
          console.log(`Received a notification from ${event.origin}`);
          console.log(event);
          var keyValue = event.data.split(",");

          console.log("initRobotArmInputWebSocket keyValue 0: " + keyValue[0] + " keyValue 1: " + keyValue[1]);
           
          var button = document.getElementById(keyValue[0]);
          if(typeof button !== 'undefined' && button !== null) <!-- stackoverflow.com/questions/13885533/it-says-that-typeerror-document -->
          {
            button.value = keyValue[1];
            if (button.id == "Record" || button.id == "Play")
            {
             button.style.backgroundColor = (button.value == "ON" ? "green" : "red");  
              enableDisableButtonsSliders(button);
            }
          }
          if (keyValue[0] == "ubatt")
          {
            console.log("ubatt: " + keyValue[1]);
            var ubattfeld = document.getElementById("ubatt");
            ubattfeld.style.fontWeight = 'bold';
            ubattfeld.style.fontSize = '25px';;
            ubattfeld.style.color = 'blue';
            ubattfeld.innerHTML = keyValue[1];

          }
        };
      }
      
      function onOpen(event) 
      {  
        console.log('Connection opened');
      }
      function sendButtonInput(key, value) 
      {
       
        var data = key + "," + value;
        console.log("key: " + key + " data: " + data);
        websocketRobotArmInput.send(data);

      }
      
      function sendMouseUp(key, value) 
      {
        var data = key + "," + value;
        websocketRobotArmInput.send(data);
      }

      function onclickButton(button) 
      {
        
        //button.value = (button.value == "ON") ? "OFF" : "ON" ;        
        <!--button.value = "Signal"; -->
        button.style.backgroundColor = (button.value == "ON" ? "green" : "red");          
        var value = (button.value ); <!--== "ON") ? 1 : 0 ;-->
        sendButtonInput(button.id, value);
        enableDisableButtonsSliders(button);
      }
      
      function enableDisableButtonsSliders(button)
      {
        if(button.id == "Play")
        {
          var disabled = "auto";
          if (button.value == "ON")
          {
            disabled = "none";            
          }
          document.getElementById("Rechts").style.pointerEvents = disabled;
          document.getElementById("Links").style.pointerEvents = disabled;
          document.getElementById("Gripper").style.pointerEvents = disabled;
          document.getElementById("Elbow").style.pointerEvents = disabled;          
          
          document.getElementById("Record").style.pointerEvents = disabled;
        }
        if(button.id == "Record")
        {
          var disabled = "auto";
          if (button.value == "ON")
          {
            disabled = "none";            
          }
          document.getElementById("Play").style.pointerEvents = disabled;
        }        
      }
           
      window.onload = initRobotArmInputWebSocket;
      document.getElementById("mainTable").addEventListener("touchend", function(event){
        event.preventDefault()
      });      
    </script>
  </body>    
</html>
)HTMLHOMEPAGE";

void handleRoot(AsyncWebServerRequest *request) 
{
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request) 
{
    request->send(404, "text/plain", "File Not Found");
}

void onRobotArmInputWebSocketEvent(AsyncWebSocket *server, 
                      AsyncWebSocketClient *client, 
                      AwsEventType type,
                      void *arg, 
                      uint8_t *data, 
                      size_t len) 
{                      
  switch (type) 
  {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      sendCurrentRobotArmState();
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      AwsFrameInfo *info;
      info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) 
      {
        std::string myData = "";
        myData.assign((char *)data, len);
        std::istringstream ss(myData);
        std::string key, value;
        std::getline(ss, key, ',');
        std::getline(ss, value, ',');
        Serial.printf("Key [%s] Value[%s]\n", key.c_str(), value.c_str()); 
        int valueInt = atoi(value.c_str()); 
        int expovalue = 0;
        if (valueInt > maxwinkel/2)
        {
          expovalue = maxwinkel/2 +  expoarray[expolevel][valueInt - maxwinkel/2];
        }
        else
        {
          expovalue = maxwinkel/2 -  expoarray[expolevel][maxwinkel/2 - valueInt ];
        }
        

        if (key == "Record")
        {
           Serial.printf("key record\n");
          recordSteps = valueInt;
          if (recordSteps)
          {
            recordedSteps.clear();
            previousTimeInMilli = millis();
          }
        }  
        else if (key == "Play")
        {
          Serial.printf("key play\n");
          playRecordedSteps = valueInt;
        }
         else if (key == "Rechts")
        {
          
          Serial.printf("Rechts: [%d]\n",valueInt);
          writeServoValues(0, 90 + ((expovalue) - 90)/3);  // Richtung inv.         
        } 
        else if (key == "Links")
        {
          Serial.printf("Links: [%d]\n",valueInt);
          writeServoValues(1,90 + ((180 - expovalue) - 90)/3);           
        }         
        else if (key == "Gripper")
        {
          writeServoValues(3, valueInt);     
        } 
        else if (key == "Test")
        {
            Serial.printf("TEST\n");
        }
        else if (key == "GripperUp")
        {
            Serial.printf("GripperUp\n");
        }
        
        else if (key == "Signal")
        {
            Serial.printf("Signal\n");
            // playTon(1);
            buttonstatus |= (1<<START_TON); // 
        }
        else if (key == "Licht")
        {
            Serial.printf("Licht\n");
             buttonstatus ^= (1<<LICHT_ON); // 
        }

             
      }
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;  
  }
}

void sendCurrentRobotArmState()
{
  for (int i = 0; i < servoPins.size(); i++)
  {
    wsRobotArmInput.textAll(servoPins[i].servoName + "," + servoPins[i].servo.read());
  }
  wsRobotArmInput.textAll(String("Record,") + (recordSteps ? "ON" : "OFF"));
  wsRobotArmInput.textAll(String("Play,") + (playRecordedSteps ? "ON" : "OFF"));  
}


void notifyClients() {
    wsRobotArmInput.textAll(String("ubatt")+ "," + ubatt);
}


void writeServoValues(int servoIndex, int value)
{
  if (recordSteps)
  {
    RecordedStep recordedStep;       
    if (recordedSteps.size() == 0) // We will first record initial position of all servos. 
    {
      for (int i = 0; i < servoPins.size(); i++)
      {
        recordedStep.servoIndex = i; 
        recordedStep.value = servoPins[i].servo.read(); 
        recordedStep.delayInStep = 0;
        recordedSteps.push_back(recordedStep);         
      }      
    }
     
    unsigned long currentTime = millis();
    recordedStep.servoIndex = servoIndex; 
    recordedStep.value = value; 
    recordedStep.delayInStep = currentTime - previousTimeInMilli;
    recordedSteps.push_back(recordedStep);  
    previousTimeInMilli = currentTime;         
  }
  Serial.printf("pin: [%d] servoindex: [%d] value: [%d]  \n",servoPins[servoIndex].servoPin , servoIndex, value);
  servoPins[servoIndex].servo.write(value);   
}

void playTon(int ton)
{
  // Cis: 554
  // e: 330
  // a: 440
  tone(18,tonfolge[ton],800);
}

void playRecordedRobotArmSteps()
{
  if (recordedSteps.size() == 0)
  {
    return;
  }
  //This is to move servo to initial position slowly. First 4 steps are initial position    
  for (int i = 0; i < 4 && playRecordedSteps; i++)
  {
    RecordedStep &recordedStep = recordedSteps[i];
    int currentServoPosition = servoPins[recordedStep.servoIndex].servo.read();
    while (currentServoPosition != recordedStep.value && playRecordedSteps)  
    {
      currentServoPosition = (currentServoPosition > recordedStep.value ? currentServoPosition - 1 : currentServoPosition + 1); 
      servoPins[recordedStep.servoIndex].servo.write(currentServoPosition);
      wsRobotArmInput.textAll(servoPins[recordedStep.servoIndex].servoName + "," + currentServoPosition);
      delay(50);
    }
  }
  delay(2000); // Delay before starting the actual steps.
  
  for (int i = 4; i < recordedSteps.size() && playRecordedSteps ; i++)
  {
    RecordedStep &recordedStep = recordedSteps[i];
    delay(recordedStep.delayInStep);
    servoPins[recordedStep.servoIndex].servo.write(recordedStep.value);
    wsRobotArmInput.textAll(servoPins[recordedStep.servoIndex].servoName + "," + recordedStep.value);
  }
}

void setUpPinModes()
{
  for (int i = 0; i < servoPins.size(); i++)
  {
    servoPins[i].servo.attach(servoPins[i].servoPin);
    servoPins[i].servo.write(servoPins[i].initialPosition);    
  }
 
  
}


void setup(void) 
{
  setUpPinModes();
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(BATT_PIN, INPUT);
  
  timer.start();
  if(timer.state() == RUNNING) Serial.println("timer running");
  delay(1000);
  timer.stop();
  if(timer.state() == STOPPED) Serial.println("timer stopped");
  Serial.print("time elapsed ms: ");
  Serial.println(timer.read());


  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
      
  wsRobotArmInput.onEvent(onRobotArmInputWebSocketEvent);
  server.addHandler(&wsRobotArmInput);

  server.begin();
  Serial.println("HTTP server started");

}

void loop() 
{
   
  if (ledmillis > ledintervall)
  {
    ledmillis = 0;
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    ubatt = analogRead(BATT_PIN);
    float ubattfloat = ubatt * 2.2*3.3/4096;
    Serial.printf("U Bat: %d %2.2f\n",ubatt, ubattfloat);
    notifyClients();
  }
  wsRobotArmInput.cleanupClients();
  if (buttonstatus & (1<<START_TON))
  {
    playTon(tonindex);
    
    if (tonindex < 3)
    {
        tonindex++;
    }
    else
    {
      buttonstatus &= ~(1<<START_TON);
      tonindex = 0;
    }
    
  }
  if (playRecordedSteps)
  { 
    playRecordedRobotArmSteps();
  }
}
