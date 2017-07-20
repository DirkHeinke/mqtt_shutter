#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define MOTOR_A_1 5
#define MOTOR_A_2 4
#define MOTOR_A_SWITCH 0
#define MOTOR_A_DOWN_TIME 3000
#define INTERRUPT_DELAY 500
#define MOTOR_A_ENABLE 14

const char* ssid = "****";
const char* password = "****";
const char* mqtt_server = "192.168.178.109";

WiFiClient espClient;
PubSubClient client(espClient);
uint32_t lastMsg = 0;
char msg[50];
uint8_t value = 0;

uint32_t stoptimer = 0;
uint32_t starttimer = 0;
uint32_t motorOffTimer = 0;
bool stop = false;
uint8_t state = 0; // 0 up, 1 moving up, 2 moving down, 3 down

void setup() {
        Serial.begin(115200);
        pinMode(MOTOR_A_1, OUTPUT);
        pinMode(MOTOR_A_2, OUTPUT);
        pinMode(MOTOR_A_ENABLE, OUTPUT);
        pinMode(MOTOR_A_SWITCH, INPUT_PULLUP);



        attachInterrupt(MOTOR_A_SWITCH, stopAInterrupt, FALLING);

        setup_wifi();
        client.setServer(mqtt_server, 1883);
        client.setCallback(callback);

}

void alive() {
    Serial.print(".");
    
}

void stopAInterrupt() {
        stop = true;
}

uint8_t counter = 0;

void loop() {
        if (!client.connected()) {
                Serial.println("Deconnected");
                reconnect();
        }
        client.loop();
        
        counter ++;
        if (counter > 100) {
            alive();
            counter = 0;
        }
        
        if(stop) {
            
            if(millis() < (starttimer + INTERRUPT_DELAY)) {
                stop = false;
                Serial.println("Blocked");
                Serial.println(millis());
            } else if(digitalRead(MOTOR_A_SWITCH) == 0) {
                rotateStopA();
                stop = false;
                if(state == 1) {
                    state = 0; 
                    client.publish("outTopic", "up");
                } else if (state == 2){
                    state = 3;
                    client.publish("outTopic", "down");
                }
                motorOffTimer = millis() + 1000;
                
            }
        }
        
        if(motorOffTimer != 0 && millis() > motorOffTimer) {
            analogWrite(MOTOR_A_ENABLE, 0);
            motorOffTimer = 0;
        }
}


void moveDown() {
        analogWrite(MOTOR_A_ENABLE, 750);
        if(state == 3) {
            return;
        }
        state = 2;
        starttimer = millis();
        // move down
        rotateDownA();
}

void moveUp() {
        analogWrite(MOTOR_A_ENABLE, 1023);
        if(state == 0) {
            return;
        }
        state = 1;
        starttimer = millis();
        // move up
        rotateUpA();
}

void rotateDownA() {
        digitalWrite(MOTOR_A_1, 1);
        digitalWrite(MOTOR_A_2, 0);
}

void rotateUpA() {
        digitalWrite(MOTOR_A_1, 0);
        digitalWrite(MOTOR_A_2, 1);
}

void rotateStopA() {
        digitalWrite(MOTOR_A_1, 0);
        digitalWrite(MOTOR_A_2, 0);
        Serial.println("Stopped");
        Serial.println(millis());
}

void setup_wifi() {

        delay(10);
        Serial.println();
        Serial.print("Connecting to ");
        Serial.println(ssid);

        WiFi.begin(ssid, password);

        while (WiFi.status() != WL_CONNECTED) {
                delay(500);
                Serial.print(".");
        }

        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
        Serial.print("Message arrived [");
        Serial.print(topic);
        Serial.print("] ");
        for (int i = 0; i < length; i++) {
                Serial.print((char)payload[i]);
        }
        Serial.println();

        if ((char)payload[0] == '1') {
                moveDown();
                Serial.println("Moving down");
                Serial.println(millis());
        } else {
                moveUp();
                Serial.println("Moving up");
                Serial.println(millis());

        }

}

void reconnect() {
        // Loop until we're reconnected
        while (!client.connected()) {
                Serial.print("Attempting MQTT connection...");
                // Attempt to connect
                if (client.connect("ESP8266Client", "dirk", "dirkdirk")) {
                        Serial.println("connected");
                        // Once connected, publish an announcement...
                        client.publish("outTopic", "hello world");
                        // ... and resubscribe
                        client.subscribe("inTopic");
                } else {
                        Serial.print("failed, rc=");
                        Serial.print(client.state());
                        Serial.println(" try again in 5 seconds");
                        // Wait 5 seconds before retrying
                        delay(5000);
                }
        }
}