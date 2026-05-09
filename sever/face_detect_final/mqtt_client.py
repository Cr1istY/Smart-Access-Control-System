import logging
import json
import paho.mqtt.client as mqtt

from config import MQTT_CONFIG
from face_engine import get_face_engine_instance

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger("MqttClient")

class MqttHandler:
    def __init__(self):
        self.client = None
        self.engine = get_face_engine_instance()

    def start(self):
        self.client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        logger.info("connecting to broker...")
        self.client.connect(MQTT_CONFIG['broker'], MQTT_CONFIG['port'], MQTT_CONFIG['keepalive'])
        self.client.loop_forever()
        logger.info("connect to broker")

    @staticmethod
    def on_connect(client, userdata, flags, reason_code, properties):
        if reason_code == 0:
            client.subscribe(MQTT_CONFIG['device_topic'])
            logger.info("Subscribed to topic" + MQTT_CONFIG['device_topic'])

        else:
            logger.error("Failed to connect to MQTT Broker")

    def on_message(self, client, userdata, msg):
        try:
            topic_parts = msg.topic.split('/')
            if len(topic_parts) == 4:
                current_device_id = topic_parts[3]
                print(f"收到来自设备 [{current_device_id}] 的图像")
            else:
                print(f"收到未知格式的主题: {msg.topic}")
                return  # 如果主题格式不对，直接返回
            image_bytes = msg.payload
            result = self.engine.detect(current_device_id, image_bytes)
        except Exception as e:
            logger.error(f"on_message error {e}")
            return

        try:
            result_json = json.dumps(result)
            result_topic = f"{MQTT_CONFIG['result_topic']}{current_device_id}"
            self.client.publish(result_topic, result_json)
            logger.info(result)
        except Exception as e:
            logger.error(f"MQTT publish failed with {e}")
            return

if __name__ == "__main__":
    handler = MqttHandler()
    handler.start()