# config.example.py - Plantilla de configuración para la aplicación Flask
# INSTRUCCIONES: 
# 1. Copia este archivo y renómbralo a config.py
# 2. Reemplaza los valores de ejemplo con tus credenciales reales
# 3. El archivo config.py no se subirá a GitHub por seguridad

# CONFIGURACIÓN DE ADAFRUIT IO
# Obtén estas credenciales desde https://io.adafruit.com/
AIO_USERNAME = "TU_USUARIO_ADAFRUIT"    # Tu nombre de usuario de Adafruit IO
AIO_KEY = "TU_CLAVE_ADAFRUIT_IO"        # Tu clave AIO Key de Adafruit IO  
AIO_SERVER = "io.adafruit.com"

# NOMBRES DE LOS FEEDS
# Cambia estos nombres por los de tus feeds en Adafruit IO
FEED_TEMPERATURA = "temperatura-dht22"   # Feed para temperatura
FEED_HUMEDAD = "humedad-dht22"          # Feed para humedad
FEED_CORRIENTE = "corriente-sct013"     # Feed para corriente
FEED_POTENCIA = "potencia-sct013"       # Feed para potencia
FEED_LED_CONTROL = "esp32-led"          # Feed para controlar el LED
