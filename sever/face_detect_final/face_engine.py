import logging
import os
import requests
import cv2
import numpy as np
import faiss
import threading

from insightface.app import FaceAnalysis

from config import DB_CONFIG, PROVIDER_CONFIG, FACE_CONFIG
from db_handler import FaceDBHandler
from ultralytics import YOLO


logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger("FaceEngine")

class FaceRecognitionEngine:
    def __init__(self):
        self.index = None
        self.user_ids = []
        self.db_handler = FaceDBHandler(DB_CONFIG)

        providers = PROVIDER_CONFIG
        logger.info("loading InsightFace model")
        self.face_app = FaceAnalysis(name='antelopev2', root='./models', providers=providers)
        self.face_app.prepare(ctx_id=0, det_size=(640, 640))
        logger.info("InsightFace model already loaded")
        logger.info("loading esp32-640 model")
        self.yolo_model = YOLO(FACE_CONFIG['yolo_model_path'])
        logger.info("esp32-640 model already loaded")
        self._load_or_build_index()
        self.frame = None

    def _load_or_build_index(self):
        index_path = FACE_CONFIG['index_path']
        id_map_path = FACE_CONFIG['id_map_path']

        # 如果路径均存在，则加载，否则，则生成
        if os.path.exists(index_path) and os.path.exists(id_map_path):
            try:
                logger.info("find index in local, loading index...")
                self.index = faiss.read_index(index_path)
                self.user_ids = np.load(id_map_path).tolist()
                logger.info("index loaded")
                return
            except Exception as e:
                logger.error(f"find index in local failed, {e}, try to build index from database...")

        logger.warning(f"build index from database...")
        if self.db_handler.connect():
            user_ids, embeddings = self.db_handler.get_all_face_features()
            self.db_handler.close()

            if len(embeddings) > 0:
                embeddings = np.array(embeddings, dtype='float32')
                self._build_new_index(user_ids, embeddings)
                # 先保存，再重新加载
                self._load_or_build_index()
            else:
                logger.error("no data in database, cannot build index")

        else:
            logger.error("cannot connect to database, build index failed")

    @staticmethod
    def _build_new_index(user_ids, embeddings):
        logger.info("build new index...")
        # 使用 L2 距离
        index = faiss.IndexFlatL2(embeddings.shape[1])
        index.add(embeddings)

        faiss.write_index(index, FACE_CONFIG['index_path'])
        np.save(FACE_CONFIG['id_map_path'], np.array(user_ids))

        logger.info("index built")

    def detect(self, esp32_id, image_bytes):
        np_arr = np.frombuffer(image_bytes, dtype=np.uint8)
        img = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)

        if img is None:
            logger.warning("cannot read image")
            return {"status": -1, "message": "cannot read image"}

        # 先使用 yolo 画框，再进行识别
        results = self.yolo_model(img, verbose=False)
        boxes = results[0].boxes.xyxy.cpu().numpy()
        face_locations = []
        if len(boxes) > 0:
            x1, y1, x2, y2 = boxes[0]
            x1, y1 = max(0, x1), max(0, y1)
            x2, y2 = min(img.shape[1], x2), min(img.shape[0], y2)
            x1, y1, x2, y2 = int(x1), int(y1), int(x2), int(y2)
            # face_recognition 需要 (上, 右, 下, 左) 格式
            face_locations.append((int(y1), int(x2), int(y2), int(x1)))

        if face_locations:
            top, right, bottom, left = face_locations[0]
            self.frame = img.copy()
            t = threading.Thread(target=self.recognize, args=(face_locations[0], esp32_id))
            t.start()
            return {
                "status": "0",
                "bbox": [left, top, right, bottom],
            }

        else:
            return {"status": "1", "message": "no face detected"}

    def recognize(self, big_face_location, esp32_id):
        h, w = self.frame.shape[:2]
        y1, x2, y2, x1 = big_face_location
        cx, cy = (x1 + x2) // 2, (y1 + y2) // 2
        bw, bh = x2 - x1, y2 - y1
        scale = 2
        new_w, new_h = int(bw * scale), int(bh * scale)

        x1_pad = max(0, cx - new_w // 2)
        y1_pad = max(0, cy - new_h // 2)
        x2_pad = min(w, cx + new_w // 2)
        y2_pad = min(h, cy + new_h // 2)
        face_roi = self.frame[y1_pad:y2_pad, x1_pad:x2_pad]
        face_roi_resized = cv2.resize(face_roi, (640, 640), interpolation=cv2.INTER_CUBIC)
        face = self.face_app.get(face_roi_resized)

        if not face:
            return

        face = face[0]

        embedding = face.normed_embedding
        logger.info(f"{esp32_id} 提取到特征值: {embedding[:5]}... (维度: {embedding.shape})")
        query_vector = np.expand_dims(embedding, axis=0).astype(np.float32)
        # 查找最近的一个结果
        distances, indices = self.index.search(query_vector, 1)
        distance = float(distances[0][0])
        faiss_idx = int(indices[0][0])

        print(f"原始距离: {distance}, 库中索引位置: {faiss_idx}, 库中总数据量: {self.index.ntotal}")

        threshold = FACE_CONFIG['threshold']

        similarity = float(np.clip(1 / (1 + np.exp((distance - threshold) / 2)), 0, 1).item())

        payload = {
            "user_id": "unknown",
            "device_id": esp32_id, # 告诉 Go 是哪个设备发来的
            "similarity": similarity, # 转换为相似度 (0~1)，方便前端展示
            "is_stranger": True
        }

        print(f"原始距离: {distance}, 计算相似度: {similarity}")
        if distance < threshold:
            # 识别成功
            matched_user_id = self.user_ids[faiss_idx]
            # 通过 http 发送到 gin，进行权限查询, 然后下发
            payload["user_id"] = matched_user_id
            payload["is_stranger"] = False
            logger.info(f"识别成功: 用户ID {matched_user_id}")
        else:
            payload["user_id"] = "stranger"
            payload["is_stranger"] = True
            logger.info(f"识别为陌生人")

        # 通过http发送
        try:
            token = FACE_CONFIG['go_token']
            headers = {'Authorization': token}
            response = requests.post(
                FACE_CONFIG['go_backend_url'], # 确保 config.py 里有这个配置项
                json=payload,
                timeout=5, # 5秒超时，防止阻塞
                headers=headers
            )
            if response.status_code == 200:
                logger.info(f"成功通知 Go 服务: {payload['user_id']}")
            else:
                logger.warning(f"Go 服务返回错误: {response.status_code}")

        except Exception as e:
            logger.error(f"连接 Go 服务失败 (请检查 Go 是否启动): {e}")

    def _rebuild_index(self):
        if self.db_handler.connect():
            user_ids, embeddings = self.db_handler.get_all_face_features()
            self.db_handler.close()

            if len(embeddings) > 0:
                embeddings = np.array(embeddings, dtype='float32')
                self._build_new_index(user_ids, embeddings)
                # 先保存，再重新加载
                self._load_or_build_index()
            else:
                logger.error("no data in database, cannot build index")

        else:
            logger.error("cannot connect to database, build index failed")

    # 一个手动刷新索引的方法
    def reload_index(self):
        logger.info("reload index...")
        self._load_or_build_index()

    def rebuild_index(self):
        logger.info("rebuild index...")
        self._rebuild_index()


# 单例模式
face_engine_instance = None

def get_face_engine_instance():
    global face_engine_instance
    if face_engine_instance is None:
        face_engine_instance = FaceRecognitionEngine()
    return face_engine_instance