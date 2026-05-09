from insightface.app import FaceAnalysis
from ultralytics import YOLO
import threading
import json
import cv2
import numpy as np
import paho.mqtt.client as mqtt

providers = [
    ('CUDAExecutionProvider', {
        'device_id': '0',
        'cudnn_conv_algo_search': 'HEURISTIC',  # 改为 HEURISTIC 以加快启动速度
        'gpu_mem_limit': 4 * 1024 * 1024 * 1024, # 可选：限制显存使用，防止占满
    }),
    'CPUExecutionProvider'
]

# 3. 初始化 InsightFace (在 YOLO 之前！)
print(">>> 正在初始化 InsightFace...")
rec_app = FaceAnalysis(name='antelopev2', root='./models', providers=providers)
rec_app.prepare(ctx_id=0, det_size=(640, 640))
print(">>> InsightFace 初始化完成！")

print(">>> 正在初始化 YOLO...")
model = YOLO('models/esp32_face_640_best.pt')
print(">>> YOLO 初始化完成！")

MQTT_BROKER = "127.0.0.1"
MQTT_PORT = 1883
MQTT_TOPIC_IMAGE = "esp32/camera/image/+"
MQTT_TOPIC_RESULT = "esp32/face/result/"

# FRAMESIZE_HQVGA
IMG_WIDTH = 240
IMG_HEIGHT = 176

latest_image = None
image_lock = threading.Lock()
current_device_id = None
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
    global latest_image, current_device_id
    try:
        topic_parts = msg.topic.split('/')
        if len(topic_parts) == 4:
            current_device_id = topic_parts[3]
            print(f"收到来自设备 [{current_device_id}] 的图像")
        else:
            print(f"收到未知格式的主题: {msg.topic}")
            return # 如果主题格式不对，直接返回
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
    global is_running, latest_image, current_device_id
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

            if len(boxes) > 0:
                box = boxes[0] # 取最大的人脸

                x1, y1, x2, y2 = box

                x1, y1 = max(0, x1), max(0, y1)
                x2, y2 = min(frame.shape[1], x2), min(frame.shape[0], y2)

                x1, y1, x2, y2 = int(x1), int(y1), int(x2), int(y2)

                # face_recognition 需要 (上, 右, 下, 左) 格式
                face_locations.append((int(y1), int(x2), int(y2), int(x1)))

                h, w = frame.shape[:2]
                cx, cy = (x1 + x2) // 2, (y1 + y2) // 2
                bw, bh = x2 - x1, y2 - y1
                scale = 2
                new_w, new_h = int(bw * scale), int(bh * scale)

                x1_pad = max(0, cx - new_w // 2)
                y1_pad = max(0, cy - new_h // 2)
                x2_pad = min(w, cx + new_w // 2)
                y2_pad = min(h, cy + new_h // 2)
                face_roi = frame[y1_pad:y2_pad, x1_pad:x2_pad]
                print(f"裁剪图片尺寸: {face_roi.shape}")
                face_roi_resized = cv2.resize(face_roi, (640, 640), interpolation=cv2.INTER_CUBIC)
                faces = rec_app.get(face_roi_resized)
                if faces:
                    embedding = faces[0].normed_embedding
                    print(f"提取到特征值: {embedding[:5]}... (维度: {embedding.shape})")

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
                result_topic = f"{MQTT_TOPIC_RESULT}{current_device_id}"
                mqtt_client.publish(result_topic, result_json)
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


