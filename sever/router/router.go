package router

import (
	"EmqxBackEnd/handlers"
	"EmqxBackEnd/middleware"
	"time"

	"github.com/gin-contrib/cors"
	"github.com/gin-gonic/gin"
)

type Router struct {
	userPermissionHandler *handlers.UserPermissionHandler
	deviceHandler         *handlers.DeviceHandler
	logPermissionHandler  *handlers.AccessLogHandler
}

func NewRouter(userPermissionHandler *handlers.UserPermissionHandler, deviceHandler *handlers.DeviceHandler, logPermissionHandler *handlers.AccessLogHandler) *Router {
	return &Router{
		userPermissionHandler: userPermissionHandler,
		deviceHandler:         deviceHandler,
		logPermissionHandler:  logPermissionHandler,
	}
}

func Setup(router *Router) *gin.Engine {
	r := gin.Default()

	corsConfig := cors.Config{
		AllowAllOrigins: false, // 生产环境必须设为false
		AllowOrigins: []string{
			"http://localhost:3000",
			"http://localhost:5173",
			"http://172.20.10.5:5173"},
		AllowMethods:     []string{"GET", "POST", "PUT", "DELETE", "OPTIONS"},
		AllowHeaders:     []string{"Origin", "Content-Type", "Authorization"},
		AllowCredentials: true,           // 允许携带Cookie
		MaxAge:           12 * time.Hour, // 预检请求缓存时间
	}

	// 应用中间件
	r.Use(cors.New(corsConfig))
	r.Use(gin.Logger())
	r.Use(gin.Recovery())
	// r.POST("/empx", handlers.Empx)
	r.POST("/empx/saveMessage", handlers.ReceiveEmpx)
	r.POST("/admin/login", handlers.Login)
	protected := r.Group("")
	protected.Use(middleware.AuthMiddlewareWithCache())
	{
		protected.GET("/admin/getinfo", handlers.GetAdminByAuth)
		protected.GET("/empx/getMessage/:type", handlers.GetMessages)
		protected.GET("/empx/openTheDoor/:nodeId", handlers.OpenTheDoor)
		protected.GET("/empx/closeTheDoor/:nodeId", handlers.CloseTheDoor)
		protected.POST("/admin/register", handlers.Register)
		protected.POST("/admin/saveNode", handlers.SaveNode)
		protected.POST("/admin/changeUserStatus", handlers.ChangeUserStatus)
		protected.GET("/admin/getAllUser", handlers.GetAllUsers)
		protected.GET("/admin/getAllNode", handlers.GetAllNodeByUserId)
	}
	taskGroup := protected.Group("/task")
	{
		taskGroup.GET("", handlers.GetTasksHandler)                      // 获取任务列表
		taskGroup.PUT("/:name/cron", handlers.UpdateTaskCronHandler)     // 更新Cron表达式
		taskGroup.PUT("/:name/status", handlers.UpdateTaskStatusHandler) // 启用/禁用任务
	}
	userPermissionGroup := protected.Group("/permission")
	{
		userPermissionGroup.POST("/add", router.userPermissionHandler.CreateUserPermission)
		userPermissionGroup.GET("/list", router.userPermissionHandler.ListAllUserDetail)
		userPermissionGroup.POST("/update", router.userPermissionHandler.UpdateUserPermission)
		userPermissionGroup.GET("/getUserById/:user_id", router.userPermissionHandler.GetUserPermissionById)
	}
	r.POST("/permission/checkPermission", router.userPermissionHandler.CheckUserPermission)
	deviceGroup := protected.Group("/device")
	{
		deviceGroup.GET("/list", router.deviceHandler.ListDevices)
		deviceGroup.POST("/changeLocation", router.deviceHandler.ChangeDeviceLocation)
		deviceGroup.GET("/getChangeDeviceLocationDetail/:deviceID", router.deviceHandler.GetChangeDeviceDetail)
		deviceGroup.GET("/listLocationAndId", router.deviceHandler.ListDevicesLocationAndId)
	}
	accessLogGroup := protected.Group("/accessLog")
	{
		accessLogGroup.GET("/query", router.logPermissionHandler.ListAccessLogs)
	}

	return r
}
