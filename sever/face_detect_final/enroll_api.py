import base64

import cv2
from flask import Flask, request, jsonify



from config import DB_CONFIG
from db_handler import FaceDBHandler
from face_engine import get_face_engine_instance

import numpy as np
import logging
app = Flask(__name__)
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger("EnrollApi")

engine = get_face_engine_instance()
db_handler = FaceDBHandler(DB_CONFIG)

@app.route("/api/enroll", methods=["POST"])
def enroll_api():
    user_id = request.form.get('user_id')

    if 'image' not in request.files:
        return jsonify({"status": "error", "message": "No image part"}), 400

    image_file = request.files['image']

    if image_file.filename == '':
        return jsonify({"status": "error", "message": "No selected image"}), 400

    image = image_file.read()

    if not user_id or not image:
        return jsonify({"status": "error", "message": "Please provide user_id and image"}), 400

    try:
        # 应该直接传递字节流
        image_data = np.frombuffer(image, dtype=np.uint8)
        img = cv2.imdecode(image_data, cv2.IMREAD_COLOR)
        if img is None:
            return jsonify({"status": "error", "message": "Please provide image"}), 400

        faces = engine.face_app.get(img)
        if not faces:
            return jsonify({"status": "error", "message": "No face"}), 400

        # 取最大的一张脸
        face = faces[0]
        if not face:
            logger.warning("在裁剪的人脸区域中未检测到有效人脸，跳过本次识别")
            return jsonify({"status": "error", "message": "No face"}), 400
        embedding = face.normed_embedding
        logger.info(f"正在录入 {user_id}:{image}")
        if db_handler.connect():
            success = db_handler.insert_face_feature(user_id, embedding.tobytes())
            db_handler.close()

            if success:
                # TODO 后期必须修改为 每天凌晨无客流时进行索引重建
                engine.rebuild_index()

                return jsonify({"status": "success", "message": "Face enrollment successful"}), 200
            else:
                return jsonify({"status": "error", "message": "Face enrollment failed"}), 400
        else:
            return jsonify({"status": "error", "message": "database connect failed"}), 400


    except Exception as e:
        logger.error(f"录入时出现错误: {e}")
        return jsonify({"status": "error", "message": "Something went wrong"}), 500

if __name__ == '__main__':
    app.run(host='localhost', port=5001)



