#include <dht11.h>
dht11 DHT;
#include <TimerOne.h>
#include <SoftwareSerial.h>

#define BAUDRATE 115200
//硬件
#define DHT_PIN 2
#define FAN_PIN 3
#define LED_PIN 4

//WiFi
#define SSID "Terabits"
#define PSWD "terabits"

//MQTT
#define URL "a11FZjB7GD7.iot-as-mqtt.cn-shanghai.aliyuncs.com,1883"
#define ClientID "test|securemode=3\\,signmethod=hmacsha1\\,timestamp=789|"
#define UserName "test&a11FZjB7GD7"
#define Password "aa5120f2ebe8f1c68bbf8d61caec5f50f9e27cbc"
//post
#define TOPIC1 "/sys/a11FZjB7GD7/test/thing/event/property/post"
//set
#define TOPIC2 "/sys/a11FZjB7GD7/test/thing/service/property/set"

// USB TTL 检错用
#define _rxpin 7
#define _txpin 8
SoftwareSerial debug(_rxpin, _txpin); // RX, TX

//
unsigned long time;
unsigned long temp;
String inputString = "";     // a String to hold incoming data
bool stringComplete = false; // whether the string is complete
int LIGHT = 0;
int FAN = 0;
double temperature = 22;
int ri1 = 0;
int ri2 = 0;

void setup()
{
    //硬件
    pinMode(FAN_PIN, OUTPUT);
    digitalWrite(FAN_PIN, LOW);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    pinMode(DHT_PIN, INPUT);

    inputString.reserve(200);
    Serial.begin(BAUDRATE);
    debug.begin(BAUDRATE);
    delay(300);
    sendDebug("AT\r");
    delay(500);
    if (Serial.find("OK"))
    {
        debug.println(" AT ready!");
        connectWiFi();
        delay(5000);
        debug.println("connecting to MQTT");
        connectMQTT();
    }
    delay(700);
    Timer1.initialize(10000000);
    Timer1.attachInterrupt(upload);
}

void loop()
{
    //listening
    if (stringComplete)
    {
        debug.println(inputString);
        if (inputString[10] == '0')
        {
            debug.println("Received From TOPIC 0 ");
            int index = inputString.indexOf("fanSwitch");
            if (index != -1)
            {
                FAN = inputString.substring(index + 11, index + 12).toInt();
                //fan
                digitalWrite(FAN_PIN, FAN);
            }

            index = inputString.indexOf("LightSwitch");
            if (index != -1)
            {
                LIGHT = inputString.substring(index + 13, index + 14).toInt();
                //light
                digitalWrite(LED_PIN, LIGHT);
            }
        }
        // clear the string:
        inputString = "";
        stringComplete = false;
    }

    //wait to get temperature
    ri1 += 1;
    if (ri1 > 30000)
    {
        ri2 += 1;
        if (ri2 > 30000)
        {
            ri1 = 0;
            ri2 = 0;
            time = millis();
            debug.print("Time interval:");
            debug.print(time - temp);
            debug.println(" ms");
            temp = time;
            getTemperature();
        }
    }
}

void serialEvent()
{
    while (Serial.available())
    {
        // get the new byte:
        char inChar = (char)Serial.read();
        // add it to the inputString:
        inputString += inChar;
        // if the incoming character is a newline, set a flag so the main loop can
        // do something about it:
        if (inChar == '\n')
        {
            stringComplete = true;
        }
    }
}

void upload()
{
    debug.println("uploading...");
    String buf = "";
    buf += "{\"id\":\"123\",\"version\":\"1.0\",\"params\":{";
    buf += "\"LightSwitch\":";
    buf += String(LIGHT);
    buf += ",\"CurrentTemperature\":";
    buf += String(temperature);
    buf += ",\"fanSwitch\":";
    buf += String(FAN);
    buf += "},\"method\":\"thing.event.property.post\"}";
    //debug.println(buf);
    int num = buf.length() + 3;
    String cmd = "AT+MQTTSEND=" + String(num) + "\r";
    Serial.println(cmd);
    //delay(200);
    Serial.println(buf);
    //delay(500);
    // clear the string:
    inputString = "";
    stringComplete = false;
}

void getTemperature()
{
    //getTemperature
    int chk;
    chk = DHT.read(DHT_PIN); // READ DATA
    switch (chk)
    {
    case DHTLIB_OK:
        temperature = DHT.temperature;
        debug.println("Temperature updated:\t" + String(temperature));
        break;
    case DHTLIB_ERROR_CHECKSUM:
        debug.print("Checksum error,\t");
        break;
    case DHTLIB_ERROR_TIMEOUT:
        //debug.print("Time out error,\t");
        break;
    default:
        debug.print("Unknown error,\t");
        break;
    }
}

