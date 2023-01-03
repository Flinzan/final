#include "mbed.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"
#include "bbcar.h"
#include "parallax_servo.h"
#include <cstdio>

Thread brain2;
float mem[2];
float speed=0;
float val;
float nowdata;
int counting1=0;
int task0=0;
int task1=0;
int task2=0;
Timer time1;
DigitalInOut ping(D3);
PwmOut servo0(D13);
PwmOut servo1(D11);
Ticker servo_ticker;
Ticker servo_feedback_ticker;
PwmOut pin11(D11), pin13(D13);
PwmIn pin10(D10), pin12(D12);
Timer t;
BBCar car(pin13, pin12, pin11, pin10, servo_ticker, servo_feedback_ticker);

BusInOut qti_pin(D4,D5,D6,D7);
DigitalIn pintest1 (D4);
DigitalIn pintest2 (D5);
DigitalIn pintest3 (D6);
DigitalIn pintest4 (D7);

void left_servocontrol(int speed) {
   if (speed > 200)       speed = 200;
   else if (speed < -200) speed = -200;

   servo0 = (CENTER_BASE + speed)/20000.0f;
}
void right_servocontrol(int speed) {
   if (speed > 200)       speed = 200;
   else if (speed < -200) speed = -200;

   servo1 = (CENTER_BASE + speed)/20000.0f;
}

void pinging(){
    while(1){
        ping.output();
        ping = 0; wait_us(200);
        ping = 1; wait_us(5);
        ping = 0; wait_us(5);

        ping.input();
        while(ping.read() == 0);
        t.start();
        while(ping.read() == 1);
        val = t.read();
        printf("Ping = %lf\r\n", val*17700.4f);
        t.stop();
        t.reset();
        if(val*17700.4f<10){
            task2=1;
            break;
        }
    }
}



// GLOBAL VARIABLES
WiFiInterface *wifi;
InterruptIn btn2(BUTTON1);
//InterruptIn btn3(SW3);
volatile int message_num = 0;
volatile int arrivedcount = 0;
volatile bool closed = false;

const char* topic = "Mbed";

Thread mqtt_thread(osPriorityHigh);
EventQueue mqtt_queue;

void messageArrived(MQTT::MessageData& md) {
    MQTT::Message &message = md.message;
    char msg[300];
    sprintf(msg, "Message arrived: QoS%d, retained %d, dup %d, packetID %d\r\n", message.qos, message.retained, message.dup, message.id);
    printf(msg);
    ThisThread::sleep_for(2000ms);
    char payload[300];
    sprintf(payload, "Payload %.*s\r\n", message.payloadlen, (char*)message.payload);
    printf(payload);
    ++arrivedcount;
}

void publish_message(MQTT::Client<MQTTNetwork, Countdown>* client) {
    message_num++;
    MQTT::Message message;
    char buff[100];
    sprintf(buff, "QoS0 Hello, Python! #%d", message_num);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*) buff;
    message.payloadlen = strlen(buff) + 1;
    int rc = client->publish(topic, message);

    printf("rc:  %d\r\n", rc);
    printf("Puslish message: %s\r\n", buff);
}

void publish_speed(float speed){
    MQTT::Client<MQTTNetwork, Countdown>* client;
    message_num++;
    MQTT::Message message;
    char buff[100];
    sprintf(buff, "the speed! #%f", speed);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*) buff;
    message.payloadlen = strlen(buff) + 1;
    int rc = client->publish(topic, message);

    printf("rc:  %d\r\n", rc);
    printf("Puslish message: %s\r\n", buff);
}

void close_mqtt() {
    closed = true;
}

