const { conversation } = require('@assistant/conversation');
const functions = require('firebase-functions');
const axios = require('axios');

const {
  dialogflow,
  Image,
} = require('actions-on-google');

// Create an app instance
const app = conversation();

app.handle('Start', conv => {
  conv.add('Olá, eu sou a Carby e estou aqui para monitorar a qualidade do ar do ambiente! Qual desses dados você gostaria de saber? Temperatura, Umidade, Concentração de CO2 ou Todos eles?');
});

app.handle('Finish', conv => {
  conv.add('Nada, assim que precisar estarei aqui, só chamar!!!')
});


function getSpreadSheetData() {
  return axios.get('https://sheetdb.io/api/v1/9g3wvetd2bx25');
}

app.handle('GetTemperature', conv => {
  return getSpreadSheetData().then(my_data => {
    const lastTemperature = my_data.data[my_data.data.length - 1].Temperatura;
    const lastUmidity = my_data.data[my_data.data.length - 1].Umidade;
    const lastCO2 = my_data.data[my_data.data.length - 1].CO2;
    const lastQuality = my_data.data[my_data.data.length - 1].Qualidade;
    conv.add(`A temperatura é de ${lastTemperature} graus`);
    conv.add('Você gostaria de saber mais algum dado? Sim ou não?');
  });
});

app.handle('GetUmidity', conv => {
  return getSpreadSheetData().then(my_data => {
    const lastUmidity = my_data.data[my_data.data.length - 1].Umidade;
    conv.add(`A umidade está em ${lastUmidity} %`);
    conv.add('Você gostaria de saber mais algum dado? Sim ou não?');
  });
});

app.handle('GetCO', conv => {
  return getSpreadSheetData().then(my_data => {
    const lastCO2 = my_data.data[my_data.data.length - 1].CO2;
    conv.add(`A concentração de CO2 é de ${lastCO2} PPM`);
    conv.add('Você gostaria de saber mais algum dado? Sim ou não?');
  });
});

app.handle('GetData', conv => {
  return getSpreadSheetData().then(my_data => {
    const lastTemperature = my_data.data[my_data.data.length - 1].Temperatura;
    const lastUmidity = my_data.data[my_data.data.length - 1].Umidade;
    const lastCO2 = my_data.data[my_data.data.length - 1].CO2;
    const lastQuality = my_data.data[my_data.data.length - 1].Qualidade;
    conv.add(`A temperatura é ${lastTemperature} graus celsius, a umidade é ${lastUmidity} %, a concentração de CO2 é ${lastCO2} ppm e a qualidade do ar está ${lastQuality}`);
    if (lastQuality === 'Ruim') {
      conv.add('Aviso!!!!, realize a troca de ar do ambiente.');
    }
  });
});


exports.ActionsOnGoogleFulfillment = functions.https.onRequest(app);
