package mqtt

import (
	"EmqxBackEnd/models"
	"EmqxBackEnd/service"
	"encoding/json"
	"log"
	"strings"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

type DeviceMqttHandler struct {
	deviceService *service.DeviceService
	alertService  *service.AlertService
	creatDevice   []models.Device
}

// 200 - 普通消息
// 400 - 出错，读取ESP32-s3发送的消息，解析

type DeviceMessage struct {
	Code    string `json:"code"`
	Message string `json:"message"`
}

func NewDeviceMqttHandler(deviceService *service.DeviceService, alertService *service.AlertService) *DeviceMqttHandler {
	return &DeviceMqttHandler{
		deviceService: deviceService,
		alertService:  alertService,
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
	unmarshalFlag := true
	var creatDevice models.CreateDevice
	creatDevice.DeviceID = deviceName
	// 解析消息
	var deviceMsg DeviceMessage
	if err := json.Unmarshal(msg.Payload(), &deviceMsg); err != nil {
		log.Printf("解析MQTT消息JSON失败: %v", err)
		unmarshalFlag = false
	}
	if unmarshalFlag {
		if deviceMsg.Code == "400" {
			_ = h.alertService.SendWeComMessage("deviceName: "+deviceMsg.Message, []string{"@all"}) // 发送
		}
	}
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
