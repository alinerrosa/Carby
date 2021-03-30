#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <vector>

using namespace std;
#define MAXTIMINGS 85
#define DHTPIN 7
vector<int> dht11_dat(5, 0);

void read_dht11_dat(){
	int laststate = HIGH;
	int counter = 0;
	int j = 0, i;
	dht11_dat.assign(5,0);

	pinMode(DHTPIN, OUTPUT);
	digitalWrite(DHTPIN, LOW);
	delay(20);
	digitalWrite(DHTPIN, HIGH);
	delayMicroseconds(40);
	pinMode( DHTPIN, INPUT );

	for (i = 0; i < MAXTIMINGS; i++){
		counter = 0;
		while (digitalRead(DHTPIN) == laststate){
			counter++;
			delayMicroseconds(1);
			if (counter == 0xFF)
				break;
		}
		laststate = digitalRead(DHTPIN);

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
		printf( "Umidade = %d.%d %% Temperatura = %d.%d *C\n", dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3]);
}

int main(){
	if (wiringPiSetup() == -1)
		exit(1);
	while (1){
		read_dht11_dat();
		delay(3000);
	}
	return(0);
}
