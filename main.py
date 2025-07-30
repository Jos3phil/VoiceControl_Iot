import os
import requests
from flask import Flask, request, jsonify
import logging
import sys

# Importar configuración desde archivo separado (no incluido en Git)
try:
    from config import (
        AIO_USERNAME, AIO_KEY, AIO_SERVER,
        FEED_TEMPERATURA, FEED_HUMEDAD, FEED_CORRIENTE, 
        FEED_POTENCIA, FEED_LED_CONTROL
    )
except ImportError:
    print("ERROR: No se encontró el archivo config.py")
    print("Por favor, copia config.example.py a config.py y configura tus credenciales")
    sys.exit(1)

# --- CONFIGURACIÓN DE LOGGING PARA CLOUD RUN ---
# Configuración robusta para asegurar que todos los logs se vean correctamente
logging.basicConfig(stream=sys.stdout, level=logging.INFO, format='%(levelname)s: %(message)s')

app = Flask(__name__)

# Las credenciales de Adafruit IO y nombres de feeds se importan desde config.py

# --- ENDPOINT PRINCIPAL PARA ALEXA ---
@app.route('/alexa-webhook', methods=['POST'])
def alexa_webhook():
    """
    Endpoint específico que maneja todas las peticiones de Alexa.
    """
    try:
        request_json = request.get_json()
        logging.info(f"Petición de Alexa recibida: {request_json}")
        
        if not request_json or 'request' not in request_json:
            logging.error("Petición inválida - no contiene 'request'")
            return jsonify({"error": "Invalid request format"}), 400
        
        return handle_alexa_request(request_json)
        
    except Exception as e:
        logging.error(f"Error en alexa_webhook: {e}", exc_info=True)
        return build_alexa_response("Lo siento, hubo un error interno. Intenta de nuevo más tarde.")

# --- ENDPOINT ROOT PARA HEALTH CHECK ---
@app.route('/', methods=['GET'])
def health_check():
    """Endpoint para verificar que el servicio está activo."""
    return "<h1>Servidor IoT para Alexa está funcionando</h1><p>Usa el endpoint /alexa-webhook para las peticiones de la Skill.</p>"