int main() {
    parallax_qti qti1(qti_pin);
    int pattern;


    wifi = WiFiInterface::get_default_instance();
    if (!wifi) {
            printf("ERROR: No WiFiInterface found.\r\n");
            return -1;
    }


    printf("\nConnecting to %s...\r\n", MBED_CONF_APP_WIFI_SSID);
    int ret = wifi->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);
    if (ret != 0) {
            printf("\nConnection error: %d\r\n", ret);
            return -1;
    }


    NetworkInterface* net = wifi;
    MQTTNetwork mqttNetwork(net);
    MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);

    //TODO: revise host to your IP
    const char* host = "172.20.10.2";
    const int port=1883;
    printf("Connecting to TCP network...\r\n");
    printf("address is %s/%d\r\n", host, port);

    int rc = mqttNetwork.connect(host, port);//(host, 1883);
    if (rc != 0) {
            printf("Connection error.");
            return -1;
    }
    printf("Successfully connected!\r\n");

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = "Mbed";

    if ((rc = client.connect(data)) != 0){
            printf("Fail to connect MQTT\r\n");
    }
    if (client.subscribe(topic, MQTT::QOS0, messageArrived) != 0){
            printf("Fail to subscribe\r\n");
    }


    brain2.start(pinging);
       while(1) {








        pattern = (int)qti1;
        //printf("%d\n",pattern);

        //printf("%d",pintest4.read());
        //printf("%d",pintest3.read());
        //printf("%d",pintest2.read());
        //printf("%d\n",pintest1.read());


        //printf("%f\n",nowdata);

        //if(nowdata<10){
        //    printf("%f\n",nowdata);
        //    car.stop();
        //    ThisThread::sleep_for(5s);
        //}

        switch (pattern) {
        //turn right   
            case 0b1000:
            speed=9;
            left_servocontrol(120);
            right_servocontrol(120); 
            break;
            case 0b1100: 
            speed=7.5;
            left_servocontrol(100);
            right_servocontrol(60);  
            break;
            case 0b0100: 
            speed=6;
            left_servocontrol(80);
            right_servocontrol(30); 
            break;
        //go straight
            case 0b0110:
            speed=6; 
            left_servocontrol(80);
            right_servocontrol(-75);
            break;
        //turn left 
            case 0b0010:
            speed=6; 
            left_servocontrol(-30);
            right_servocontrol(-80); 
            break;
            case 0b0011: 
            speed=7.5;
            left_servocontrol(-60);
            right_servocontrol(-100); 
            break;
            case 0b0001: 
            speed=9;
            left_servocontrol(-120);
            right_servocontrol(-120); 
            break;

        //special case
            case 0b1101:
                car.stop();
                ThisThread::sleep_for(500ms);
                task0=1;
                printf("task0 - turn left!\n");
                car.goStraight(100);
                ThisThread::sleep_for(500ms);
            break;

            case 0b1011: 
                car.stop();
                ThisThread::sleep_for(500ms);
                task1=1; 
                printf("task1 - turn right!\n");
                car.goStraight(100);
                ThisThread::sleep_for(500ms);
            break;

            case 0b1111: 
                if(task0||task1||task2==1){
                    MQTT::Message message;
                    char buff[100];

                    sprintf(buff, "task found");
                    message.qos = MQTT::QOS0;
                    message.retained = false;
                    message.dup = false;
                    message.payload = (void*) buff;
                    message.payloadlen = strlen(buff) + 1;
                    int rc = client.publish(topic, message);

                    printf("rc:  %d\r\n", rc);
                    printf("Puslish message: %s\r\n", buff);
                }
                if(task0==1){
                    car.stop();
                    ThisThread::sleep_for(500ms);
                    car.turn(70,0.1);
                    ThisThread::sleep_for(2s);
                    printf("turned-left!\n");
                    task0=0;
                }

                else if(task1==1){
                    car.stop();
                    ThisThread::sleep_for(500ms);
                    car.turn(70,-0.1);
                    
                    ThisThread::sleep_for(2s);

                    printf("turned-right!\n");
                    task1=0;
                }
                else if (task2==1){
                    printf("found obstacle\n");
                    task2=0;
                }
                else{
                    car.stop();
                    ThisThread::sleep_for(500ms);
                    car.goStraight(100);
                    ThisThread::sleep_for(500ms);
                    task1=0;
                }
                //printf("error distance = %f\n", (car.servo0.targetAngle - car.servo0.angle)*6.5*3.14/360);
            break;
        default: car.goStraight(50);
      }

      ThisThread::sleep_for(10ms);
   }






    for (int counting=0;counting<3;counting++){
            message_num++;
    MQTT::Message message;
    char buff[100];
    sprintf(buff, "QoS0 Hello, Python! #%d", counting);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*) buff;
    message.payloadlen = strlen(buff) + 1;
    int rc = client.publish(topic, message);

    printf("rc:  %d\r\n", rc);
    printf("Puslish message: %s\r\n", buff);
        
    }


    mqtt_thread.start(callback(&mqtt_queue, &EventQueue::dispatch_forever));
    btn2.rise(mqtt_queue.event(&publish_message, &client));
    //btn3.rise(&close_mqtt);

    int num = 0;
    while (num != 5) {
            client.yield(100);
            ++num;
    }

    while (1) {
            if (closed) break;
            client.yield(500);
            ThisThread::sleep_for(1500ms);
    }

    printf("Ready to close MQTT Network......\n");

    if ((rc = client.unsubscribe(topic)) != 0) {
            printf("Failed: rc from unsubscribe was %d\n", rc);
    }
    if ((rc = client.disconnect()) != 0) {
    printf("Failed: rc from disconnect was %d\n", rc);
    }

    mqttNetwork.disconnect();
    printf("Successfully closed!\n");

    return 0;
}