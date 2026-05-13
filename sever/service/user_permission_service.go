package service

import (
	"EmqxBackEnd/models"
	"EmqxBackEnd/repository"
	"context"
	"errors"
	"time"

	"github.com/google/uuid"
)

type UserPermissionService struct {
	userPermissionRepo *repository.UserPermissionRepository
}

func NewUserPermissionService(userPermissionRepo *repository.UserPermissionRepository) *UserPermissionService {
	return &UserPermissionService{
		userPermissionRepo: userPermissionRepo,
	}
}

func (s *UserPermissionService) CreateUserPermission(userPermission *models.CreateUserPermission) error {
	newUUID := uuid.NewString()
	userPermission.UserID = newUUID
	if userPermission.ValidStart.IsZero() {
		userPermission.ValidStart = time.Now()
	}
	if userPermission.ValidStart.IsZero() {
		userPermission.ValidStart = time.Now().AddDate(1, 0, 0) // 默认一年后取消授权
	}
	ctx := context.Background()
	return s.userPermissionRepo.CreateUserPermission(ctx, userPermission)
}

func (s *UserPermissionService) ListAllUser(userPermissions *[]models.CreateUserPermission) error {
	ctx := context.Background()
	return s.userPermissionRepo.ListAllUser(ctx, userPermissions)
}

func (s *UserPermissionService) ListAllUserDetail(userPermissions *[]models.UserPermission) error {
	ctx := context.Background()
	return s.userPermissionRepo.ListAllUserDetail(ctx, userPermissions)
}

func (s *UserPermissionService) GetUserPermissionById(userPermission *models.UserPermission, userID string) error {
	ctx := context.Background()
	return s.userPermissionRepo.GetUserPermissionByID(ctx, userPermission, userID)
}

func (s *UserPermissionService) UpdateUserPermission(userPermission *models.CreateUserPermission) error {
	if userPermission.UserID == "" {
		return errors.New("user_id is required")
	}
	ctx := context.Background()
	return s.userPermissionRepo.UpdateUserPermission(ctx, userPermission)
}

func (s *UserPermissionService) CheckPermission(userID string, targetDeviceID string) (bool, error) {
	ctx := context.Background()
	return s.userPermissionRepo.CheckPermission(ctx, userID, targetDeviceID)
}
