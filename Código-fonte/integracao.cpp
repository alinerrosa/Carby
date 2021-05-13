#include <errno.h>
#include <pigpio.h>
#include <fstream>
#include <limits.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/spi/spidev.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <map>
#include <math.h>
#include <algorithm>
#include <iostream>
#include <iomanip>

using namespace std;
#define MAX_ADC_CH 8

#define DHTPIN 4
#define PINO_Vm 23
#define PINO_A 18
#define PINO_Vd 17

string x, y;

// string formid_dht11 = "15IJEaBsBONEIt4QTXtlaNEV0Q7UbLJjDFw5itk2_eoY";
string formid = "1_q2E8SroEc5OOQh54l85XWK4mJrh9U7zLE_mN11dT_I";
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
    static uint32_t lastTick = 0;
    static enum pulse_state state = PS_IDLE;
    static uint64_t accum = 0;
    static int count = 0;
    uint32_t len = tick - lastTick;
    lastTick = tick;

    switch (state) {
      case PS_IDLE:
        if (level == 1 && len > 70 && len < 95) {
          state = PS_PREAMBLE_STARTED;
        }
        else {
          state = PS_IDLE;
        }
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
  else {
    state = PS_IDLE;
  }

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
        x = temp;
        y = humi;
        // string command = "curl https://docs.google.com/forms/d/" + formid_dht11 + "/formResponse -d ifq -d \"entry.1115749519= +" + temp + "\" -d \"entry.460396820= + " + humi + "\" -d submit=Submit;";
        // system(command.c_str());
        database << temp << ';' << humi << '\n';

      }
    }
    break;
  }
  if (verbose) {
    printf("pulse %c %4uµS state = %d digits = %d\n", (level == 0 ? 'H' : (level == 1 ? 'L' : 'W')), len, state, count);
  }
}

int selectedChannels[MAX_ADC_CH];
int channels[MAX_ADC_CH];
char spidev_path[] = "/dev/spidev0.0";
const int blocksDefault = 1;
const int channelDefault = 0;
const int samplesDefault = 100;
const int freqDefault = 0;
const int clockRateDefault = 3600000;
const int coldSamples = 1000;

vector<double> CO2Curve {2.0, 0.004, -0.34};
class MQ {
 public:
  int MQ_PIN = 0;
  int RL_VALUE = 20;
  double RO_CLEAN_AIR_FACTOR = 3.8;
  int val;

  int CALIBRATION_SAMPLE_TIMES = 50;
  int CALIBRATION_SAMPLE_INTERVAL = 500;

  int READ_SAMPLE_INTERVAL = 50;
  int READ_SAMPLE_TIMES = 5;
  int GAS_CO2 = 0;
  int Ro;
  int analogPin;
  MQ() : Ro(185), analogPin(0) {}
  MQ(int _ro, int _analogPin) {
    Ro = _ro;
    MQ_PIN = _analogPin;
    cout << "Calibrating..." << '\n';
    Ro = MQCalibration(MQ_PIN);
    cout << "Calibration is done..." << '\n';
    cout.precision(3);
    cout << "Ro= " << Ro << '\n';
  }
  double MQResistanceCalculation(int raw_adc) {
    return double(RL_VALUE * (1023.0 - raw_adc) / double(raw_adc));
  }

  double MQCalibration(int mq_pin) {
    double value = 0.0;
    for (int i = 0; i < CALIBRATION_SAMPLE_TIMES; i++) {
      value += MQResistanceCalculation(val);
      sleep(CALIBRATION_SAMPLE_INTERVAL / 1000.0);
    }
    value /= CALIBRATION_SAMPLE_TIMES;
    value /= RO_CLEAN_AIR_FACTOR;
    return value;
  }

  double MQRead(int mq_pin) {
    double rs = 0.0;
    for (int i = 0; i < READ_SAMPLE_TIMES; i++) {
      rs += MQResistanceCalculation(val);
      sleep(READ_SAMPLE_INTERVAL / 1000.0);
    }
    rs /= READ_SAMPLE_TIMES;
    return rs;
  }

  double MQGetGasPercentage(double rs_ro_ratio, int gas_id) {
    if (gas_id == GAS_CO2) {
      return MQGetPercentage(rs_ro_ratio, CO2Curve);
    }
    return 0;
  }

  double MQGetPercentage(double rs_ro_ratio, const vector<double>& pcurve) {
    return (pow(10, (((log(rs_ro_ratio) - pcurve[1]) / pcurve[2]) + pcurve[0])));
  }

  map<string, double> MQPercentage() {
    map<string, double> val;
    double read = MQRead(MQ_PIN);
    val["GAS_CO2"] = MQGetGasPercentage(read / Ro, GAS_CO2);
    return val;
  }
};

