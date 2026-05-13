package handlers

import (
	"EmqxBackEnd/models"
	"EmqxBackEnd/service"
	"net/http"

	"github.com/gin-gonic/gin"
)

type DeviceHandler struct {
	deviceService *service.DeviceService
}

func NewDeviceHandler(deviceService *service.DeviceService) *DeviceHandler {
	return &DeviceHandler{
		deviceService: deviceService,
	}
}

func (h *DeviceHandler) ListDevices(c *gin.Context) {
	var devices []models.Device
	if err := h.deviceService.ListDevices(&devices); err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "Failed to list devices"})
		return
	}
	c.JSON(http.StatusOK, devices)
}
