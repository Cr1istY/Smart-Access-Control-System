package service

import (
	"EmqxBackEnd/models"
	"EmqxBackEnd/repository"
	"context"
	"time"
)

type AccessLogService struct {
	accessLogRepo *repository.AccessLogRepository
}

func NewAccessLogService(accessLogRepo *repository.AccessLogRepository) *AccessLogService {
	return &AccessLogService{
		accessLogRepo: accessLogRepo,
	}
}

func (s *AccessLogService) AddAccessLog(accessLog *models.AccessLog) error {
	accessLog.AccessTime = time.Now()
	accessLog.CreatedAt = time.Now()

	ctx := context.Background()
	return s.accessLogRepo.Create(ctx, accessLog)
}
