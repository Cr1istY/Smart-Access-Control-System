import threading
import json
import cv2
import numpy as np
import paho.mqtt.client as mqtt
from ultralytics import YOLO

model = YOLO('esp32_face_640_best.pt')

MQTT_BROKER = "127.0.0.1"
MQTT_PORT = 1883
MQTT_TOPIC_IMAGE = "esp32/camera/image"
MQTT_TOPIC_RESULT = "esp32/face/result"

# FRAMESIZE_HQVGA
IMG_WIDTH = 240
IMG_HEIGHT = 176

latest_image = None
image_lock = threading.Lock()
is_running = True

# MQTT
def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print("Connected to MQTT Broker")
        client.subscribe(MQTT_TOPIC_IMAGE)
        print("Subscribed to topic" + MQTT_TOPIC_IMAGE)
    else:
        print("Could not connect to MQTT Broker")

def on_message(client, userdata, msg):
    global latest_image
    try:
        # print(msg.payload)
        image_data = np.frombuffer(msg.payload, dtype=np.uint8)
        image = cv2.imdecode(image_data, cv2.IMREAD_COLOR)

        if image is not None:
            with image_lock:
                latest_image = image
        else:
            print("Error while reading image")
    except Exception as e:
        print(e)

mqtt_client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

def ai_processing_loop():
    global is_running, latest_image
    print("Starting AI Processing Loop")
    while is_running:
        frame = None
        with image_lock:
            if latest_image is not None:
                frame = latest_image.copy() # 复制一份
                latest_image = None

        if frame is not None:
            results = model(frame, verbose=False)
            boxes = results[0].boxes.xyxy.cpu().numpy()
            face_locations = []

            for box in boxes:
                x1, y1, x2, y2 = box
                # face_recognition 需要 (上, 右, 下, 左) 格式
                face_locations.append((int(y1), int(x2), int(y2), int(x1)))

            if face_locations:
                top, right, bottom, left = face_locations[0]
                result = {
                    "status": "0",
                    "bbox": [left, top, right, bottom],
                }
                print(f"face detected: {result['bbox']}")
            else:
                result = {
                    "status": "1",
                    "message": "No face detected",
                }
            try:
                result_json = json.dumps(result)
                mqtt_client.publish(MQTT_TOPIC_RESULT, result_json)
                print("published result")
            except Exception as e:
                print("MQTT publish failed " + str(e))




if __name__ == "__main__":
    ai_thread = threading.Thread(target=ai_processing_loop)
    ai_thread.start()

    try:
        print("try to connect to MQTT Broker")
        mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
        mqtt_client.loop_forever()
    except KeyboardInterrupt:
        print("Closing MQTT Broker")
        mqtt_client.disconnect()
    except Exception as e:
        print("connect to MQTT Broker failed " + str(e))
    finally:
        is_running = False
        ai_thread.join()
        print("EXIT")


