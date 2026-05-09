```txt
face_service/
├── main.py                 # 程序总入口（负责启动 MQTT 监听等）
├── config.py               # 全局配置（数据库账号、MQTT 地址、阈值等）
├── db_handler.py           # 数据库交互层（负责从 PostgreSQL 加载特征值）
├── face_engine.py          # AI 核心逻辑层（负责加载 Faiss 索引、InsightFace 推理与检索）
└── mqtt_client.py          # 通信层（负责连接 MQTT Broker、接收图片、回调处理）
```