package database

import (
	"EmqxBackEnd/config"
	"database/sql"
	"fmt"
	"log"
	"time"

	_ "github.com/lib/pq"
)

var DB *sql.DB

func Init() (*sql.DB, error) {
	var err error
	DB, err = sql.Open("postgres", config.DBConnStr)
	if err != nil {
		log.Fatal("Failed to connect to DB", err)
	}
	if err = DB.Ping(); err != nil {
		log.Fatal("Failed to ping DB", err)
	}

	DB.SetMaxOpenConns(25)
	DB.SetMaxIdleConns(5)
	DB.SetConnMaxLifetime(5 * time.Minute)

	if err := DB.Ping(); err != nil {
		log.Fatal("Failed to ping DB", err)
		return nil, err
	}
	return DB, nil
}

func InitWithConfig(host, user, password, name, port string) (*sql.DB, error) {
	var err error

	DBConfig := fmt.Sprintf("postgres://%s:%s@%s:%s/%s?sslmode=disable", user, password, host, port, name)

	DB, err = sql.Open("postgres", DBConfig)
	if err != nil {
		log.Fatal("Failed to connect to DB", err)
	}
	if err = DB.Ping(); err != nil {
		log.Fatal("Failed to ping DB", err)
	}

	DB.SetMaxOpenConns(25)
	DB.SetMaxIdleConns(5)
	DB.SetConnMaxLifetime(5 * time.Minute)

	if err := DB.Ping(); err != nil {
		log.Fatal("Failed to ping DB", err)
		return nil, err
	}
	return DB, nil
}
