DB_CONFIG = {
    'host': '127.0.0.1',
    'port': '15432',
    'database': 'postgres',
    'user': 'root',
    'password': 'root'
}

MQTT_CONFIG = {
    'broker': 'mosquitto',
    'port': 1883,
    'device_topic': 'esp32/face/image'
}

FACE_CONFIG = {
    'index_path': 'face_index.bin',
    'id_map_path': 'user_ids.npy',
    'threshold': 0.8
}