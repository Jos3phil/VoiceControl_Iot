#include <Arduino.h>
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <DHT.h>
#include "config.h" // Asegúrate de que este archivo contiene tus credenciales y nombres de feeds

// ======================= CONFIGURACIÓN DE PINES Y SENSORES =======================
const int SENSOR_CORRIENTE_PIN = 34;
const int SENSOR_DHT_PIN = 4;
const int LED_PIN = 2;

#define DHTTYPE DHT22
const float VOLTAJE_RED_NOMINAL = 220.0;
const float UMBRAL_RUIDO_A = 0.15; // Puedes ajustar esto si es necesario
const float VOLTAJE_REFERENCIA_ESP = 3.3;
const int RESOLUCION_ADC = 4095;
const int NUM_MUESTRAS = 1500;
const float VUELTAS_TRANSFORMADOR = 2000;
const float RESISTENCIA_CARGA = 33.0;

// ======================= OBJETOS Y VARIABLES GLOBALES =======================
DHT dht(SENSOR_DHT_PIN, DHTTYPE);
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Feeds MQTT
Adafruit_MQTT_Publish temperaturaFeed( &mqtt, String(String(AIO_USERNAME) + "/feeds/" + String(FEED_TEMPERATURA)).c_str() );
Adafruit_MQTT_Publish humedadFeed( &mqtt, String(String(AIO_USERNAME) + "/feeds/" + String(FEED_HUMEDAD)).c_str() );
Adafruit_MQTT_Publish corrienteFeed( &mqtt, String(String(AIO_USERNAME) + "/feeds/" + String(FEED_CORRIENTE)).c_str() );
Adafruit_MQTT_Publish potenciaFeed( &mqtt, String(String(AIO_USERNAME) + "/feeds/" + String(FEED_POTENCIA)).c_str() );
Adafruit_MQTT_Subscribe ledControlFeed( &mqtt, String(String(AIO_USERNAME) + "/feeds/" + String(FEED_LED_CONTROL)).c_str() );

double offsetICal; // ¡NUESTRA VARIABLE DE CALIBRACIÓN HA VUELTO!
unsigned long previousMillis = 0;
const long interval = 10000;

// ======================= DECLARACIÓN DE FUNCIONES (Forward Declaration) =======================
// Buena práctica para que el compilador sepa que estas funciones existen
void ledControlCallback(char *data, uint16_t len);
void conectarMQTT();
void leerYPublicarSensores();
void calibrarSensorCorriente();

// ======================= SETUP =======================
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  dht.begin();

  Serial.println("\n=== Monitor IoT Integrado ===");
  
  // Conectar a WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando a WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado! IP: " + WiFi.localIP().toString());
  
  // ¡PASO CRÍTICO! Calibrar el sensor ANTES de empezar a medir
  calibrarSensorCorriente();

  // Configurar y suscribirse al control del LED
  ledControlFeed.setCallback(ledControlCallback);
  conectarMQTT(); // Conectamos una vez para suscribirnos
  mqtt.subscribe(&ledControlFeed);
  
  Serial.println("==============================");
  Serial.println("Sistema iniciado. Esperando el primer ciclo de lectura...");
}

// ======================= LOOP PRINCIPAL (Más limpio) =======================
void loop() {
  conectarMQTT(); // Asegura que la conexión esté activa

  // Revisa constantemente si hay nuevos comandos para el LED
  mqtt.processPackets(1000); 

  // Mantenemos viva la conexión para recibir comandos al instante
  if(!mqtt.ping()) {
    mqtt.disconnect();
  }
  
  // Publica los datos de los sensores cada 30 segundos
  if (millis() - previousMillis >= interval) {
    previousMillis = millis();
    leerYPublicarSensores();
  }
}

// ======================= MANEJADOR DE COMANDOS DEL LED =======================
void ledControlCallback(char *data, uint16_t len) {
  String comando = String(data).substring(0, len);
  comando.toLowerCase();
  
  Serial.println("Comando para LED recibido: " + comando);
  
  if (comando == "on" || comando == "1" || comando == "encender") {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("-> LED encendido");
  } else if (comando == "off" || comando == "0" || comando == "apagar") {
    digitalWrite(LED_PIN, LOW);
    Serial.println("-> LED apagado");
  }
}

// ======================= FUNCIÓN PARA LEER Y PUBLICAR TODO =======================
void leerYPublicarSensores() {
    Serial.println("\n======== Leyendo y Publicando Sensores ========");
    
    // --- Leer y Publicar DHT22 ---
    float temperatura = dht.readTemperature();
    float humedad = dht.readHumidity();
    if (!isnan(temperatura) && !isnan(humedad)) {
        Serial.printf("Temperatura: %.2f °C, Humedad: %.2f %%\n", temperatura, humedad);
        temperaturaFeed.publish(temperatura);
        humedadFeed.publish(humedad);
    } else {
        Serial.println("Error al leer el sensor DHT22");
    }

    // --- Leer y Publicar SCT-013 ---
    double sumaCuadrados = 0;
    for (int i = 0; i < NUM_MUESTRAS; i++) {
      // ¡LA MAGIA ESTÁ AQUÍ! Restamos el offset REAL que medimos
      double lecturaSinOffset = (double)analogRead(SENSOR_CORRIENTE_PIN) - offsetICal;
      sumaCuadrados += lecturaSinOffset * lecturaSinOffset;
    }

    double IrmsADC = sqrt(sumaCuadrados / NUM_MUESTRAS);
    double VrmsSensor = (IrmsADC / RESOLUCION_ADC) * VOLTAJE_REFERENCIA_ESP;
    double IrmsSensor = VrmsSensor / RESISTENCIA_CARGA;
    double corrienteRMS = IrmsSensor * VUELTAS_TRANSFORMADOR;
    
    if (corrienteRMS < UMBRAL_RUIDO_A) {
      corrienteRMS = 0.0;
    }
    double potencia = corrienteRMS * VOLTAJE_RED_NOMINAL;

    Serial.printf("Corriente: %.3f A, Potencia: %.2f W\n", corrienteRMS, potencia);
    corrienteFeed.publish(corrienteRMS);
    potenciaFeed.publish(potencia);
    
    Serial.println("==========================================\n");
}

// ======================= FUNCIÓN DE CALIBRACIÓN (REINTRODUCIDA) =======================
void calibrarSensorCorriente() {
  Serial.println("\nIniciando calibración del sensor de corriente...");
  Serial.println("Asegúrate de que no pasa ningún cable por el sensor.");
  
  long sumaLecturas = 0;
  for (int i = 0; i < 2000; i++) {
    sumaLecturas += analogRead(SENSOR_CORRIENTE_PIN);
    delay(1);
  }
  
  offsetICal = (double)sumaLecturas / 2000.0;
  
  Serial.print("Calibración completada. Offset (punto cero) detectado en: ");
  Serial.println(offsetICal);
}

// ======================= FUNCIÓN PARA CONECTAR MQTT (Sin cambios) =======================
void conectarMQTT() {
  int8_t resultado;
  if (mqtt.connected()) return;
  Serial.print("Conectando a MQTT... ");
  uint8_t reintentos = 3;
  while ((resultado = mqtt.connect()) != 0) {
    Serial.println(mqtt.connectErrorString(resultado));
    Serial.println("Reintentando en 5 segundos...");
    mqtt.disconnect();
    delay(5000);
    reintentos--;
    if (reintentos == 0) ESP.restart();
  }
  Serial.println("MQTT Conectado!");
}