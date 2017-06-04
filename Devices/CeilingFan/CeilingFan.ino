/*
 * ESP8266 Web server with Web Socket to control an LED.
 *
 * The web server keeps all clients' LED status up to date and any client may
 * turn the LED on or off.
 *
 * For example, clientA connects and turns the LED on. This changes the word
 * "LED" on the web page to the color red. When clientB connects, the word
 * "LED" will be red since the server knows the LED is on.  When clientB turns
 * the LED off, the word LED changes color to black on clientA and clientB web
 * pages.
 *
 * References:
 *
 * https://github.com/Links2004/arduinoWebSockets
 *
 */
 
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>

#define MEM_ALOC_SIZE 4

const char* host = "CeilingFan";
const char* ssid = "RThomaz";
const char* password = "2919517400";

// These constants won't change.  They're used to give names
// to the pins used:
const int digitalOutPinLamp = 5;                      // Digital output pin that the LED is attached to
const int digitalOutPinFanDirectionForward = 4;       // Digital output pin that the Fan Direction Forward is attached to
const int digitalOutPinFanDirectionReverse = 13;      // Digital output pin that the Fan Direction Reverse is attached to
const int analogOutPinFanSpeed = 16;                  // Analog output pin that the Fan Speed is attached to

const int addressLastSpeedFanDirectionForward = 0;    // Address of last speed of fan direction forward
const int addressLastSpeedFanDirectionReverse = 1;    // Address of default value of fan direction reverse

const String fanDirectionNone = "none";
const String fanDirectionForward = "forward";
const String fanDirectionReverse = "reverse";

ESP8266WebServer server(80);

static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <style>
        /* Center the loader */
        .loader {
            position: absolute;
            left: 50%;
            top: 50%;
            z-index: 1;
            width: 150px;
            height: 150px;
            margin: -75px 0 0 -75px;
            border: 16px solid #f3f3f3;
            border-radius: 50%;
            border-top: 16px solid #3498db;
            width: 120px;
            height: 120px;
            -webkit-animation: spin 2s linear infinite;
            animation: spin 2s linear infinite;
            display:none;
        }

        @-webkit-keyframes spin {
            0% {
                -webkit-transform: rotate(0deg);
            }

            100% {
                -webkit-transform: rotate(360deg);
            }
        }

        @keyframes spin {
            0% {
                transform: rotate(0deg);
            }

            100% {
                transform: rotate(360deg);
            }
        }        
    </style>
    <title></title>
  <meta charset='utf-8' />
</head>
<body>
    <form id="form" action='' title='Lâmpada'>
        <fieldset>
            <legend>Lâmpada</legend>
            <input type='radio' name='optLamp' value='true'> Ligado<br>
            <input type='radio' name='optLamp' value='false'> Desligado
        </fieldset>
        <fieldset>
            <legend>Modo</legend>
            <input type='radio' name='optFanDirection' value='forward'> Ventilador<br>
            <input type='radio' name='optFanDirection' value='none'> Desligado<br>
            <input type='radio' name='optFanDirection' value='reverse'> Exaustor
        </fieldset>
        <fieldset>
            <legend>Velocidade</legend>
            <input type='range' id="rngFanSpeed" min='0' max='255' />
        </fieldset>
    </form>
    <div class="loader"></div>
</body>
</html>

<script src='https://cdnjs.cloudflare.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>

