package models

import "time"

// LogQuery 用于封装复杂的查询条件
type LogQuery struct {
	UserID     string    // 用户ID
	DeviceID   string    // 设备ID
	Result     string    // 结果：success, failure
	AuthMethod string    // 认证方式：face, rfid
	StartTime  time.Time // 开始时间（零值表示不限制）
	EndTime    time.Time // 结束时间（零值表示不限制）
}

type LogQueryRequest struct {
	UserID     string `form:"user_id"`
	DeviceID   string `form:"device_id"`
	Result     string `form:"result"`
	AuthMethod string `form:"auth_method"`
	StartTime  string `form:"start_time"` // 前端通常传字符串格式的时间，如 "2023-10-01 12:00:00"
	EndTime    string `form:"end_time"`
	Page       int    `form:"page" binding:"required,min=1"`      // 必填，且最小为1
	PageSize   int    `form:"page_size" binding:"required,min=1"` // 必填，且最小为1
}
