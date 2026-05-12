package handlers

import (
	"EmqxBackEnd/models"
	"EmqxBackEnd/service"
	"net/http"

	"github.com/bytedance/gopkg/util/logger"
	"github.com/gin-gonic/gin"
)

type UserPermissionHandler struct {
	userPermissionService *service.UserPermissionService
}

func NewUserPermissionHandler(userPermissionService *service.UserPermissionService) *UserPermissionHandler {
	return &UserPermissionHandler{userPermissionService: userPermissionService}
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
