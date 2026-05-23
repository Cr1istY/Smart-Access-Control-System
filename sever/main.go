package main

import (
	"EmqxBackEnd/database"
	"EmqxBackEnd/handlers"
	"EmqxBackEnd/mqtt"
	"EmqxBackEnd/repository"
	"EmqxBackEnd/router"
	"EmqxBackEnd/service"
	"context"
	"database/sql"
	"fmt"
	"log"
	"os"
	"os/exec"
	"os/signal"
	"strconv"
	"syscall"
	"time"

	"github.com/joho/godotenv"
)

var pythonCmds []*exec.Cmd

func startServer(dir string, args ...string) {
	cmd := exec.Command(args[0], args[1:]...)
	cmd.Dir = dir
	// Windows 下不需要也不支持 SysProcAttr 的 Setsid 字段
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	err := cmd.Start()
	if err != nil {
		log.Fatalf("启动 Python 服务器失败: %v", err)
	}
	log.Printf("Python 服务器启动成功，PID: %d", cmd.Process.Pid)

	pythonCmds = append(pythonCmds, cmd)
}

func main() {
	var runPython bool
	err := godotenv.Load()
	if err != nil {
		log.Println("注入设置失败，启用默认值")
		runPython = false
	} else {
		runPythonVal := os.Getenv("RUN_PYTHON")
		runPython, err = strconv.ParseBool(runPythonVal)
		if err != nil {
			runPython = false
		}
	}

	// 启动 python 服务器 uv 环境

	if runPython {
		startServer("./face_detect_final", "uv", "run", "main.py")
		startServer("./face_detect_final", "uv", "run", "enroll_api.py")
		startServer("./web", "npm", "run", "dev")
	}

	pdHost := os.Getenv("HOST")
	pdUser := os.Getenv("PD_USER")
	pdPass := os.Getenv("PD_USER_PASSWORD")
	pdName := os.Getenv("PD_NAME")
	pdPort := os.Getenv("PD_PORT")
	pythonToken := os.Getenv("PYTHON_TOKEN")
	if pythonToken == "" {
		log.Fatal("python端连接的密钥未配置！")
	}

	if pdHost == "" || pdUser == "" || pdPass == "" {
		log.Fatal("数据库连接的环境变量(HOST/USER/PASSWORD)未配置！")
	}

	db, err := database.InitWithConfig(pdHost, pdUser, pdPass, pdName, pdPort)
	if err != nil {
		log.Fatal("Failed to connect to DB", err)
		return
	}
	defer func(db *sql.DB) {
		_ = db.Close()
	}(db)

	dsn := fmt.Sprintf("host=%s user=%s password=%s dbname=%s port=%s sslmode=disable TimeZone=Asia/Shanghai",
		pdHost, pdUser, pdPass, pdName, pdPort)

	gorm, err := database.InitDBGorm(dsn)

	userPermissionRepo := repository.NewUserPermissionRepository(gorm)
	deviceRepo := repository.NewDeviceRepository(gorm)
	accessLogRepo := repository.NewAccessLogRepository(gorm)

	userPermissionService := service.NewUserPermissionService(userPermissionRepo)
	deviceService := service.NewDeviceService(deviceRepo)
	accessLogService := service.NewAccessLogService(accessLogRepo)
	alertService := service.NewAlertService()

	deviceMqttHandler := mqtt.NewDeviceMqttHandler(deviceService, alertService)
	err = deviceMqttHandler.GetAllDevice() // 后期改用redis，目前，直接存在程序中
	if err != nil {
		log.Println(err)
	}

	deviceHandler := handlers.NewDeviceHandler(deviceService)
	userPermissionHandler := handlers.NewUserPermissionHandler(userPermissionService, accessLogService, alertService, pythonToken)
	accessLogHandler := handlers.NewAccessLogHandler(accessLogService)

	myRouter := router.NewRouter(userPermissionHandler, deviceHandler, accessLogHandler)

	r := router.Setup(myRouter)

	mqttBroker := "mqtt://localhost:1883"
	mqttUser := ""
	mqttPass := ""
	mqttTopicRegister := os.Getenv("DEVICE_TOPIC")
	// mqttTopicOpenTheDoor := os.Getenv("OPEN_THE_DOOR_TOPIC")
	if err := mqtt.InitClient(mqttBroker, "cron_task_client", mqttUser, mqttPass, mqttTopicRegister, deviceMqttHandler); err != nil {
		log.Fatalf("MQTT初始化失败: %v", err)
	}
	defer mqtt.Close()

	quit := make(chan os.Signal, 1)
	signal.Notify(quit, syscall.SIGINT, syscall.SIGTERM)

	go func() {
		<-quit
		log.Println("\n收到退出信号，正在关闭服务...")

		for _, cmd := range pythonCmds {
			if cmd != nil && cmd.Process != nil {
				log.Printf("正在终止 python 进程 PID：%d", cmd.Process.Pid)
				_ = cmd.Process.Signal(syscall.SIGTERM)
				time.Sleep(2 * time.Second)
				_ = cmd.Process.Kill()
			}
		}
		pythonCmds = nil

		// 5秒超时
		_, cancel := context.WithTimeout(context.Background(), 5*time.Second)
		defer cancel()

		// 关闭数据库连接
		_ = db.Close()

		// 断开MQTT连接
		mqtt.Close()

		log.Println("所有资源已释放，服务已停止")
		os.Exit(0)
	}()

	err = r.Run(":8080")
	if err != nil {
		return
	}
}
