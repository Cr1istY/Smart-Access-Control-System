package service

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
)

type AlertService struct {
}

func NewAlertService() *AlertService {
	return &AlertService{}
}

var (
	EMPTYWECOMURLERR = fmt.Errorf("未配置企业微信机器人地址")
)

// WeComMessage 企业微信机器人消息结构体
type WeComMessage struct {
	MsgType string   `json:"msgtype"`
	Text    TextInfo `json:"text"`
}

type TextInfo struct {
	Content       string   `json:"content"`
	MentionedList []string `json:"mentioned_list"`
}

func (s *AlertService) SendWeComMessage(content string, mentionedList []string) error {
	webhookURL := os.Getenv("WECHAT_WEBHOOK_URL")
	if webhookURL == "" {
		return EMPTYWECOMURLERR
	}
	// 构造要发送的消息体
	msg := WeComMessage{
		MsgType: "text",
		Text: TextInfo{
			Content:       content,
			MentionedList: mentionedList, // 可以指定要@的用户，例如：["@all"]
		},
	}
	jsonData, err := json.Marshal(msg)
	if err != nil {
		return err
	}
	req, err := http.NewRequest("POST", webhookURL, bytes.NewBuffer(jsonData))
	if err != nil {
		return err
	}
	req.Header.Set("Content-Type", "application/json")
	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		return err
	}
	defer func(Body io.ReadCloser) {
		err := Body.Close()
		if err != nil {
			log.Println(err)
		}
	}(resp.Body)
	return nil
}