# --- MANEJADOR ESPECÍFICO PARA ALEXA (VERSIÓN FINAL) ---
def handle_alexa_request(alexa_json):
    request_data = alexa_json.get('request', {})
    request_type = request_data.get('type')
    logging.info(f"Tipo de Petición: {request_type}")

    # Cuando el usuario lanza la skill con "Alexa, abre [nombre invocación]"
    if request_type == 'LaunchRequest':
        return build_alexa_response(
            "Hola, bienvenido al monitor del taller. Puedes pedirme la temperatura, humedad, corriente o potencia.",
            should_end_session=False
        )

    # Cuando el usuario hace una pregunta específica
    elif request_type == 'IntentRequest':
        intent_name = request_data.get('intent', {}).get('name')
        logging.info(f"Intent detectado: {intent_name}")

        # --- MANEJO DE TODOS LOS INTENTS ---
        
        if intent_name == 'TemperaturaIntent':
            temp = get_adafruit_feed_value(FEED_TEMPERATURA)
            response_text = f"La temperatura actual es de {temp:.1f} grados." if temp is not None else "No pude obtener la lectura de temperatura."
            return build_alexa_response(response_text)

        elif intent_name == 'HumedadIntent':
            hum = get_adafruit_feed_value(FEED_HUMEDAD)
            response_text = f"La humedad relativa es del {hum:.1f} por ciento." if hum is not None else "No pude obtener la lectura de humedad."
            return build_alexa_response(response_text)
            
        elif intent_name == 'CorrienteIntent':
            corr = get_adafruit_feed_value(FEED_CORRIENTE)
            response_text = f"El consumo de corriente es de {corr:.2f} amperios." if corr is not None else "Lo siento, no pude leer la corriente eléctrica."
            return build_alexa_response(response_text)

        elif intent_name == 'PotenciaIntent':
            pot = get_adafruit_feed_value(FEED_POTENCIA)
            response_text = f"La potencia estimada es de {pot:.0f} vatios." if pot is not None else "Lo siento, no pude leer la potencia eléctrica."
            return build_alexa_response(response_text)

        elif intent_name == 'EstadoCompletoIntent':
            temp = get_adafruit_feed_value(FEED_TEMPERATURA)
            hum = get_adafruit_feed_value(FEED_HUMEDAD)
            pot = get_adafruit_feed_value(FEED_POTENCIA)
            response_text = (
                f"Reporte completo del taller: Temperatura de {temp:.1f} grados, "
                f"humedad del {hum:.1f} por ciento, y un consumo de potencia de {pot:.0f} vatios."
            ) if all(v is not None for v in [temp, hum, pot]) else "No pude obtener todos los datos. Por favor, intenta de nuevo."
            return build_alexa_response(response_text)

        elif intent_name == 'EncenderLedIntent':
            success = set_adafruit_feed_value(FEED_LED_CONTROL, "1")
            response_text = "Entendido, LED encendido." if success else "No pude encender el LED."
            return build_alexa_response(response_text)
        
        elif intent_name == 'ApagarLedIntent':
            success = set_adafruit_feed_value(FEED_LED_CONTROL, "0")
            response_text = "Claro, LED apagado." if success else "No pude apagar el LED."
            return build_alexa_response(response_text)
        
        # Intents estándar de Amazon
        elif intent_name in ['AMAZON.StopIntent', 'AMAZON.CancelIntent']:
            return build_alexa_response("Hasta luego.", should_end_session=True)
        
        elif intent_name == 'AMAZON.HelpIntent':
            return build_alexa_response(
                "Puedes pedirme la temperatura, humedad, corriente, potencia, o encender y apagar el LED. ¿Qué necesitas?",
                should_end_session=False
            )
        
        else:
            logging.warning(f"Intent no reconocido: {intent_name}")
            return build_alexa_response("Lo siento, no entendí esa solicitud. ¿Puedes intentarlo de nuevo?", should_end_session=False)

    # Sesión terminada
    elif request_type == 'SessionEndedRequest':
        logging.info("Sesión de Alexa terminada.")
        return "", 200

    else:
        logging.warning(f"Tipo de request no manejado: {request_type}")
        return build_alexa_response("Lo siento, ocurrió algo inesperado.")

# --- FUNCIÓN DE AYUDA PARA CONSTRUIR RESPUESTAS DE ALEXA ---
def build_alexa_response(speech_text, should_end_session=True):
    response = {
        "version": "1.0",
        "response": {
            "outputSpeech": {
                "type": "PlainText", # Usamos PlainText para máxima simplicidad y compatibilidad
                "text": speech_text
            },
            "shouldEndSession": should_end_session
        }
    }
    logging.info(f"Enviando respuesta a Alexa: {speech_text}")
    return jsonify(response)

# --- Funciones de comunicación con Adafruit ---
def get_adafruit_feed_value(feed_name):
    try:
        url = f"https://{AIO_SERVER}/api/v2/{AIO_USERNAME}/feeds/{feed_name}/data/last"
        headers = {"X-AIO-Key": AIO_KEY}
        response = requests.get(url, headers=headers, timeout=10)
        if response.status_code == 200:
            return float(response.json()["value"])
        logging.error(f"Error en Adafruit GET {feed_name}: {response.status_code}")
        return None
    except Exception as e:
        logging.error(f"Excepción en Adafruit GET {feed_name}: {e}", exc_info=True)
        return None

def set_adafruit_feed_value(feed_name, value):
    try:
        url = f"https://{AIO_SERVER}/api/v2/{AIO_USERNAME}/feeds/{feed_name}/data"
        headers = {"X-AIO-Key": AIO_KEY, "Content-Type": "application/json"}
        data = {"value": str(value)}
        response = requests.post(url, json=data, headers=headers, timeout=10)
        return response.status_code in [200, 201]
    except Exception as e:
        logging.error(f"Excepción en Adafruit SET {feed_name}: {e}", exc_info=True)
        return False

# --- Bloque de arranque del servidor ---
if __name__ == "__main__":
    port = int(os.environ.get("PORT", 8080))
    # debug=True no es recomendable en producción, pero es útil para desarrollo
    app.run(host='0.0.0.0', port=port)