boolean connectWiFi()
{
    // 连接到Wifi
    String cmd = "AT+WJAP=";
    cmd += SSID;
    cmd += ",";
    cmd += PSWD;
    cmd += "\r";
    sendDebug(cmd);
    int i = 0;
    i = 0;
    while (1)
    {
        if (Serial.find("OK"))
        {
            debug.println(" WiFi OK!");
            return true;
        }
        else
        {
            debug.println(" waiting...");
        }
        delay(1000);
        i += 1;
        if (i > 10)
        {
            debug.println("try to connect WiFi failed");
            return false;
        }
    }
}

void connectMQTT()
{
    int i;
    //UserName
    String cmd = "AT+MQTTAUTH=";
    cmd += UserName;
    cmd += ",";
    cmd += Password;
    cmd += "\r";
    sendDebug(cmd);
    i = 0;
    while (1)
    {
        if (Serial.find("OK"))
        {
            debug.println(" AUTH OK!");
            break;
        }
        else
        {
            debug.println(" waiting...");
        }
        delay(1000);
        i += 1;
        if (i > 5)
        {
            debug.println("AUTH failed");
            break;
        }
    }

    //ClientID
    cmd = "AT+MQTTCID=";
    cmd += ClientID;
    cmd += "\r";
    sendDebug(cmd);
    i = 0;
    while (1)
    {
        if (Serial.find("OK"))
        {
            debug.println(" ClientID OK!");
            break;
        }
        else
        {
            debug.println(" waiting...");
        }
        delay(1000);
        i += 1;
        if (i > 5)
        {
            debug.println("ClientID failed");
            break;
        }
    }
    //URL
    cmd = "AT+MQTTSOCK=";
    cmd += URL;
    cmd += "\r";
    sendDebug(cmd);
    i = 0;
    while (1)
    {
        if (Serial.find("OK"))
        {
            debug.println(" URL OK!");
            break;
        }
        else
        {
            debug.println(" waiting...");
        }
        delay(1000);
        i += 1;
        if (i > 5)
        {
            debug.println("URL failed");
            break;
        }
    }
    //KEEPALIVE
    cmd = "AT+MQTTKEEPALIVE=60\r";
    sendDebug(cmd);
    i = 0;
    while (1)
    {
        if (Serial.find("OK"))
        {
            debug.println(" KEEPALIVE OK!");
            break;
        }
        else
        {
            debug.println(" waiting...");
        }
        delay(1000);
        i += 1;
        if (i > 5)
        {
            debug.println("KEEPALIVE failed");
            break;
        }
    }

    //RECONNECT
    cmd = "AT+MQTTRECONN=ON\r";
    sendDebug(cmd);
    i = 0;
    while (1)
    {
        if (Serial.find("OK"))
        {
            debug.println(" RECONN OK!");
            break;
        }
        else
        {
            debug.println(" waiting...");
        }
        delay(1000);
        i += 1;
        if (i > 5)
        {
            debug.println("RECONN failed");
            break;
        }
    }

    //START
    cmd = "AT+MQTTSTART\r";
    sendDebug(cmd);
    i = 0;
    while (1)
    {
        if (Serial.find("SUCCESS"))
        {
            debug.println(" START OK!");
            break;
        }
        else
        {
            debug.println(" waiting...");
        }
        delay(1000);
        i += 1;
        if (i > 5)
        {
            debug.println("START failed");
            break;
        }
    }

    //SUBSCRIBE
    cmd = "AT+MQTTSUB=0,";
    cmd += TOPIC2;
    cmd += ",1\r";
    sendDebug(cmd);
    i = 0;
    while (1)
    {
        if (Serial.find("SUCCESS"))
        {
            debug.println(" SUBSCRIBE OK!");
            break;
        }
        else
        {
            debug.println(" waiting...");
        }
        delay(1000);
        i += 1;
        if (i > 5)
        {
            debug.println("SUBSCRIBE failed");
            break;
        }
    }

    //PUBSCRIBE
    cmd = "AT+MQTTPUB=";
    cmd += TOPIC1;
    cmd += ",1\r";
    sendDebug(cmd);
    i = 0;
    while (1)
    {
        if (Serial.find("OK"))
        {
            debug.println(" PUBSCRRIBE OK!");
            break;
        }
        else
        {
            debug.println(" waiting...");
        }
        delay(1000);
        i += 1;
        if (i > 5)
        {
            debug.println("PUBSCRIBE failed");
            break;
        }
    }
}

void sendDebug(String cmd)
{
    // 传到 USB TTL
    debug.print("SEND: ");
    debug.println(cmd);
    Serial.println(cmd);
}