<script type='text/javascript'>

    var url = 'http://192.168.0.25/';

    jQuery.ajaxSetup({
        beforeSend: function () {
            $('.loader').show();
            $('#form').hide();
        },
        complete: function () {
            $('.loader').hide();
            $('#form').show();
        },
        success: function () { }
    });

    $(document).ready(function () {

        $.ajax({
            url: url + 'getCurrent',
            type: 'get',
            data: null,
            success: function (response) {
                response = convertToJSON(response);
                $("[name=optLamp]").val([response.lamp]);
                $("[name=optFanDirection]").val([response.fanDirection]);
                $('#rngFanSpeed').val(response.fanSpeed);
            },
            error: function (xhr) {
                //Do Something to handle error
            }
        });

        $('input[type=radio][name=optLamp]').change(function () {
            var data = 'value=' + this.value;
            $.ajax({
                url: url + 'lamp',
                type: 'post',
                data: data,
                success: function (response) {
                    //Do Something                    
                },
                error: function (xhr) {
                    //Do Something to handle error
                }
            });
        });

        $('input[type=radio][name=optFanDirection]').change(function () {
            var fanDirection = this.value;
            var data = 'value=' + fanDirection;
            $.ajax({
                url: url + 'fanDirection',
                type: 'post',
                data: data,
                success: function (response) {
                    response = convertToJSON(response);
                    $('#rngFanSpeed').val(response.fanSpeed);
                    $("#rngFanSpeed").prop('disabled', fanDirection == 'none');
                },
                error: function (xhr) {
                    //Do Something to handle error
                }
            });
        });

        $('#rngFanSpeed').on('change', function () {
            var fanDirection = $('input[type=radio][name=optFanDirection]:checked').val();
            var data = 'fanDirection=' + fanDirection + '&value=' + this.value;
            $.ajax({
                url: url + 'fanSpeed',
                type: 'post',
                data: data,
                success: function (response) {
                    //Do Something                    
                },
                error: function (xhr) {
                    //Do Something to handle error
                }
            });            
        });

    });

    function convertToJSON(value) {
        value = value.replace(/'/g, '!@#').replace(/"/g, "'").replace(/!@#/g, '"');
        return JSON.parse(value);
    }

</script>
)rawliteral";

byte getLastFanSpeed(String fanDirection){
  if (fanDirection == fanDirectionForward)
    return EEPROM.read(addressLastSpeedFanDirectionForward);
  else if (fanDirection == fanDirectionReverse)
    return EEPROM.read(addressLastSpeedFanDirectionReverse);
  else if (fanDirection == fanDirectionNone)
    return 0;
}

void setLastFanSpeed(String fanDirection, byte value){
  if (fanDirection == fanDirectionForward)
    EEPROM.write(addressLastSpeedFanDirectionForward, value);
  else if (fanDirection == fanDirectionReverse)
    EEPROM.write(addressLastSpeedFanDirectionReverse, value);  
  EEPROM.commit();
}

String getCurrentFanDirection(){
  int forward = digitalRead(digitalOutPinFanDirectionForward);
  int reverse = digitalRead(digitalOutPinFanDirectionReverse);
  if(forward == LOW && reverse == LOW){
    return fanDirectionNone;
  }
  else if(forward == HIGH && reverse == LOW){
    return fanDirectionForward;
  }
  else if(forward == LOW && reverse == HIGH){
    return fanDirectionReverse;
  }
  else{
    //erro 
  }  
}

bool getCurrentLamp(){
  int value = digitalRead(digitalOutPinLamp);
  return value == HIGH;
}

String convertBoolToString(bool value){
  if(value){
    return "true";
  }
  else{
    return "false";
  }
}

