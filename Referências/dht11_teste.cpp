/******************************************************************************
DHT11.cpp
*******************************************************************************/
#include "dht11.h"

using namespace std;
#define MAXTIMINGS 85
#define DHTPIN 7
vector<int> dht11_dat(5, 0);

int read_dht11_dat(){
	int laststate = HIGH;
	int counter = 0;
	int j = 0, i;
	dht11_dat.assign(5,0);

	gpioSetMode(DHTPIN, PI_OUTPUT);
	gpioRead(DHTPIN, LOW);
	delay(20);
	gpioWrite(DHTPIN, HIGH);
	delayMicroseconds(40);
	gpioSetMode( DHTPIN, PI_INPUT);

	for (i = 0; i < MAXTIMINGS; i++){
		counter = 0;
		while (gpioRead(DHTPIN) == laststate){
			counter++;
			delayMicroseconds(1);
			if (counter == 0xFF)
				break;
		}
		laststate = gpioRead(DHTPIN);

		if (counter == 0xFF)
			break;

		if ((i >= 4) && (i % 2 == 0)){
			dht11_dat[j / 8] <<= 1;
			if (counter > 50)
				dht11_dat[j / 8] |= 1;
			j++;
		}
	}
	if ((j >= 40) && (dht11_dat[4] == ( (dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF)))
		//cout << "Umidade = " << dht11_dat[0] << "." << dht11_dat[1] << " % Temperatura = " << dht11_dat[2] << "." << dht11_dat[3] << " *C" << '\n';
        //_umidade = 
        //_temperatura =
    return dht11_dat[4];
}


