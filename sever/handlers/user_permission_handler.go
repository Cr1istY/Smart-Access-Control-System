package handlers

import (
	"EmqxBackEnd/models"
	"EmqxBackEnd/mqtt"
	"EmqxBackEnd/service"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"sync"
	"time"

	"github.com/bytedance/gopkg/util/logger"
	"github.com/gin-gonic/gin"
)

// 冷却时间机制引入
var cooldownCache sync.Map

const cooldownDuration = 60 * time.Second

type UserPermissionHandler struct {
	userPermissionService *service.UserPermissionService
	accessLogService      *service.AccessLogService
	alertService          *service.AlertService
	token                 string
}

func NewUserPermissionHandler(userPermissionService *service.UserPermissionService, accessLogService *service.AccessLogService, alertService *service.AlertService, token string) *UserPermissionHandler {
	return &UserPermissionHandler{
		userPermissionService: userPermissionService,
		accessLogService:      accessLogService,
		alertService:          alertService,
		token:                 token,
	}
}

// CreateUserPermission 新建用户
// 传入: name, number, validStart, validEnd
func (h *UserPermissionHandler) CreateUserPermission(c *gin.Context) {
	var createUserPermission models.CreateUserPermission
	if err := c.ShouldBindJSON(&createUserPermission); err != nil {
		logger.Debug("create user permission failed", err)
		c.JSON(http.StatusBadRequest, gin.H{"error": "create user permission failed"})
		return
	}

	if len(createUserPermission.Number) <= 0 {
		c.JSON(http.StatusBadRequest, gin.H{"error": "number is required"})
		return
	}

	// 传指针，最后返回创建号的 user_id, 前端，顺势查找
	if err := h.userPermissionService.CreateUserPermission(&createUserPermission); err != nil {
		logger.Debug("create user permission failed", err)
		c.JSON(http.StatusBadRequest, gin.H{"error": "create user permission failed"})
		return
	}
	if createUserPermission.UserID == "" {
		logger.Debug("create user permission failed user_id is empty")
		c.JSON(http.StatusBadRequest, gin.H{"error": "user_id created failed"})
		return
	}
	userId := createUserPermission.UserID
	c.JSON(http.StatusOK, gin.H{"user_id": userId})
}

func (h *UserPermissionHandler) ListAllUser(c *gin.Context) {
	var createUserPermissions []models.CreateUserPermission
	if err := h.userPermissionService.ListAllUser(&createUserPermissions); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "list all user failed"})
		return
	}
	var listAllUserStructs []models.ListUser
	for _, createUserPermission := range createUserPermissions {
		listAllUserStructs = append(listAllUserStructs, models.ListUser{
			UserID: createUserPermission.UserID,
			Name:   createUserPermission.Name,
		})
	}
	c.JSON(http.StatusOK, listAllUserStructs)
}

func (h *UserPermissionHandler) ListAllUserDetail(c *gin.Context) {
	var UserPermissions []models.UserPermission
	if err := h.userPermissionService.ListAllUserDetail(&UserPermissions); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "list all user failed"})
		return
	}
	c.JSON(http.StatusOK, UserPermissions)
}

func (h *UserPermissionHandler) GetUserPermissionById(c *gin.Context) {
	userID := c.Param("user_id")
	if userID == "" {
		c.JSON(http.StatusBadRequest, gin.H{"error": "user_id is empty"})
		return
	}
	var userPermission models.UserPermission
	userPermission.UserID = userID
	if err := h.userPermissionService.GetUserPermissionById(&userPermission, userID); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "get user permission failed"})
		return
	}
	c.JSON(http.StatusOK, userPermission)
}

func (h *UserPermissionHandler) UpdateUserPermission(c *gin.Context) {
	var userPermission models.CreateUserPermission
	if err := c.ShouldBindJSON(&userPermission); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "bind json failed"})
		return
	}
	if err := h.userPermissionService.UpdateUserPermission(&userPermission); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "update user permission failed"})
		return
	}
	c.JSON(http.StatusOK, gin.H{"message": "update user permission success"})
}

