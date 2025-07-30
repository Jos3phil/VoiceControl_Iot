#include <Arduino.h>
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <DHT.h>
#include "config.h"  // Archivo con credenciales (no incluido en Git)

// ======================= CONFIGURACIÓN DE PINES =======================
const int SENSOR_CORRIENTE_PIN = 34;
const int SENSOR_DHT_PIN = 4;
const int LED_PIN = 2;

// ======================= CONFIGURACIÓN DE SENSORES =======================
#define DHTTYPE DHT22
const float VOLTAJE_RED_NOMINAL = 220.0;
const float UMBRAL_RUIDO_A = 0.15;
const float VOLTAJE_REFERENCIA_ESP = 3.3;
const int RESOLUCION_ADC = 4095;
const int NUM_MUESTRAS = 1500;
const float VUELTAS_TRANSFORMADOR = 2000;
const float RESISTENCIA_CARGA = 33.0;

// ======================= CONFIGURACIÓN DE RED Y ADAFRUIT IO =======================
// Las credenciales están definidas en config.h para mayor seguridad
// Para configurar: copia config.example.h a config.h y edita con tus credenciales

// ======================= INICIALIZACIÓN DE OBJETOS =======================
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
DHT dht(SENSOR_DHT_PIN, DHTTYPE);

// ======================= FEEDS DE ADAFRUIT IO =======================
// Feeds para PUBLICAR (enviar datos a la nube) - nombres definidos en config.h
Adafruit_MQTT_Publish temperaturaFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/" FEED_TEMPERATURA);
Adafruit_MQTT_Publish humedadFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/" FEED_HUMEDAD);
Adafruit_MQTT_Publish corrienteFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/" FEED_CORRIENTE);
Adafruit_MQTT_Publish potenciaFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/" FEED_POTENCIA);

// Feed para SUSCRIBIRSE (recibir comandos de la nube)
Adafruit_MQTT_Subscribe ledControlFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/" FEED_LED_CONTROL);

// ======================= VARIABLES GLOBALES =======================
double offsetICal;
unsigned long ultimaPublicacion = 0;
const long intervaloPublicacion = 30000; // Publicar datos cada 30 segundos

// Estructura para las mediciones de corriente
struct MedicionesCorriente {
  double corriente;
  double potenciaEstimada;
};

// ======================= FUNCIÓN DE CONEXIÓN WIFI =======================
void conectarWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando a WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado! IP: " + WiFi.localIP().toString());
}

// ======================= FUNCIÓN DE CONEXIÓN MQTT =======================
void MQTT_connect() {
  int8_t ret;
  if (mqtt.connected()) { return; }

  Serial.print("Conectando a Adafruit IO MQTT... ");
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) {
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Reintentando en 5 segundos...");
       mqtt.disconnect();
       delay(5000);
       retries--;
       if (retries == 0) {
         ESP.restart(); // Si no puede conectar, reinicia el ESP32
       }
  }
  Serial.println("MQTT Conectado!");
}

// ======================= FUNCIONES DE SENSORES =======================
void calibrarSensorCorriente() {
  Serial.println("\nIniciando calibración del sensor de corriente...");
  long sumaLecturas = 0;
  for (int i = 0; i < 2000; i++) {
    sumaLecturas += analogRead(SENSOR_CORRIENTE_PIN);
    delay(1);
  }
  offsetICal = (double)sumaLecturas / 2000.0;
  Serial.println("Calibración de corriente completa. Offset: " + String(offsetICal));
}

MedicionesCorriente leerSensorCorriente() {
  long sumaCuadrados = 0;
  for (int i = 0; i < NUM_MUESTRAS; i++) {
    double lecturaSinOffset = (double)analogRead(SENSOR_CORRIENTE_PIN) - offsetICal;
    sumaCuadrados += lecturaSinOffset * lecturaSinOffset;
  }
  double IrmsADC = sqrt((double)sumaCuadrados / NUM_MUESTRAS);
  double VrmsSensor = (IrmsADC / RESOLUCION_ADC) * VOLTAJE_REFERENCIA_ESP;
  double IrmsSensor = VrmsSensor / RESISTENCIA_CARGA;
  double I_real = IrmsSensor * VUELTAS_TRANSFORMADOR;
  if (I_real < UMBRAL_RUIDO_A) { I_real = 0.0; }
  double P_estimada = VOLTAJE_RED_NOMINAL * I_real;
  MedicionesCorriente mediciones = {I_real, P_estimada};
  return mediciones;
}

// ======================= MANEJADOR DE COMANDOS DEL LED =======================
void handleLedMessage(char *data, uint16_t len) {
  String message = String(data);
  message.trim(); // Limpia espacios en blanco
  Serial.print("Comando para LED recibido de Adafruit: '");
  Serial.print(message);
  Serial.println("'");

  if (message == "1" || message.equalsIgnoreCase("ON")) {
    Serial.println("-> Encendiendo el LED");
    digitalWrite(LED_PIN, HIGH);
  } else if (message == "0" || message.equalsIgnoreCase("OFF")) {
    Serial.println("-> Apagando el LED");
    digitalWrite(LED_PIN, LOW);
  } else {
    Serial.println("-> Comando no reconocido.");
  }
}

// ======================= SETUP =======================
void setup() {
  Serial.begin(115200);
  while (!Serial);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  dht.begin();

  conectarWiFi();
  calibrarSensorCorriente();
  
  // Configurar la función que se llamará cuando llegue un mensaje para el LED
  ledControlFeed.setCallback(handleLedMessage);

  // Conectarse a MQTT y suscribirse al feed del LED
  MQTT_connect();
  mqtt.subscribe(&ledControlFeed);
  
  Serial.println("\n=== SISTEMA INTEGRADO LISTO ===");
}

// ======================= LOOP PRINCIPAL =======================
void loop() {
  // Mantenemos la conexión con Adafruit IO activa
  MQTT_connect();

  // Tarea 1: Escuchar por nuevos comandos (muy importante)
  // processPackets revisa si ha llegado algún mensaje de los feeds suscritos
  mqtt.processPackets(1000); // Timeout de 1 segundo

  // Tarea 2: Publicar datos de sensores cada 30 segundos
  if (millis() - ultimaPublicacion >= intervaloPublicacion) {
    Serial.println("\n--- Publicando datos de sensores a Adafruit IO ---");
    
    // Leemos los sensores
    MedicionesCorriente lecturasCorriente = leerSensorCorriente();
    float temperatura = dht.readTemperature();
    float humedad = dht.readHumidity();

    // Publicamos en Adafruit IO
    if (!isnan(temperatura)) temperaturaFeed.publish(temperatura);
    if (!isnan(humedad)) humedadFeed.publish(humedad);
    corrienteFeed.publish(lecturasCorriente.corriente);
    potenciaFeed.publish(lecturasCorriente.potenciaEstimada);
    
    Serial.println("Datos enviados.");
    Serial.println("------------------------------------");
    ultimaPublicacion = millis();
  }
}