#include <pigpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fstream>
#include <limits.h>
#include <string>

using namespace std;
#define DHTPIN 4
bool ok = true;

//string formid = "1_q2E8SroEc5OOQh54l85XWK4mJrh9U7zLE_mN11dT_I";
string formid = "15IJEaBsBONEIt4QTXtlaNEV0Q7UbLJjDFw5itk2_eoY";

static unsigned cleanupPin = UINT_MAX;
static bool verbose = false;

int read_dht11(unsigned pin) {
    gpioSetMode(pin, PI_OUTPUT);
    gpioDelay(19 * 1000);
    gpioSetMode(pin, PI_INPUT);
    return 0;
}

static void cleanup(void) {
    if (verbose) {
      fprintf(stderr,"... cleanup()\n");
    }
    if (cleanupPin != UINT_MAX) {
      gpioSetPullUpDown( cleanupPin, PI_PUD_OFF);
    }
    gpioTerminate();
}

enum pulse_state { PS_IDLE = 0, PS_PREAMBLE_STARTED, PS_DIGITS };

static void pulse_reader(int gpio, int level, uint32_t tick) {
    ofstream database; 
    database.open ("database.csv", std::ofstream::out | std::ofstream::app);
    if(ok){
      //database << "Temperatura;Umidade\n";
    }
    ok = false;
    static uint32_t lastTick = 0;
    static enum pulse_state state = PS_IDLE;
    static uint64_t accum = 0;
    static int count = 0;
    uint32_t len = tick - lastTick; // not handling rollover, you will get a bad read on that one
    lastTick = tick;

    switch (state) {
      case PS_IDLE:
        if (level == 1 && len > 70 && len < 95) state = PS_PREAMBLE_STARTED;
        else state = PS_IDLE;
        break;
      case PS_PREAMBLE_STARTED:
  if (level == 0 && len > 70 && len < 95) {
      state = PS_DIGITS;
      accum = 0;
      count= 0;
  } else state = PS_IDLE;
    break;
      case PS_DIGITS:
  if (level == 1 && len >= 35 && len <= 65);
  else if (level == 0 && len >= 15 && len <= 35) { 
    accum <<= 1;
    count++; 
  }
  else if (level == 0 && len >= 60 && len <= 80) { 
    accum = (accum << 1) + 1; 
    count++; 
  }
  else state = PS_IDLE;

  if (count == 40) {
      state = PS_IDLE;

      uint8_t parity = (accum & 0xff);
      uint8_t tempLow = ((accum>>8) & 0xff);
      uint8_t tempHigh = ((accum>>16) & 0xff);
      uint8_t humLow = ((accum>>24) & 0xff);
      uint8_t humHigh = ((accum>>32) & 0xff);

      uint8_t sum = tempLow + tempHigh + humLow + humHigh;
      bool valid = (parity == sum);
      
      if (valid) {
        printf("{\"Temperatura\": %d,%d, \"Umidade\": %d,%d}\n", tempHigh, tempLow, humHigh, humLow);
        string temp = to_string(tempHigh) + ',' + to_string(tempLow);
        string humi = to_string(humHigh) + ',' + to_string(humLow);

        //string command = "curl https://docs.google.com/forms/d/" + formid + "/formResponse -d ifq -d \"entry.1076682711= +" + temp + "\" -d \"entry.1505376468= + " + humi + "\" -d \"entry.1031950862= 430\" -d submit=Submit;";
        string command = "curl https://docs.google.com/forms/d/" + formid + "/formResponse -d ifq -d \"entry.1115749519= +" + temp + "\" -d \"entry.460396820= + " + humi + "\" -d submit=Submit;";
        system(command.c_str());
        database << temp << ';' << humi << '\n';

      }
    }
    break;
  }
  if (verbose) {
    printf("pulse %c %4u??S state=%d digits=%d\n", (level==0 ? 'H' : (level==1?'L':'W') ), len, state, count);
  }
}

int main( int argc, char **argv) {
    unsigned pin = DHTPIN;
    // database << "Temperature,Humidity\n"; 
    if (gpioInitialise() == PI_INIT_FAILED) {
      fprintf(stderr, "failed to initialize GPIO\n");
      exit(EXIT_FAILURE);
    }
    atexit(cleanup);
    gpioSetMode( pin, PI_INPUT);
    gpioSetPullUpDown( pin, PI_PUD_UP);
    gpioWrite( pin, 0); 
    cleanupPin = pin; 
    gpioSetWatchdog(pin, 50);
    while (1) {
      gpioSetAlertFunc( pin, pulse_reader);
      read_dht11(pin);
      gpioDelay(60000000);
    }
    exit(2);
    return 0;
}
