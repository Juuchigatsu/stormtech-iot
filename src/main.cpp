#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <Hash.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// Definições de WIFI
const char *SSID = "ARAUJO OI FIBRA";
const char *PASSWORD = "91E3027321";
WiFiClient espClient;

// Definições MQTT
const char *SERVER = "test.mosquitto.org";
PubSubClient client(espClient);

// Cria um objeto AsyncWebServer
AsyncWebServer server(80);

// Função para se conectar ao WIFI
void ConnectWIFI(){
  delay(1000);
  Serial.println();
  Serial.print("Conectando: ");
  Serial.println(SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  while(WiFi.status()!=WL_CONNECTED){
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WIFI conectado");
  Serial.print("Endereco de IP: ");
  Serial.println(WiFi.localIP());
  delay(1000);
}

// Função chamada sempre que chega mensagem em algum tópico assinado
void Callback(char *topic, byte *payload, unsigned int lenght){
  payload[lenght] = '\0';
  String msg = String((char*)payload);
  Serial.print("Mensagem [");
  Serial.print(topic);
  Serial.print("]");
  Serial.println(msg);
}

// Função para se reconectar ao servidor MQTT
void Reconnect(){
  while(!client.connected()){
    Serial.print("Tentanto se reconectar ao servidor MQTT: ");
    String clientID = "ESP8266-";
    clientID += String(random(0xffff), HEX);
    if(client.connect(clientID.c_str())){
      Serial.println("Conectado");
      client.subscribe("stormtech-iot/msg");
      client.publish("stormtech-iot/msg", "ON");
    }
    else{
      client.publish("stormtech-iot/msg", "OFF");
      Serial.print("Falha, codigo: ");
      Serial.print(client.state());
      Serial.println("Tentando novamente em 10 segundos");
      delay(10000);
    }
  }
}

// Constantes de tempo
unsigned long previousMillis = 0;
const long tempo = 30000;

// Variáveis globais para envio MQTT
char temp[10];
char umid[10];
char ind[10];

// Variáveis globais para armazenar os dados do sensor
float temperatura;
float umidade;
float indice;

// Definições DHT11
#define DHTTYPE DHT11
#define DHTPIN 5
DHT dht(DHTPIN, DHTTYPE);

// Função HTML
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>

<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css"
    integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
      font-family: Arial;
      display: inline-block;
      margin: 0px auto;
      text-align: center;
    }

    h2 {
      font-size: 3.0rem;
    }

    p {
      font-size: 3.0rem;
    }

    .units {
      font-size: 1.2rem;
    }

    .dht-labels {
      font-size: 1.5rem;
      vertical-align: middle;
      padding-bottom: 15px;
    }

    #wrapper {
      background-color: #084387;
      box-shadow: 1px 5px 25px 3px #444;
      border-radius: 10px;
      margin: 100px auto;
      max-width: 720px;
      padding: 1px;
      color: white;
    }
  </style>
</head>

<body>
  <div class="container" id="wrapper">
    <h2>PREVISAO DE HOJE</h2>
    <div style="display: grid;">
      <p>
        <span style="margin-right: 50px;">
          <i class="fas fa-thermometer-half" style="color:#f08802;"></i>
          <span class="dht-labels">Temperatura</span>
        </span>
        <span>
          <span id="temperatura">%TEMPERATURA%</span>
          <sup class="units">&deg;C</sup>
        </span>
      </p>
      <p>
        <span style="margin-right: 92px;">
          <i class="fas fa-tint" style="color:#00add6;"></i>
          <span class="dht-labels">Umidade</span>
        </span>
        <span>
          <span id="umidade">%UMIDADE%</span>
          <sup class="units">%</sup>
        </span>
      </p>
      <p>
        <span style="margin-right: 36px;">
          <i class="fas fa-thermometer-half" style="color:#ff5e01;"></i>
          <span class="dht-labels">Indice de calor</span>
        </span>
        <span>
          <span id="indice">0</span>
          <sup class="units">&deg;C</sup>
        </span>
      </p>
    </div>
    <div style="text-align: end;padding-right: 10px;"><span>STORMTECH</span></div>
  </div>
</body>
<script>

  setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("temperatura").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/temperatura", true);
    xhttp.send();
  }, 10000);

  setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("umidade").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/umidade", true);
    xhttp.send();
  }, 10000);

  setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("indice").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/indice", true);
    xhttp.send();
  }, 10000);
</script>

</html>)rawliteral";

String processor(const String& var){
  if(var == "TEMPERATURA"){
    return String(temperatura);
  }

  else if(var == "UMIDADE"){
    return String(umidade);
  }

  else if(var == "INDICE"){
    return String(indice);
  }

  return String();
}

void setup(){
  // Inicializa comunicação serial
  Serial.begin(115200);

  // Inicializa o server
  server.begin();

  // Inicializa o sensor DHT11
  dht.begin();

  // Chama a função ConnectWIFI()
  ConnectWIFI();

  // Configura o servidor MQTT
  client.setServer(SERVER, 1883);

  // Configura a função callback
  client.setCallback(Callback);

  // Configura o servidor
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/temperatura", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(temperatura).c_str());
  });

  server.on("/umidade", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(umidade).c_str());
  });

  server.on("/indice", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(indice).c_str());
  });
}

void loop(){
  // Verifica a conexão
  if(!client.connected()){
    Reconnect();
  }

  client.loop();

  // Publica periodicamente as informações no broker mosquitto
  dtostrf(temperatura, 6, 2, temp);
  client.publish("stormtech-iot/temperatura", temp);
  dtostrf(umidade, 6, 2, umid);
  client.publish("stormtech-iot/umidade", umid);
  dtostrf(indice, 6, 2, ind);
  client.publish("stormtech-iot/indice", ind);

  // Função para ler o sensor DHT11
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= tempo){
    previousMillis = currentMillis;

    float newTemperatura = dht.readTemperature();
    if(isnan(newTemperatura)){
      Serial.println("Falha ao ler o sensor DHT11");
    }
    else{
      temperatura = newTemperatura;
      Serial.println(temperatura);
    }

    float newUmidade = dht.readHumidity();
    if(isnan(newUmidade)){
      Serial.println("Falha ao ler o sensor DHT11");
    }
    else{
      umidade = newUmidade;
      Serial.println(umidade);
    }

    float newIndice = dht.computeHeatIndex(newTemperatura, newUmidade, false);
    if(isnan(newIndice)){
      Serial.println("Falha ao ler o sensor DHT11");
    }
    else{
      indice = newIndice;
      Serial.println(indice);
    }
  }
}