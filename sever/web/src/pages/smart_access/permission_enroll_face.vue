<template>
  <div class="face-enroll-container">
    <el-card class="box-card">
      <template #header>
        <div class="card-header">
          <span>智能门禁 - 人脸信息录入</span>
        </div>
      </template>

      <!-- 用户信息输入 -->
      <el-form :model="form" label-width="100px" style="max-width: 500px; margin-bottom: 20px;">
        <el-form-item label="用户ID">
          <el-input v-model="form.userId" placeholder="请输入需要录入的用户ID" />
        </el-form-item>
      </el-form>

      <!-- 摄像头与拍照区域 -->
      <div class="camera-area">
        <div class="video-wrapper">
          <video ref="videoRef" autoplay playsinline class="video-element"></video>
          <!-- 隐藏的 Canvas 用于截取图片 -->
          <canvas ref="canvasRef" style="display: none;"></canvas>
        </div>
        
        <div class="preview-wrapper" v-if="photoData">
          <p>拍照预览：</p>
          <img :src="photoData" alt="预览图" class="preview-img" />
        </div>
      </div>

      <!-- 操作按钮 -->
      <div class="action-buttons">
        <el-button type="primary" @click="startCamera" :disabled="isCameraOpen">打开摄像头</el-button>
        <el-button type="success" @click="takePhoto" :disabled="!isCameraOpen || isProcessing">拍照</el-button>
        <el-button type="warning" @click="retakePhoto" :disabled="!photoData || isProcessing">重拍</el-button>
        <el-button type="danger" @click="stopCamera" :disabled="!isCameraOpen">关闭摄像头</el-button>
        <el-button 
          type="primary" 
          @click="submitEnroll" 
          :loading="isProcessing" 
          :disabled="!photoData || !form.userId"
        >
          提交录入
        </el-button>
      </div>
    </el-card>
  </div>
</template>

<script setup lang="js">
import { ref, reactive } from 'vue'
import { ElMessage } from 'element-plus'

// 响应式数据
const videoRef = ref(null)
const canvasRef = ref(null)
const photoData = ref('') // 存储 base64 预览图
const isCameraOpen = ref(false)
const isProcessing = ref(false)

const form = reactive({
  userId: ''
})

let mediaStream = null

// 1. 打开摄像头
const startCamera = async () => {
  try {
    mediaStream = await navigator.mediaDevices.getUserMedia({ 
      video: { width: 640, height: 480, facingMode: 'user' } 
    })
    if (videoRef.value) {
      videoRef.value.srcObject = mediaStream
      isCameraOpen.value = true
    }
  } catch (error) {
    console.error('无法访问摄像头:', error)
    ElMessage.error('无法访问摄像头，请检查设备权限。')
  }
}

// 2. 关闭摄像头
const stopCamera = () => {
  if (mediaStream) {
    mediaStream.getTracks().forEach(track => track.stop())
    if (videoRef.value) videoRef.value.srcObject = null
    isCameraOpen.value = false
    photoData.value = ''
  }
}

// 3. 拍照
const takePhoto = () => {
  if (!videoRef.value || !canvasRef.value) return
  
  const video = videoRef.value
  const canvas = canvasRef.value
  const context = canvas.getContext('2d')
  
  canvas.width = 640
  canvas.height = 640
  
  // 计算视频画面的宽高比，进行居中裁剪缩放
  const scale = Math.max(640 / video.videoWidth, 640 / video.videoHeight)
  const x = (640 / 2) - (video.videoWidth / 2) * scale
  const y = (640 / 2) - (video.videoHeight / 2) * scale
  
  // 绘制时进行缩放和居中
  context.drawImage(video, x, y, video.videoWidth * scale, video.videoHeight * scale)
  
  photoData.value = canvas.toDataURL('image/jpeg', 0.6)
}

// 4. 重拍
const retakePhoto = () => {
  photoData.value = ''
}

// 5. 提交录入 (核心逻辑：将图片转为后端需要的二进制流)
const submitEnroll = async () => {
  if (!form.userId || !photoData.value) {
    ElMessage.warning('请填写用户ID并拍照')
    return
  }

  isProcessing.value = true
  
  try {
    // 将 base64 转为 Blob 二进制对象
    const blob = await (await fetch(photoData.value)).blob()
    
    // 使用 FormData 封装数据
    const formData = new FormData()
    formData.append('user_id', form.userId)
    // 后端使用 image = data.get("image") 获取，这里字段名必须对应
    formData.append('image', blob, 'face.png') 

    // 发送请求到后端 (注意修改为你实际的后端地址)
    const response = await fetch('http://localhost:5001/api/enroll', {
      method: 'POST',
      body: formData
      // 注意：使用 FormData 时，千万不要手动设置 Content-Type，让浏览器自动识别并带上 boundary
    })

    const result = await response.json()

    if (result.status === 'success') {
      ElMessage.success('人脸录入成功！')
      // 录入成功后重置状态
      retakePhoto()
      form.userId = ''
    } else {
      ElMessage.error(result.message || '录入失败')
    }
  } catch (error) {
    console.error('提交出错:', error)
    ElMessage.error('网络请求失败，请检查后端服务是否启动')
  } finally {
    isProcessing.value = false
  }
}
</script>

<style scoped>
.face-enroll-container {
  display: flex;
  justify-content: center;
  padding: 20px;
  background-color: #f5f7fa;
  min-height: 100vh;
}
.box-card {
  width: 800px;
}
.card-header {
  font-weight: bold;
  font-size: 18px;
}
.camera-area {
  display: flex;
  justify-content: space-around;
  align-items: flex-start;
  margin-bottom: 20px;
}
.video-wrapper, .preview-wrapper {
  text-align: center;
}
.video-element {
  width: 400px;
  height: 300px;
  background-color: #000;
  border-radius: 4px;
}
.preview-img {
  width: 400px;
  height: 300px;
  object-fit: cover;
  border-radius: 4px;
  border: 2px solid #409eff;
}
.action-buttons {
  text-align: center;
}
</style>