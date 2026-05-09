import psycopg2
import logging
import numpy as np
from config import DB_CONFIG

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger("DBHandler")


class FaceDBHandler():
    def __init__(self, config: dict):
        self.config = config
        self.conn = None

    def connect(self) -> bool:
        try:
            self.conn = psycopg2.connect(
                host=self.config['host'],
                port=self.config['port'],
                database=self.config['database'],
                user=self.config['user'],
                password=self.config['password']
            )
            logger.info("Connected to database")
            return True

        except Exception as e:
            logger.error(e)
            return False

    def close(self) -> None:
        if self.conn:
            self.conn.close()
            logger.info("Closed connection to database")

    def get_all_face_features(self):
        if not self.conn:
            if not self.connect():
                return [], np.array([])

        cursor = None
        try:
            cursor = self.conn.cursor()
            query = "SELECT user_id, face_feature FROM iotplus.user_permissions WHERE face_feature IS NOT NULL"
            cursor.execute(query)
            rows = cursor.fetchall()
            user_ids = []
            embeddings = []

            for row in rows:
                user_id = row[0]
                feature_blob = row[1]

                # 将二进制数据还原为 numpy 数组
                embedding = np.frombuffer(feature_blob, dtype=np.float32)

                if embedding.shape == (512, ):
                    user_ids.append(user_id)
                    embeddings.append(embedding)
                else:
                    logger.warning(f"user_id: {user_id} has a wrong shape: {embedding.shape}, pass")

            embeddings_array = np.array(embeddings, dtype=np.float32)
            logger.info(f"{len(user_ids)} users have embeddings")
            return user_ids, embeddings_array
        except Exception as e:
            logger.error(f"get face features error: {e}")
            return [], np.array([])
        finally:
            if cursor:
                cursor.close()

if __name__ == "__main__":
    handler = FaceDBHandler(DB_CONFIG)
    if handler.connect():
        ids, vecs = handler.get_all_face_features()
        print(f"加载到的 IDs: {ids[:5]}...") # 打印前5个
        print(f"向量矩阵形状: {vecs.shape}")
        handler.close()
