#include "Particle.h"
#include<Wire.h>
//#include "Adafruit_10DOF_IMU/Adafruit_10DOF_IMU.h"

SYSTEM_THREAD(ENABLED);

int devicesHandler(String data); // forward declaration
void sendData(void);

const unsigned long REQUEST_WAIT_MS = 10000;
const unsigned long RETRY_WAIT_MS = 30000;
const unsigned long SEND_WAIT_MS = 40;

// Sensors

/* Assign a unique ID to the sensors */
// Adafruit_10DOF dof;
// Adafruit_LSM303_Accel_Unified accel(30301);
// Adafruit_LSM303_Mag_Unified mag(30302);
// Adafruit_BMP085_Unified bmp(18001);
// float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;

const int MPU_addr=0x68;  // I2C address of the MPU-6050
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;

enum State { STATE_REQUEST, STATE_REQUEST_WAIT, STATE_CONNECT, STATE_SEND_DATA, STATE_RETRY_WAIT };
State state = STATE_REQUEST;
unsigned long stateTime = 0;
IPAddress serverAddr;
int serverPort;
char nonce[34];
TCPClient client;
bool sensorsInitialized;

void setup() {
	Serial.begin(9600);
	Particle.function("devices", devicesHandler);

	// Initialize sensors
	// sensorsInitialized = (accel.begin() && mag.begin() && bmp.begin());
	Wire.begin();
	Wire.beginTransmission(MPU_addr);
	Wire.write(0x6B);  // PWR_MGMT_1 register
	Wire.write(0);     // set to zero (wakes up the MPU-6050)
	Wire.endTransmission(true);

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
}
void sendData(void){
	Wire.beginTransmission(MPU_addr);
	Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
	Wire.endTransmission(false);
	Wire.requestFrom(MPU_addr,14,true);  // request a total of 14 registers
	AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)     
	AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
	AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
	Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
	GyX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
	GyY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
	GyZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)

	Serial.print(","); Serial.print(GyX);
	Serial.print(","); Serial.print(GyY);
	Serial.print(","); Serial.println(GyZ);
	// Use printf and manually added a \n here. The server code splits on LF only, and using println/
	// printlnf adds both a CR and LF. It's easier to parse with LF only, and it saves a byte when
	// transmitting.
	client.printf("%.3f,%.3f,%.3f,%.3f,%.2f,%.1f\n",
			AcX, AcY, AcZ,
			GyX, GyY, Tmp/340.00+36.53);

	// roll,pitch,heading,altitude,pressure,temperature
	// Example:
	// 0.000,-0.449,-49.091,263.317,982.02,27.5
	// -0.224,-0.224,-48.955,262.719,982.09,27.5
	// 0.000,-0.449,-48.704,262.719,982.09,27.5
	// 0.000,-0.449,-48.704,262.890,982.07,27.5
}


void sendData_old(void) {
	// Called periodically when connected via TCP to the server to update data.
	// Unlike Particle.publish you can push a very large amount of data through this connection,
	// theoretically up to about 800 Kbytes/sec, but really you should probably shoot for something
	// lower than that, especially with the way connection is being served in the node.js server.


	// Taken from the Adafruit 10-DOF example code
	sensors_event_t accel_event;
	sensors_event_t mag_event;
	sensors_event_t bmp_event;
	sensors_vec_t   orientation;

	// Read the accelerometer and magnetometer
	accel.getEvent(&accel_event);
	mag.getEvent(&mag_event);

	// Use the new fusionGetOrientation function to merge accel/mag data
	if (!dof.fusionGetOrientation(&accel_event, &mag_event, &orientation)) {
		// Failed to get data
		return;
	}
	// float orientation.roll
	// float orientation.pitch
	// float orientation.heading

	// Calculate the altitude using the barometric pressure sensor
	bmp.getEvent(&bmp_event);
	if (!bmp_event.pressure) {
		// Failed to get pressure
		return;
	}
	// Get ambient temperature in C
	float temperature;
	bmp.getTemperature(&temperature);

	float altitude = bmp.pressureToAltitude(seaLevelPressure, bmp_event.pressure, temperature);

	// Use printf and manually added a \n here. The server code splits on LF only, and using println/
	// printlnf adds both a CR and LF. It's easier to parse with LF only, and it saves a byte when
	// transmitting.
	client.printf("%.3f,%.3f,%.3f,%.3f,%.2f,%.1f\n",
			orientation.roll, orientation.pitch, orientation.heading,
			altitude, bmp_event.pressure, temperature);

	// roll,pitch,heading,altitude,pressure,temperature
	// Example:
	// 0.000,-0.449,-49.091,263.317,982.02,27.5
	// -0.224,-0.224,-48.955,262.719,982.09,27.5
	// 0.000,-0.449,-48.704,262.719,982.09,27.5
	// 0.000,-0.449,-48.704,262.890,982.07,27.5
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
