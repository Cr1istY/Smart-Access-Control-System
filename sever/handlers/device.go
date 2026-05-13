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

func (h *DeviceHandler) ChangeDeviceLocation(c *gin.Context) {
	var device models.UpdateDevice
	if err := c.ShouldBindJSON(&device); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "on bind json"})
		return
	}
	if err := h.deviceService.ChangeDeviceLocation(device.DeviceID, device.Location); err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "Failed to change device location"})
		return
	}
	c.JSON(http.StatusOK, gin.H{"message": "Device location changed successfully"})

}

func (h *DeviceHandler) GetChangeDeviceDetail(c *gin.Context) {
	deviceID := c.Param("deviceID")
	var device models.UpdateDevice
	device.DeviceID = deviceID
	if err := h.deviceService.GetChangeDeviceDetail(&device); err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "Failed to get device detail"})
		return
	}
	c.JSON(http.StatusOK, device)

}