void setup(void){
  
  Serial.begin(9600);

  EEPROM.begin(MEM_ALOC_SIZE);
  
  // prepare GPIO Lamp
  pinMode(digitalOutPinLamp, OUTPUT);
  digitalWrite(digitalOutPinLamp, LOW);

  // prepare GPIO FanDirectionForward
  pinMode(digitalOutPinFanDirectionForward, OUTPUT);
  digitalWrite(digitalOutPinFanDirectionForward, LOW);

  // prepare GPIO FanDirectionReverse
  pinMode(digitalOutPinFanDirectionReverse, OUTPUT);
  digitalWrite(digitalOutPinFanDirectionReverse, LOW);
  
  Serial.println();
  Serial.println("Booting Sketch...");
  
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  
  if(WiFi.waitForConnectResult() == WL_CONNECTED){
    
    MDNS.begin(host);
      
    server.on("/", HTTP_GET, [](){
      
      String serverIndex = "";

      serverIndex += "<!DOCTYPE html>\r\n";
      serverIndex += "<html>\r\n";
      serverIndex += "<head>\r\n";
      serverIndex += "    <title></title>\r\n";
      serverIndex += "  <meta charset='utf-8' />\r\n";
      serverIndex += "</head>\r\n";
      serverIndex += "<body>\r\n";
      serverIndex += "    <form action='' title='Lâmpada'>\r\n";
      serverIndex += "        <fieldset>\r\n";
      serverIndex += "            <legend>Lâmpada</legend>\r\n";
      serverIndex += "            <input type='radio' name='optLampadaOnOff' value='on'> Ligado<br>\r\n";
      serverIndex += "            <input type='radio' name='optLampadaOnOff' value='off'> Desligado\r\n";
      serverIndex += "        </fieldset>\r\n";
      serverIndex += "        <fieldset>\r\n";
      serverIndex += "            <legend>Ventilador</legend>\r\n";
      serverIndex += "            <input type='radio' name='optVentiladorOnOff' value='on'> Ligado<br>\r\n";
      serverIndex += "            <input type='radio' name='optVentiladorOnOff' value='off'> Desligado\r\n";
      serverIndex += "        </fieldset>\r\n";
      serverIndex += "        <fieldset>\r\n";
      serverIndex += "            <legend>Direção</legend>\r\n";
      serverIndex += "            <input type='radio' name='optVentiladorDirection' value='ventilador'> Ventilador<br>\r\n";
      serverIndex += "            <input type='radio' name='optVentiladorDirection' value='exaustor'> Exaustor\r\n";
      serverIndex += "        </fieldset>\r\n";
      serverIndex += "        <fieldset>\r\n";
      serverIndex += "            <legend>Velocidade</legend>\r\n";
      serverIndex += "            <input type='range' min='0' max='100' />\r\n";
      serverIndex += "        </fieldset>\r\n";
      serverIndex += "    </form>\r\n";
      serverIndex += "</body>\r\n";
      serverIndex += "</html>\r\n";
      serverIndex += "<script src='https://cdnjs.cloudflare.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>\r\n";
      serverIndex += "<script type='text/javascript'>\r\n";
      serverIndex += "    var url = 'http://192.168.0.25/';\r\n";
      serverIndex += "    $(document).ready(function () {\r\n";
      serverIndex += "        $('input[type=radio][name=optLampadaOnOff]').change(function () {\r\n";
      serverIndex += "            var postName = '';\r\n";
      serverIndex += "            if (this.value == 'on') {\r\n";
      serverIndex += "                postName += 'led5_on';\r\n";
      serverIndex += "            }\r\n";
      serverIndex += "            else if (this.value == 'off') {\r\n";
      serverIndex += "                postName += 'led5_off';\r\n";
      serverIndex += "            }\r\n";
      serverIndex += "            $.ajax({\r\n";
      serverIndex += "                url: url + postName,\r\n";
      serverIndex += "                type: 'post',\r\n";
      serverIndex += "                data: null,\r\n";
      serverIndex += "                success: function (response) {\r\n";
      serverIndex += "                    //Do Something                    \r\n";
      serverIndex += "                },\r\n";
      serverIndex += "                error: function (xhr) {\r\n";
      serverIndex += "                    //Do Something to handle error\r\n";
      serverIndex += "                }\r\n";
      serverIndex += "            });\r\n";
      serverIndex += "        });\r\n";
      serverIndex += "        $('input[type=radio][name=optVentiladorOnOff]').change(function () {\r\n";
      serverIndex += "            var postName = '';\r\n";
      serverIndex += "            if (this.value == 'on') {\r\n";
      serverIndex += "                postName += 'led4_on';\r\n";
      serverIndex += "            }\r\n";
      serverIndex += "            else if (this.value == 'off') {\r\n";
      serverIndex += "                postName += 'led4_off';\r\n";
      serverIndex += "            }\r\n";
      serverIndex += "            $.ajax({\r\n";
      serverIndex += "                url: url + postName,\r\n";
      serverIndex += "                type: 'post',\r\n";
      serverIndex += "                data: null,\r\n";
      serverIndex += "                success: function (response) {\r\n";
      serverIndex += "                    //Do Something                    \r\n";
      serverIndex += "                },\r\n";
      serverIndex += "                error: function (xhr) {\r\n";
      serverIndex += "                    //Do Something to handle error\r\n";
      serverIndex += "                }\r\n";
      serverIndex += "            });\r\n";
      serverIndex += "        });\r\n";
      serverIndex += "    });\r\n";
      serverIndex += "</script>\r\n";

      // send to client
      server.send(200, "text/html", INDEX_HTML);
    });

    server.on("/getCurrent", HTTP_GET, [](){
      String fanDirection = getCurrentFanDirection();
      bool lamp = getCurrentLamp();
      byte fanSpeed = getLastFanSpeed(fanDirection);
      String jsonResult = "{'lamp':" + convertBoolToString(lamp) + ",'fanDirection':'" + fanDirection + "','fanSpeed':" + String(fanSpeed) + "}";     
      server.send(200, "text/html", jsonResult);
    });
    
    server.on("/lamp", HTTP_POST, [](){      
      // read the data in value:
      String value = server.arg("value");
      if (value == "true"){
        // change the digital out value to true:
        digitalWrite(digitalOutPinLamp, HIGH);
      }
      else if(value == "false"){
        // change the digital out value to false:
        digitalWrite(digitalOutPinLamp, LOW);
      }  
      else{
        //error 
      }          
      // send to client
      server.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");      
      // print the results to the serial monitor:
      Serial.print("Post Method = ");
      Serial.print("lamp");
      Serial.print(" | ");
      Serial.print("value = ");
      Serial.println(value);
    });
    
    server.on("/fanDirection", HTTP_POST, [](){      
      // read the data in value:
      String value = server.arg("value");            
      if (value == fanDirectionForward){
        // change the digital out value to ventilator:
        digitalWrite(digitalOutPinFanDirectionForward, HIGH);
        digitalWrite(digitalOutPinFanDirectionReverse, LOW);        
      }
      else if(value == fanDirectionReverse){
        // change the digital out value to fumeCupboard:
        digitalWrite(digitalOutPinFanDirectionForward, LOW);
        digitalWrite(digitalOutPinFanDirectionReverse, HIGH);
      }  
      else if(value == fanDirectionNone){
        // change the digital out value to none:
        digitalWrite(digitalOutPinFanDirectionForward, LOW);
        digitalWrite(digitalOutPinFanDirectionReverse, LOW);
      }  
      else{
        //error 
      }               
      // change the analog out value to fan speed:
      byte fanSpeed = getLastFanSpeed(value);
      analogWrite(analogOutPinFanSpeed, fanSpeed);
      // send to client      
      String jsonResult = "{'fanSpeed':'" + String(fanSpeed) + "'}";
      server.send(200, "text/plain", jsonResult);      
      // print the results to the serial monitor:
      Serial.print("Post Method = ");
      Serial.print("fanDirection");
      Serial.print(" | ");
      Serial.print("value = ");
      Serial.print(value);
      Serial.print(" | ");
      Serial.print("fanSpeed = ");
      Serial.println(fanSpeed);
    });

    server.on("/fanSpeed", HTTP_POST, [](){            
      // read the data in value:
      String fanDirection = server.arg("fanDirection");
      int value = server.arg("value").toInt();
      // change the analog out value:
      analogWrite(analogOutPinFanSpeed, value);
      setLastFanSpeed(fanDirection, value);      
      // send to client
      server.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
      // print the results to the serial monitor:
      Serial.print("Post Method = ");
      Serial.print("speed");
      Serial.print(" | ");
      Serial.print("fanDirection = ");
      Serial.print(fanDirection);  
      Serial.print(" | ");
      Serial.print("value = ");
      Serial.println(value);  
    });
    
    server.begin();
    MDNS.addService("http", "tcp", 80);
  
    Serial.printf("Ready! Open http://%s.local in your browser\n", host);
  } else {
    Serial.println("WiFi Failed");
  }
}
 
void loop(void){
  server.handleClient();
  delay(1);
} 
