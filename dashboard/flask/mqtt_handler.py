# mqtt_handler.py
import paho.mqtt.publish as publish
import os
import logging
import json

logger = logging.getLogger("mqtt")

def publish_mqtt(data):
    try:
        broker = os.getenv("MQTT_BROKER")
        port = int(os.getenv("MQTT_PORT", 1883))
        topic = os.getenv("MQTT_TOPIC", "garden/sensors")
        user = os.getenv("MQTT_USER")
        password = os.getenv("MQTT_PASSWORD")


        if not broker:
            logger.error("[MQTT] env variable missing!")
            return

        auth = {'username': user, 'password': password} if user and password else None
        payload = json.dumps(data)
        logger.info(f"MQTT Broker: {broker}, Port: {port}, Topic: {topic}")
        publish.single(topic, payload=payload, hostname=broker, port=port, auth=auth, retain=True)
        logger.info(f"[MQTT] Published to {topic}: {payload}")
    except Exception as e:
        logger.error(f"[MQTT] Error publishing to MQTT: {e}")
    
