package handlers

import (
	"EmqxBackEnd/models"
	"EmqxBackEnd/service"
	"net/http"
	"time"

	"github.com/gin-gonic/gin"
)

type AccessLogHandler struct {
	accessLogService *service.AccessLogService
}

func NewAccessLogHandler(accessLogService *service.AccessLogService) *AccessLogHandler {
	return &AccessLogHandler{
		accessLogService: accessLogService,
	}
}

func (h *AccessLogHandler) ListAccessLogs(c *gin.Context) {
	var req models.LogQueryRequest
	// 自动绑定并校验 Query 参数 (GET 请求) 或 Form 表单参数
	if err := c.ShouldBindQuery(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "参数错误: " + err.Error()})
		return
	}

	// 将 Request DTO 转换为 Repo 层需要的 LogQuery 结构体
	query := &models.LogQuery{
		UserID:     req.UserID,
		DeviceID:   req.DeviceID,
		Result:     req.Result,
		AuthMethod: req.AuthMethod,
	}

	// 解析时间范围 (根据你的实际时间格式调整 Layout)
	if req.StartTime != "" {
		if t, err := time.Parse("2006-01-02 15:04:05", req.StartTime); err == nil {
			query.StartTime = t
		}
	}
	if req.EndTime != "" {
		if t, err := time.Parse("2006-01-02 15:04:05", req.EndTime); err == nil {
			query.EndTime = t
		}
	}

	// 调用 Service 层获取数据
	logs, total, err := h.accessLogService.GetLogList(c.Request.Context(), query, req.Page, req.PageSize)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "获取日志失败"})
		return
	}

	c.JSON(http.StatusOK, gin.H{
		"code":    0,
		"message": "success",
		"data": gin.H{
			"list":      logs,
			"total":     total,
			"page":      req.Page,
			"page_size": req.PageSize,
		},
	})
}
