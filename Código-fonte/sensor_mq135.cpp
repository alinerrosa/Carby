#include <bits/stdc++.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

using namespace std;

template <typename T>
class MCP3008 {
 public:
  T bus, device;
  int max_speed_hz;
  MCP3008() : bus(0), device(0) {
    max_speed_hz = wiringPiSPISetup(0, 1000000);
  }
  MCP3008(T _bus, T _device) : bus(_bus), device(_device) {
    max_speed_hz = wiringPiSPISetup(0, 1000000);
  }
  ~MCP3008() {} 
  
  void open() {
  }
};

vector<double> CO2Curve {2.0, 0.004, -0.34};
class MQ {
 public:
  int MQ_PIN = 0;
  int RL_VALUE = 20;
  double RO_CLEAN_AIR_FACTOR = 3.8;

  int CALIBRATION_SAMPLE_TIMES = 50;
  int CALIBRATION_SAMPLE_INTERVAL = 500;

  int READ_SAMPLE_INTERVAL = 50;
  int READ_SAMPLE_TIMES = 5;
  int GAS_CO2 = 0;
  int Ro;
  int analogPin;
  MQ() : Ro(50), analogPin(0) {}
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
      value += MQResistanceCalculation(1);
      sleep(CALIBRATION_SAMPLE_INTERVAL / 1000.0);
    }
    value /= CALIBRATION_SAMPLE_TIMES;
    value /= RO_CLEAN_AIR_FACTOR;
    return value;
  }

  double MQRead(int mq_pin) {
    double rs = 0.0;
    for (int i = 0; i < READ_SAMPLE_TIMES; i++) {
      rs += MQResistanceCalculation(1);
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

int main() {
  ios::sync_with_stdio(false);
  cin.tie(0);
  MQ *mq = new MQ();
  // mq->MQPercentage();
  while (true) {
    map<string, double> perc = mq->MQPercentage();
    cout << "\r" << '\n';
    cout << "\033[K" << '\n';
    cout << "CO2: " << perc["GAS_CO2"] << " ppm\n";
    sleep(1.0);
  }
  return 0;
}