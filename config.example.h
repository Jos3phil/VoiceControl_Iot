#ifndef CONFIG_H
#define CONFIG_H

// ==================== CONFIGURACIÓN WiFi ====================
// INSTRUCCIONES: Reemplaza con tus credenciales reales
const char* WIFI_SSID = "TU_WIFI_SSID";              // Reemplaza con tu SSID de WiFi
const char* WIFI_PASSWORD = "TU_WIFI_PASSWORD";      // Reemplaza con tu contraseña de WiFi

// ==================== CONFIGURACIÓN ADAFRUIT IO ====================
// INSTRUCCIONES: Obtén estas credenciales desde https://io.adafruit.com/
const char* AIO_USERNAME = "TU_USUARIO_ADAFRUIT";    // Tu nombre de usuario de Adafruit IO
const char* AIO_KEY = "TU_CLAVE_ADAFRUIT_IO";        // Tu clave AIO Key de Adafruit IO
const char* AIO_SERVER = "io.adafruit.com";
const int AIO_SERVERPORT = 1883;

// ==================== NOMBRES DE LOS FEEDS ====================
// INSTRUCCIONES: Cambia estos nombres por los de tus feeds en Adafruit IO
const char* FEED_TEMPERATURA = "temperatura-dht22";   // Feed para temperatura
const char* FEED_HUMEDAD = "humedad-dht22";          // Feed para humedad
const char* FEED_CORRIENTE = "corriente-sct013";     // Feed para corriente
const char* FEED_POTENCIA = "potencia-sct013";       // Feed para potencia
const char* FEED_LED_CONTROL = "esp32-led";          // Feed para controlar el LED

#endif