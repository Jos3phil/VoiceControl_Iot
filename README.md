# 🏠 VoiceControl IoT - Sistema de Control por Voz para Hogar Inteligente

Sistema completo de IoT que integra ESP32, sensores ambientales, y control por voz usando Alexa y Adafruit IO.

## 🌟 Características

- **📊 Monitoreo Ambiental**: Temperatura y humedad con sensor DHT22
- **⚡ Medición Eléctrica**: Monitoreo de corriente y potencia con sensor SCT-013
- **💡 Control Remoto**: Control de LED a través de comandos de voz
- **🔊 Integración con Alexa**: Control por voz usando Amazon Alexa
- **☁️ Conectividad IoT**: Sincronización con Adafruit IO
- **🌐 API REST**: Servidor Flask para integración con servicios externos

## 🏗️ Arquitectura del Sistema

```
┌─────────────────┐    WiFi     ┌─────────────────┐    MQTT    ┌─────────────────┐
│     ESP32       │ ◄────────► │   Adafruit IO   │ ◄────────► │   Flask API     │
│                 │             │                 │            │                 │
│ • DHT22         │             │ • Feed Temp     │            │ • Alexa Webhook │
│ • SCT-013       │             │ • Feed Humedad  │            │ • REST Endpoints│
│ • LED Control   │             │ • Feed Corriente│            │ • Voice Control │
└─────────────────┘             │ • Feed Potencia │            └─────────────────┘
                                │ • Feed LED      │                     ▲
                                └─────────────────┘                     │
                                                                  ┌─────────────────┐
                                                                  │   Amazon Alexa  │
                                                                  │                 │
                                                                  │ "¿Cuál es la    │
                                                                  │  temperatura?"  │
                                                                  └─────────────────┘
```

## 🚀 Configuración Rápida

### 1. Configuración del Hardware ESP32

1. **Clona el repositorio**:
   ```bash
   git clone https://github.com/Jos3phil/VoiceControl_Iot.git
   cd VoiceControl_Iot
   ```

2. **Configura las credenciales**:
   ```bash
   # Para Arduino
   cp config.example.h config.h
   
   # Para Python
   cp config.example.py config.py
   ```

3. **Edita `config.h` con tus credenciales**:
   ```cpp
   const char* WIFI_SSID = "TU_WIFI";
   const char* WIFI_PASSWORD = "TU_PASSWORD";
   const char* AIO_USERNAME = "TU_USUARIO_ADAFRUIT";
   const char* AIO_KEY = "TU_CLAVE_ADAFRUIT";
   ```

4. **Sube el código al ESP32** usando Arduino IDE

### 2. Configuración del Servidor Python

1. **Instala las dependencias**:
   ```bash
   pip install -r requirements.txt
   ```

2. **Configura `config.py`** con las mismas credenciales

3. **Ejecuta el servidor**:
   ```bash
   python main.py
   ```

## 📱 Configuración de Adafruit IO

