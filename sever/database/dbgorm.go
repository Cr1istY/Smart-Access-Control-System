package database

import (
	"fmt"
	"log"
	"time"

	"gorm.io/driver/postgres"
	"gorm.io/gorm"
)

var DBGorm *gorm.DB

// InitDBGorm 初始化数据库连接
func InitDBGorm(dsn string) (*gorm.DB, error) {
	var err error
	DBGorm, err = gorm.Open(postgres.Open(dsn), &gorm.Config{})
	if err != nil {
		log.Fatalf("❌ 数据库连接失败: %v", err)
		return nil, err
	}

	fmt.Println("✅ 数据库连接成功")

	// 获取底层 *sql.DB 对象以配置连接池
	sqlDB, err := DBGorm.DB()
	if err != nil {
		log.Fatal(err)
	}

	// 设置连接池参数（生产环境重要）
	sqlDB.SetMaxIdleConns(10)           // 最大空闲连接数
	sqlDB.SetMaxOpenConns(100)          // 最大打开连接数
	sqlDB.SetConnMaxLifetime(time.Hour) // 连接最大存活时间

	return DBGorm, nil
}
