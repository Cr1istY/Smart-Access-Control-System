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

func (s *AccessLogService) GetLogList(ctx context.Context, query *models.LogQuery, page, pageSize int) ([]*models.AccessLog, int64, error) {
	// 可以在这里增加一些业务层面的默认值处理或校验
	if page <= 0 {
		page = 1
	}
	if pageSize <= 0 || pageSize > 100 { // 限制最大每页条数，防止恶意拖垮数据库
		pageSize = 10
	}

	return s.accessLogRepo.List(ctx, query, page, pageSize)
}
