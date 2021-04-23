#include <errno.h>
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
string formid = "19G0yK3KeaRI4AkYW-SpCNy2DJooWY-eOLfsvJWB4frQ";

#define MAX_ADC_CH 8
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
  MQ() : Ro(67), analogPin(0) {}
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
                //cout << "Val == " << val << '\n';
                map<string, double> perc = mq->MQPercentage();
                cout << "CO2: " << perc["GAS_CO2"] << " ppm\n";
                string CO2 = to_string(perc["GAS_CO2"]);
                std::replace( CO2.begin(), CO2.end(), '.', ','); 
                string command = "curl https://docs.google.com/forms/d/" + formid + "/formResponse -d ifq -d \"entry.916980903= +" + CO2 + "\" -d submit=Submit;";
                system(command.c_str());
                sleep(60.0);
                data[count + i] = val;
            }
            count += ch_len * vBlocks;
            if (microDelay > 0) {
                usleep(microDelay);
            }
        }
    loop_done:
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