int main(int argc, char *argv[]) {
    unsigned pin = DHTPIN;
    if (gpioInitialise() == PI_INIT_FAILED) {
      fprintf(stderr, "failed to initialize GPIO\n");
      exit(EXIT_FAILURE);
    }
    atexit(cleanup);
    gpioSetMode(pin, PI_INPUT);
    gpioSetPullUpDown(pin, PI_PUD_UP);
    gpioWrite(pin, 0); 
    cleanupPin = pin; 
    gpioSetWatchdog(pin, 50);
    MQ *mq = new MQ();
    while (true) {
        int i, j;
        int ch_len = 0;
        int vSamples = samplesDefault;
        double vFreq = freqDefault;
        int vClockRate = clockRateDefault;
        int vBlocks = blocksDefault;

        if (ch_len == 0) {
            ch_len = 1;
            channels[0] = channelDefault;
        }
        int microDelay = 0;
        if (vFreq != 0) {
            microDelay = 1000000 / vFreq;
        }
        int count = 0;
        int fd = 0;
        int val;
        struct timeval start;
        int *data;
        data = (int*)malloc(ch_len * vSamples * sizeof(int));
        struct spi_ioc_transfer *tr = 0;
        unsigned char *tx = 0;
        unsigned char *rx = 0;
        tr = (struct spi_ioc_transfer *)malloc(ch_len * vBlocks * sizeof(struct spi_ioc_transfer));
        if (!tr) {
            perror("malloc");
            goto loop_done;
        }
        tx = (unsigned char *)malloc(ch_len * vBlocks * 4);
        if (!tx) {
            perror("malloc");
            goto loop_done;
        }
        rx = (unsigned char *)malloc(ch_len * vBlocks * 4);
        if (!rx) {
            perror("malloc");
            goto loop_done;
        }
        memset(tr, 0, ch_len * vBlocks * sizeof(struct spi_ioc_transfer));
        memset(tx, 0, ch_len * vBlocks);
        memset(rx, 0, ch_len * vBlocks);
        for (i = 0; i < vBlocks; i++) {
            for (j = 0; j < ch_len; j++) {
                tx[(i * ch_len + j) * 4] = 0x60 | (channels[j] << 2);
                tr[i * ch_len + j].tx_buf = (unsigned long)&tx[(i * ch_len + j) * 4];
                tr[i * ch_len + j].rx_buf = (unsigned long)&rx[(i * ch_len + j) * 4];
                tr[i * ch_len + j].len = 3;
                tr[i * ch_len + j].speed_hz = vClockRate;
                tr[i * ch_len + j].cs_change = 1;
            }
        }
        tr[ch_len * vBlocks - 1].cs_change = 0;
        fd = open(spidev_path, O_RDWR);
        if (fd < 0) {
            perror("open()");
            printf("%s\n", spidev_path);
            goto loop_done;
        }
        while (count < coldSamples) {
            if (ioctl(fd, SPI_IOC_MESSAGE(ch_len * vBlocks), tr) < 0) {
                perror("ioctl");
                goto loop_done;
            }
            count += ch_len * vBlocks;
        }
        count = 0;
        if (gettimeofday(&start, NULL) < 0) {
            perror("gettimeofday: start");
            return 1;
        }
        while (count < ch_len * vSamples) {
            if (ioctl(fd, SPI_IOC_MESSAGE(ch_len * vBlocks), tr) < 0) {
                perror("ioctl");
                goto loop_done;
            }
            for (i = 0, j = 0; i < ch_len * vBlocks; i++, j += 4) {
                val = (rx[j + 1] << 2) + (rx[j + 2] >> 6);
                mq->val = val;
                data[count + i] = val;
            }
            count += ch_len * vBlocks;
            if (microDelay > 0) {
                usleep(microDelay);
            }
        }
    loop_done:
      map<string, double> perc = mq->MQPercentage();
      cout << "CO2: " << perc["GAS_CO2"] << " ppm\n";
      string quality = "";
      if (perc["GAS_CO2"] < 600.0) {
        cout << "A qualidade do ar está boa" << '\n';
        gpioSetMode(PINO_Vm, 0);
        gpioSetMode(PINO_Vd, PI_ALT0);
        gpioSetMode(PINO_A, 0);
        quality = "Boa";
      } else if (perc["GAS_CO2"] >= 600.0 && perc["GAS_CO2"] <= 1000.0) {
        cout << "A qualidade do ar está média" << '\n';
        gpioSetMode(PINO_Vm, 0);
        gpioSetMode(PINO_Vd, 0);
        gpioSetMode(PINO_A, PI_ALT0);
        quality = "Media";
      } else if (perc["GAS_CO2"] > 1000.0) {
        cout << "A qualidade do ar está ruim" << '\n';
        gpioSetMode(PINO_Vm, PI_ALT0);
        gpioSetMode(PINO_Vd, 0);
        gpioSetMode(PINO_A, 0);
        quality = "Ruim";
      }
      string CO2 = to_string(perc["GAS_CO2"]);
      replace(CO2.begin(), CO2.end(), '.', ','); 
      string CO2_dininutivo = "";
      for(char c:CO2){
        if(c == ','){
          break;
        }
        CO2_dininutivo += c;
      }
      gpioSetAlertFunc(pin, pulse_reader);
      read_dht11(pin);
      if (x != "" && y != "") {
        string command = "curl https://docs.google.com/forms/d/" + formid + "/formResponse -d ifq -d \"entry.1076682711= +" + x + "\" -d \"entry.1505376468= + " + y + "\" -d \"entry.1031950862= + " + CO2_dininutivo + "\" -d \"entry.41984470= + " + quality + "\" -d submit=Submit;";
        system(command.c_str());
        sleep(60.0);
      }
      if (fd)
        close(fd);
      if (rx)
        free(rx);
      if (tx)
        free(tx);
      if (tr)
        free(tr);
    }
    return 0;
}
