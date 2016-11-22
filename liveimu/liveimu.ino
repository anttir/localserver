#include "MPU6050/I2Cdev.h"
#include "MPU6050/MPU6050.h"
#include "Particle.h"
//#include "Wire.h"
//#include "Adafruit_10DOF_IMU/Adafruit_10DOF_IMU.h"

SYSTEM_THREAD(ENABLED);

int devicesHandler(String data); // forward declaration
void sendData(void);

const unsigned long REQUEST_WAIT_MS = 10000;
const unsigned long RETRY_WAIT_MS = 30000;
const unsigned long SEND_WAIT_MS = 40;

// ---------------------
// Sensors
//
// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#if (I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE) && !defined (SPARK)
    #include "Wire.h"
#endif

// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for InvenSense evaluation board)
// AD0 high = 0x69
MPU6050 accelgyro;
//MPU6050 accelgyro(0x69); // <-- use for AD0 high
int16_t ax, ay, az;
int16_t gx, gy, gz;

const int loopDelay = 10;
//
// ---------------------
// RC-receiver
//
int steeringValue; // Here's where we'll keep our channel values
int throttleValue;
const int throttlePin = 5;
const int steeringPin = 6;

//
// ---------------------

#if defined (SPARK)
#define LED_PIN D7 // (Spark Core is D7)
#else
#define LED_PIN 13 // (Arduino is 13, Teensy is 11, Teensy++ is 6)
#endif
bool blinkState = false;

enum State { STATE_REQUEST, STATE_REQUEST_WAIT, STATE_CONNECT, STATE_SEND_DATA, STATE_RETRY_WAIT };
State state = STATE_REQUEST;
unsigned long stateTime = 0;
IPAddress serverAddr;
int serverPort;
char nonce[34];
TCPClient client;
bool sensorsInitialized;

// =========================

void setup() {
    // join I2C bus (I2Cdev library doesn't do this automatically)
    #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
        Wire.begin();
    #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
        Fastwire::setup(400, true);
    #endif

    // initialize serial communication
    // (38400 chosen because it works as well at 8MHz as it does at 16MHz, but
    // it's really up to you depending on your project)
    Serial.begin(9600);

	Particle.function("devices", devicesHandler);

    // initialize device
    Serial.println("Initializing I2C devices...");
    accelgyro.initialize();

    // verify connection
    Serial.println("Testing device connections...");
    Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

    // use the code below to change accel/gyro offset values
    /*
    Serial.println("Updating internal sensor offsets...");
    // -76	-2359	1688	0	0	0
    Serial.print(accelgyro.getXAccelOffset()); Serial.print("\t"); // -76
    Serial.print(accelgyro.getYAccelOffset()); Serial.print("\t"); // -2359
    Serial.print(accelgyro.getZAccelOffset()); Serial.print("\t"); // 1688
    Serial.print(accelgyro.getXGyroOffset()); Serial.print("\t"); // 0
    Serial.print(accelgyro.getYGyroOffset()); Serial.print("\t"); // 0
    Serial.print(accelgyro.getZGyroOffset()); Serial.print("\t"); // 0
    Serial.print("\n");
    accelgyro.setXGyroOffset(220);
    accelgyro.setYGyroOffset(76);
    accelgyro.setZGyroOffset(-85);
    Serial.print(accelgyro.getXAccelOffset()); Serial.print("\t"); // -76
    Serial.print(accelgyro.getYAccelOffset()); Serial.print("\t"); // -2359
    Serial.print(accelgyro.getZAccelOffset()); Serial.print("\t"); // 1688
    Serial.print(accelgyro.getXGyroOffset()); Serial.print("\t"); // 0
    Serial.print(accelgyro.getYGyroOffset()); Serial.print("\t"); // 0
    Serial.print(accelgyro.getZGyroOffset()); Serial.print("\t"); // 0
    Serial.print("\n");
    */
    
    // set PWM-pins
    pinMode(throttlePin, INPUT); // Set our input pins as such
    pinMode(steeringPin, INPUT);

    // configure Arduino LED for
    pinMode(LED_PIN, OUTPUT);

}

