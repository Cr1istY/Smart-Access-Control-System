package mqtt

import (
	"EmqxBackEnd/models"
	"EmqxBackEnd/service"
	"encoding/json"
	"log"
	"strings"
	"time"

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
		// log.Printf("解析MQTT消息JSON失败: %v", err)
		unmarshalFlag = false
	}
	if unmarshalFlag {
		if deviceMsg.Code == "400" {
			_ = h.alertService.SendWeComMessage("deviceName: "+deviceName+deviceMsg.Message, []string{"@all"}) // 发送
		}
	}
	var updateHeartBeat = false
	timeNow := time.Now()
	// 下面的代码，遍历已注册设备列表，如果设备id已经存在，那么，跳过注册，更新心跳即可
	for _, device := range h.creatDevice {
		if device.DeviceID == deviceName {
			_ = h.deviceService.UpdateHeartbeat(deviceName, "online")
			updateHeartBeat = true
			// 心跳更新后，立刻更新已注册列表，避免被离线
			err := h.GetAllDevice()
			if err != nil {
				log.Println(err)
			}
		}
		// 下面，检查是否过期，说实话，性能确实不够强，不过随便了，明天要检查，先实现一下
		// TODO: 使用REDIS管理过期心跳
		if device.Status == "online" && timeNow.After(device.LastHeartbeat.Add(time.Hour*1)) {
			// 一小时未注册，视为离线
			err := h.deviceService.DeviceOff(deviceName)
			if err != nil {
				log.Println("设备下线失败：", err)
			}
			_ = h.alertService.SendWeComMessage("deviceName: "+deviceName+" 设备过期在 "+timeNow.String(), []string{"@all"})
		}

		if updateHeartBeat {
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
