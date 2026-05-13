DB_CONFIG = {
    'host': '127.0.0.1',
    'port': '15432',
    'database': 'postgres',
    'user': 'root',
    'password': 'root'
}

MQTT_CONFIG = {
    'broker': 'localhost',
    'port': 1883,
    'keepalive': 60,
    'device_topic': 'esp32/camera/image/+',
    'result_topic': 'esp32/face/result/',
}

FACE_CONFIG = {
    'yolo_model_path': 'models/esp32_face_640_best.pt',
    'index_path': 'face_index.bin',
    'id_map_path': 'user_ids.npy',
    'threshold': 1.2,
    'go_backend_url': 'http://localhost:8080/',
}

PROVIDER_CONFIG = [
    ('CUDAExecutionProvider', {
        'device_id': '0',
        'cudnn_conv_algo_search': 'HEURISTIC',  # 改为 HEURISTIC 以加快启动速度
        'gpu_mem_limit': 4 * 1024 * 1024 * 1024,  # 可选：限制显存使用，防止占满
    }),
    'CPUExecutionProvider'
]