import sys
import logging


try:
    from face_engine import get_face_engine_instance
    from mqtt_client import MqttHandler
    print("预加载模型")
    engine = get_face_engine_instance()
    print("加载完成")

except Exception as e:
    print(e)
    sys.exit(0)

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger("Main")

def main():
    try:
        logger.info("初始化mqtt客户端")
        mqtt_handler = MqttHandler()
        mqtt_handler.start()

    except KeyboardInterrupt:
        logger.info("检测到用户中断 正在退出")
    except Exception as e:
        logger.error(f"运行过程存在致命错误，{e}")
        sys.exit(0)

if __name__ == "__main__":
    main()