1. **Crea una cuenta** en [Adafruit IO](https://io.adafruit.com/)

2. **Crea los siguientes feeds**:
   - `temperatura-dht22` - Para datos de temperatura
   - `humedad-dht22` - Para datos de humedad  
   - `corriente-sct013` - Para datos de corriente
   - `potencia-sct013` - Para datos de potencia
   - `esp32-led` - Para control del LED

3. **Obtén tu AIO Key** desde tu perfil de usuario

4. **Crea un dashboard** para visualizar los datos

## 🔊 Configuración de Alexa (Opcional)

1. **Despliega el servidor Flask** en un servicio cloud (Google Cloud Run, Heroku, etc.)

2. **Crea una Alexa Skill** en [Amazon Developer Console](https://developer.amazon.com/alexa)

3. **Configura el endpoint** apuntando a tu servidor: `https://tu-servidor.com/alexa-webhook`

4. **Define los intents**:
   - `TemperaturaIntent` - "¿Cuál es la temperatura?"
   - `HumedadIntent` - "¿Cuál es la humedad?"
   - `CorrienteIntent` - "¿Cuánta corriente se está consumiendo?"
   - `PotenciaIntent` - "¿Cuál es la potencia?"
   - `EncenderLedIntent` - "Enciende el LED"
   - `ApagarLedIntent` - "Apaga el LED"

## 🛠️ Hardware Requerido

| Componente | Descripción | Cantidad |
|------------|-------------|----------|
| ESP32 | Microcontrolador principal | 1 |
| DHT22 | Sensor de temperatura y humedad | 1 |
| SCT-013 | Sensor de corriente no invasivo | 1 |
| LED | Para control visual | 1 |
| Resistencias | 10kΩ, 220Ω | 2 |
| Breadboard | Para conexiones | 1 |
| Cables Jumper | Para conexiones | Varios |

## 🔌 Diagrama de Conexiones

```
ESP32 Pinout:
┌─────────────────┐
│     ESP32       │
│                 │
│ GPIO 4  ──────  │ DHT22 (Data)
│ GPIO 34 ──────  │ SCT-013 (Analog)
│ GPIO 2  ──────  │ LED (+ Resistencia 220Ω)
│ 3.3V    ──────  │ DHT22 (VCC)
│ GND     ──────  │ Común (DHT22, LED, SCT-013)
└─────────────────┘
```

## 📁 Estructura del Proyecto

```
VoiceControl_Iot/
├── sketch_jul23a.ino          # Código principal ESP32
├── config.h                   # Credenciales (NO en Git)
├── config.example.h           # Plantilla de configuración
├── main.py                    # Servidor Flask para Alexa
├── config.py                  # Configuración Python (NO en Git)  
├── config.example.py          # Plantilla configuración Python
├── requirements.txt           # Dependencias de Python
├── .gitignore                 # Archivos excluidos de Git
├── README.md                  # Este archivo
└── Dockerfile                 # Para despliegue en contenedor
```

## 🐳 Despliegue con Docker

```bash
# Construir la imagen
docker build -t voice-control-iot .

# Ejecutar el contenedor
docker run -p 8080:8080 voice-control-iot
```

## 🔒 Seguridad

⚠️ **IMPORTANTE**: Los archivos `config.h` y `config.py` contienen credenciales sensibles y están excluidos del repositorio por seguridad.

- ✅ Las credenciales están separadas del código
- ✅ `.gitignore` protege archivos sensibles
- ✅ Se incluyen plantillas de ejemplo para nuevos usuarios
- ✅ El código es seguro para repositorios públicos

## 🛠️ Comandos de Voz Disponibles

| Comando | Respuesta |
|---------|-----------|
| "¿Cuál es la temperatura?" | Retorna la temperatura actual |
| "¿Cuál es la humedad?" | Retorna la humedad relativa |
| "¿Cuánta corriente hay?" | Retorna el consumo de corriente |
| "¿Cuál es la potencia?" | Retorna la potencia estimada |
| "Enciende el LED" | Enciende el LED del ESP32 |
| "Apaga el LED" | Apaga el LED del ESP32 |

## 🚨 Solución de Problemas

### ESP32 no se conecta a WiFi
- Verifica las credenciales en `config.h`
- Asegúrate de que el ESP32 esté en rango de la red
- Reinicia el ESP32

### Error "config.py not found"
- Copia `config.example.py` a `config.py`
- Configura las credenciales correctas

### Alexa no responde
- Verifica que el servidor esté corriendo
- Revisa los logs del servidor Flask
- Confirma la configuración del endpoint en la Alexa Skill

## 🤝 Contribuciones

1. Fork el proyecto
2. Crea una rama para tu feature (`git checkout -b feature/AmazingFeature`)
3. Commit tus cambios (`git commit -m 'Add some AmazingFeature'`)
4. Push a la rama (`git push origin feature/AmazingFeature`)
5. Abre un Pull Request

## 📄 Licencia

Este proyecto está bajo la Licencia MIT - ver el archivo [LICENSE](LICENSE) para detalles.

## 👨‍💻 Autor

**Josep** - [Jos3phil](https://github.com/Jos3phil)

## 🙏 Agradecimientos

- [Adafruit](https://adafruit.com/) por las librerías de IoT
- [Amazon Alexa](https://developer.amazon.com/alexa) por la plataforma de voz
- Comunidad Arduino y ESP32