package mqtt

import (
	"log"
	"sync"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

var (
	client     mqtt.Client
	clientOnce sync.Once
)

func InitClient(broker, clientID, username, password, topic string, deviceMqttHandler *DeviceMqttHandler) error {
	var initErr error
	clientOnce.Do(func() {
		opts := mqtt.NewClientOptions().
			AddBroker(broker).
			SetClientID(clientID).
			SetUsername(username).
			SetPassword(password).
			SetKeepAlive(60).
			SetPingTimeout(10 * time.Minute).
			SetAutoReconnect(true).
			SetCleanSession(true).
			SetConnectionLostHandler(func(c mqtt.Client, err error) {
				log.Println("MQTT connection lost:", err)
			}).
			SetOnConnectHandler(func(c mqtt.Client) {
				log.Println("MQTT connected")
				// 订阅主题
				token := c.Subscribe(topic, 1, nil)
				if token.WaitTimeout(5*time.Second) && token.Error() != nil {
					log.Printf("订阅主题 [%s] 失败: %v\n", topic, token.Error())
				} else {
					log.Printf("成功订阅主题: %s\n", topic)
				}
			})

		client = mqtt.NewClient(opts)
		client.AddRoute(topic, deviceMqttHandler.DeviceRegister)

		if token := client.Connect(); token.Wait() && token.Error() != nil {
			initErr = token.Error()
			return
		}

		log.Println("MQTT init over...")

	})
	return initErr
}

func GetClient() mqtt.Client {
	return client
}

func IsConnected() bool {
	return client != nil && client.IsConnected()
}

func Close() {
	if client != nil {
		client.Disconnect(250)
		log.Println("MQTT closed")
	}
}
