package mqtt

import (
	"EmqxBackEnd/models"
	"EmqxBackEnd/service"
	"strings"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

type DeviceMqttHandler struct {
	deviceService *service.DeviceService
}

func NewDeviceMqttHandler(deviceService *service.DeviceService) *DeviceMqttHandler {
	return &DeviceMqttHandler{
		deviceService: deviceService,
	}
}

func getLastName(topicName string) string {
	// 找到最后一个 "/" 出现的位置
	lastIndex := strings.LastIndex(topicName, "/")

	if lastIndex == -1 {
		return topicName
	}

	// 截取最后一个 "/" 之后的内容
	return topicName[lastIndex+1:]
}

func (h *DeviceMqttHandler) DeviceRegister(client mqtt.Client, msg mqtt.Message) {
	deviceName := getLastName(msg.Topic())
	var creatDevice models.CreateDevice
	creatDevice.DeviceID = deviceName
	if err := h.deviceService.CreateDevice(&creatDevice); err != nil {
		_ = h.deviceService.UpdateHeartbeat(deviceName, "online")
	}
}
