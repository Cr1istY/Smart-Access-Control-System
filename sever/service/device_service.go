package service

import (
	"EmqxBackEnd/models"
	"EmqxBackEnd/repository"
	"context"
	"log"
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
	deviceModel.Status = "online"
	deviceModel.FirmwareVersion = "0.1"
	deviceModel.LastHeartbeat = time.Now()
	ctx := context.Background()
	log.Println("设备 注册 ", deviceModel.DeviceID)
	return s.deviceRepo.Create(ctx, &deviceModel)
}

func (s *DeviceService) UpdateHeartbeat(deviceID string, status string) error {
	ctx := context.Background()
	return s.deviceRepo.UpdateHeartbeat(ctx, deviceID, status)
}

func (s *DeviceService) ListDevices(devices *[]models.Device) error {
	ctx := context.Background()
	return s.deviceRepo.List(ctx, devices)
}

func (s *DeviceService) ListDevicesLocationAndId(devices *[]models.UpdateDevice) error {
	ctx := context.Background()
	return s.deviceRepo.ListDevicesLocationAndId(ctx, devices)
}

func (s *DeviceService) ChangeDeviceLocation(deviceID string, location string) error {
	ctx := context.Background()
	return s.deviceRepo.ChangeDeviceLocation(ctx, deviceID, location)
}

func (s *DeviceService) GetChangeDeviceDetail(device *models.UpdateDevice) error {
	ctx := context.Background()
	return s.deviceRepo.GetChangeDeviceDetail(ctx, device)
}