void loop() {

	switch(state) {
	case STATE_REQUEST:
		if (Particle.connected()) {
			Serial.println("sending devicesRequest");
			Particle.publish("devicesRequest", WiFi.localIP().toString().c_str(), 10, PRIVATE);
			state = STATE_REQUEST_WAIT;
			stateTime = millis();
		}
		break;

	case STATE_REQUEST_WAIT:
		if (millis() - stateTime >= REQUEST_WAIT_MS) {
			state = STATE_RETRY_WAIT;
			stateTime = millis();
		}
		break;

	case STATE_CONNECT:
		if (client.connect(serverAddr, serverPort)) {
			client.println("POST /devices HTTP/1.0");
			client.printlnf("Authorization: %s", nonce);
			client.printlnf("Content-Length: 99999999");
		    client.println();
		    state = STATE_SEND_DATA;
		}
		else {
			state = STATE_RETRY_WAIT;
			stateTime = millis();
		}
		break;

	case STATE_SEND_DATA:
		// In this state, we send data until we lose the connection to the server for whatever
		// reason. We'll to the server again.
		if (!client.connected()) {
			Serial.println("server disconnected");
			client.stop();
			state = STATE_RETRY_WAIT;
			stateTime = millis();
			break;
		}

		if (millis() - stateTime >= SEND_WAIT_MS) {
			stateTime = millis();

			sendData();
		}
		break;

	case STATE_RETRY_WAIT:
		if (millis() - stateTime >= RETRY_WAIT_MS) {
			state = STATE_REQUEST;
		}
		break;
	}
	delay(loopDelay);
}
void sendData(void){
    
    // Read PWM-values
    steeringValue = pulseIn(throttlePin, HIGH); // Read the pulse width of 
    throttleValue = pulseIn(steeringPin, HIGH); // each channel
  
    // read raw accel/gyro measurements from device
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    // these methods (and a few others) are also available
    //accelgyro.getAcceleration(&ax, &ay, &az);
    //accelgyro.getRotation(&gx, &gy, &gz);

    // display tab-separated accel/gyro x/y/z values
    //Serial.print("a/g:\t");
    Serial.print(ax); Serial.print(",");
    Serial.print(ay); Serial.print(",");
    Serial.print(az); Serial.print(",");
    Serial.print(gx); Serial.print(",");
    Serial.print(gy); Serial.print(",");
    Serial.print(gz); Serial.print(",");
    Serial.print(steeringValue); Serial.print(",");
    Serial.println(throttleValue);

	// Use printf and manually added a \n here. The server code splits on LF only, and using println/
	// printlnf adds both a CR and LF. It's easier to parse with LF only, and it saves a byte when
	// transmitting.
//	client.printf("%.3f,%.3f,%.3f,%.3f,%.2f,%.1f\n",
	client.printf("%d,%d,%d,%d,%d,%d,%d,%d\n",
			ax, ay, az,
			gx, gy, gz,
			steeringValue, throttleValue);
//	client.printf("0.000,-0.449,-49.091,263.317,982.02,27.5\n");

	//client.printf("%.3f,%.3f,%.3f,%.3f,%.2f,%.1f\n",
	//		orientation.roll, orientation.pitch, orientation.heading,
	//		altitude, bmp_event.pressure, temperature);
	
	// roll,pitch,heading,altitude,pressure,temperature
	// Example:
	// 0.000,-0.449,-49.091,263.317,982.02,27.5
	// -0.224,-0.224,-48.955,262.719,982.09,27.5
	// 0.000,-0.449,-48.704,262.719,982.09,27.5
	// 0.000,-0.449,-48.704,262.890,982.07,27.5
	
	// blink LED to indicate activity
    blinkState = !blinkState;
    digitalWrite(LED_PIN, blinkState);
}



int devicesHandler(String data) {
	Serial.printlnf("devicesHandler data=%s", data.c_str());
	int addr[4];

	if (sscanf(data, "%u.%u.%u.%u,%u,%32s", &addr[0], &addr[1], &addr[2], &addr[3], &serverPort, nonce) == 6) {
		serverAddr = IPAddress(addr[0], addr[1], addr[2], addr[3]);
		Serial.printlnf("serverAddr=%s serverPort=%u nonce=%s", serverAddr.toString().c_str(), serverPort, nonce);
		state = STATE_CONNECT;
	}
	return 0;
}
