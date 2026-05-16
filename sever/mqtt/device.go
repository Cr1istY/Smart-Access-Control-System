package mqtt

import (
	"EmqxBackEnd/models"
	"EmqxBackEnd/service"
	"log"
	"strings"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

type DeviceMqttHandler struct {
	deviceService *service.DeviceService
	creatDevice   []models.Device
}

func NewDeviceMqttHandler(deviceService *service.DeviceService) *DeviceMqttHandler {
	return &DeviceMqttHandler{
		deviceService: deviceService,
	}
}

func (h *DeviceMqttHandler) GetAllDevice() error {
	err := h.deviceService.ListDevices(&h.creatDevice)
	return err
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
	for _, device := range h.creatDevice {
		if device.DeviceID == deviceName {
			_ = h.deviceService.UpdateHeartbeat(deviceName, "online")
			return
		}
	}
	if err := h.deviceService.CreateDevice(&creatDevice); err != nil {
		log.Println(err)
	}
	err := h.GetAllDevice()
	if err != nil {
		log.Println(err)
	}
	// TODO: 考虑，处理设备其他事件
	// 例如，当使用其他解锁方式时，设备上传事件
	// 后端，对比日志和设备号，将解锁方式挂载到日志上（允许陌生人）
}
