package service

import (
	"EmqxBackEnd/models"
	"EmqxBackEnd/repository"
	"context"
	"time"
)

type DeviceService struct {
	deviceRepo *repository.DeviceRepository
}

func NewDeviceService(deviceRepo *repository.DeviceRepository) *DeviceService {
	return &DeviceService{
		deviceRepo: deviceRepo,
	}
}

func (s *DeviceService) CreateDevice(device *models.CreateDevice) error {
	var deviceModel models.Device
	deviceModel.DeviceID = device.DeviceID
	deviceModel.CreatedAt = time.Now()
	deviceModel.Status = "active"
	deviceModel.FirmwareVersion = "0.1"
	deviceModel.LastHeartbeat = time.Now()
	ctx := context.Background()
	return s.deviceRepo.Create(ctx, &deviceModel)
}

func (s *DeviceService) UpdateHeartbeat(deviceID string, status string) error {
	ctx := context.Background()
	return s.deviceRepo.UpdateHeartbeat(ctx, deviceID, status)
}