func (h *UserPermissionHandler) CheckUserPermission(c *gin.Context) {
	token := c.GetHeader("Authorization")
	if token == "" {
		c.JSON(http.StatusUnauthorized, gin.H{"error": "缺少认证信息"})
		return
	}
	if token != h.token {
		c.JSON(http.StatusUnauthorized, gin.H{"error": "Token 无效"})
		c.Abort()
		return
	}
	var checkUserPermission models.CheckUserPermission
	if err := c.ShouldBindJSON(&checkUserPermission); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "bind json failed"})
		return
	}
	// 陌生人
	if checkUserPermission.IsStranger {
		// 发送警告
		_ = h.alertService.SendWeComMessage(checkUserPermission.DeviceID+"出现未授权用户", []string{"@all"})
		// 存入数据库，包括图片路径
		var accessLog models.AccessLog
		accessLog.UserID = checkUserPermission.UserID // TODO: 创建陌生人用户ID
		accessLog.DeviceID = checkUserPermission.DeviceID
		accessLog.AuthMethod = "face"
		accessLog.Result = "fail"
		accessLog.Reason = "这是个陌生人"
		accessLog.PhotoURL = checkUserPermission.PhotoURL
		err := h.accessLogService.AddAccessLog(&accessLog)
		if err != nil {
			log.Println(err)
		}
		c.JSON(http.StatusOK, gin.H{"error": "stranger"})
		return
	}

	ok, err := h.userPermissionService.CheckPermission(checkUserPermission.UserID, checkUserPermission.DeviceID)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "check user permission failed"})
		return
	}

	if ok {
		c.JSON(http.StatusOK, gin.H{"message": "check user permission success"})
		err = h.openTheDoorByFace(checkUserPermission)
		if err != nil {
			log.Println(err)
			ok = false
			_ = h.insertLog(&checkUserPermission, ok) // 记入日志
			return
		}
	} else {
		c.JSON(http.StatusOK, gin.H{"error": "no permission"})
	}
	_ = h.insertLog(&checkUserPermission, ok) // 记入日志
}

func (h *UserPermissionHandler) insertLog(checkUserPermission *models.CheckUserPermission, ok bool) error {
	cacheKey := fmt.Sprintf("%s:%s", checkUserPermission.UserID, checkUserPermission.DeviceID)
	if lastTime, isOk := cooldownCache.Load(cacheKey); isOk {
		// 获取上次记录的时间
		lastRecordTime := lastTime.(time.Time)
		// 如果当前时间距离上次记录的时间小于冷却时间，说明还在冷却期，直接跳过
		if time.Since(lastRecordTime) < cooldownDuration {
			// fmt.Printf("⏳ 用户 [%s] 在设备 [%s] 处于冷却期，跳过重复记录\n", checkUserPermission.UserID, checkUserPermission.DeviceID)
			return nil
		}
	}
	var accessLog models.AccessLog
	accessLog.UserID = checkUserPermission.UserID
	accessLog.DeviceID = checkUserPermission.DeviceID
	accessLog.AuthMethod = "face"
	if ok {
		accessLog.Result = "success"
	} else {
		accessLog.Result = "fail"
		accessLog.Reason = "no permission"
	}
	if err := h.accessLogService.AddAccessLog(&accessLog); err != nil {
		return err
	}
	cooldownCache.Store(cacheKey, time.Now())
	return nil
}

func (h *UserPermissionHandler) openTheDoorByFace(permission models.CheckUserPermission) error {
	// 检查MQTT连接
	if !mqtt.IsConnected() {
		return fmt.Errorf("mqtt not connected")
	}
	client := mqtt.GetClient()
	if client == nil {
		return fmt.Errorf("MQTT客户端未初始化")
	}
	payload := map[string]interface{}{
		"device_id": permission.DeviceID,
		"action":    "open_door_face",
		"timestamp": time.Now().Unix(),
	}
	jsonData, err := json.Marshal(payload)
	if err != nil {
		return fmt.Errorf("payload marshal failed in sending mqtt to : %w", err)
	}
	// 发布参数
	qos := byte(1)
	topic := "esp32/go/openTheDoor/" + permission.DeviceID

	// 发布消息
	go func() {
		token := client.Publish(topic, qos, false, jsonData)
		// 设置合理的超时时间
		if !token.WaitTimeout(5 * time.Second) {
			log.Printf("[MQTT] 开门指令发布超时, device: %s", permission.DeviceID)
			return
		}
		if token.Error() != nil {
			log.Printf("[MQTT] 开门指令发布失败: %v, device: %s", token.Error(), permission.DeviceID)
			// 这里可以加入重试逻辑或写入死信队列
		} else {
			log.Printf("[MQTT] 开门指令发送成功, device: %s", permission.DeviceID)
		}
	}()

	return nil
}
