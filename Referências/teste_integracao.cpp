/******************************************************************************
Air quality

*******************************************************************************/

#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdio.h>  
#include <stdlib.h>  
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define VOLT_STEP 0.004883 // Voltagem na leitura da porta analogica 5/1023
#define RL_MQ135 10000 // Valor da resistência de carga para o sensor MQ135
#define R_STANDARD_MQ135 27338 // Valor de R0 para o sensor MQ135 com base em experiencia realizada
#define CALIBRATION_SAMPLING_TIME 100 // Valor do tempo de amostragem para a calibração em [ms]
#define NUMBER_OF_CALIBRATION_SAMPLE 1000 // Quantidade de amostras para a calibração
#define HEAT_TIME 86400 // Tempo de aquecimento em segundos


int i_sensor_analog_reading = 0;
float R0 = 0;

void loop() {
    float temperature, humidity;
    float RS = 0;
    float rs_r0_ratio = 0;
    float PPM = 0;
    //humidity = dht11.dht11_data();
    //temperature = dht.readTemperature();
    delay(2000);
    /*********MQ-135|CO2*************/
    i_sensor_analog_reading = analogRead(SENSOR_ANALOG_PORT);
    RS = RL_MQ135 * ((5 / (VOLT_STEP * i_sensor_analog_reading)) - 1);
    //R0 = 19000;
    rs_r0_ratio = Ajust_ratio((RS / R0), MQ6);
    PPM = pow(10, ((-log10(rs_r0_ratio) + 0.79349) / 0.38306));
    lcd.print(" PPM de H2");
}
    
/************************** Ajust_ratio(int r0_rs_ratio, int sensor)
*************************************
* Entrada - r0_rs_ratio;sensor
* Saída - ajusted_ratio
* Obs - Ajusta o valor da razão r0/rs de acordo com a temperatura e umidade
73
* de acordo com as tabelas fornecidas no datasheet
***************************************************************************
**/
float Ajust_ratio(int r0_rs_ratio, int sensor)
{
    float ajusted_ratio = 0; //Valor da razao ajustada pela temperatura e humidade
    float max_value_ratio = 0; //Valor de correcao da razao a 33% de umidade
    float min_value_ratio = 0; //Valor de correcao da razao a 85% de umidade
    int humidity_ar = dht.readHumidity(); //Leitura da umidade pelo sensor
    int temperature_ar = dht.readTemperature(); //Leitura da temperatura pelo sensor
    if(humidity_ar>85 || humidity_ar<33 || temperature_ar<0 ||temperature_ar>50)
    {
        return NAN;
    }
    delay(1000);
    // SENSOR MQ-135
    max_value_ratio = -0.000005 * pow(temperature_ar, 3) + 0.000653 * pow(temperature_ar, 2) - 0.028508 * temperature_ar + 1.366829; //Curva para 33% deumidade
    min_value_ratio = -0.0000046 * pow(temperature_ar, 3) + 0.000573 * pow(temperature_ar, 2) - 0.025026 * temperature_ar + 1.2384997; //Curva para 85% de umidade
    ajusted_ratio = r0_rs_ratio * (max_value_ratio + (min_value_ratio - max_value_ratio) * (humidity_ar - 33) / 52); //Interpolacao entre os valores maximos e minimos e a umidade
    return ajusted_ratio;
}

/************************** Calibration(int sensor)
*********************************
* Entrada - sensor
* Saída - R0_calibrated
* Obs - Calcula o valor de R0. Assume-se que o sensor está sendo calibrado em um
* ambiente com ar limpo
***************************************************************************
**/
float Calibration(int sensor)
{
    float R0_calibrated = 0; //Valor de R0 apos a calibração
    float sensor_resistence_clean_air = 0; //Valor da resistência do sensor ao ar limpo
    int analog_read = 0; //Valor da leitura da porta analógica ligada ao sensor
    for (int i = 0; i < NUMBER_OF_CALIBRATION_SAMPLE; i++)
    {
        analog_read = analogRead(SENSOR_ANALOG_PORT);
        sensor_resistence_clean_air += RL_MQ135 * ((5 / (0.004883 * analog_read)) - 1);
        delay(CALIBRATION_SAMPLING_TIME);
        //lcd.setCursor(0, 1); //Permite mostrar o andamento da calibração ao usuário
        //lcd.print((i * 100 / NUMBER_OF_CALIBRATION_SAMPLE)); //Permite mostrar o andamento da calibração ao usuário
        //lcd.print("%"); //Permite mostrar o andamento da calibração ao usuário
    }
    R0_calibrated = sensor_resistence_clean_air / (Ajust_ratio(3.67, sensor) * NUMBER_OF_CALIBRATION_SAMPLE);//3.67 é o padrão para ar limpo
    return R0_calibrated;
}

void mcp3008_read(uint8_t);
 
int main(void)
{
    // setup GPIO, this uses actual BCM pin numbers 
    wiringPiSetupGpio();
    wiringPiSPISetup(1, 4*1000*1000);
    delay(50);
    for (;;){
        mcp3008_read(1);
        delay(50);
    }
    
    return 0;
}

// read a channel
void mcp3008_read(uint8_t adcnum)
{ 
    unsigned int commandout = 0;
    unsigned int adcout = 0;

    commandout = adcnum & 0x3;  // only 0-7
    commandout |= 0x18;     // start bit + single-ended bit

    uint8_t spibuf[3];

    spibuf[0] = commandout;
    spibuf[1] = 0;
    spibuf[2] = 0;

    wiringPiSPIDataRW(1, spibuf, 3);    

    adcout = ((spibuf[1] << 8) | (spibuf[2])) >> 4;

    printf("%d\n", adcout);
    
} 

