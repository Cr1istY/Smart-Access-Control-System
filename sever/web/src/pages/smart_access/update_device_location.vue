<template>
  <div class="device-edit-container">
    <el-card class="box-card" shadow="hover">
      <template #header>
        <div class="card-header">
          <span>编辑设备信息</span>
          <!-- 返回列表按钮 -->
          <el-button @click="router.back()">返回</el-button>
        </div>
      </template>

      <!-- 编辑表单 -->
      <el-form 
        ref="formRef"
        :model="formData" 
        :rules="rules" 
        label-width="100px" 
        style="max-width: 600px; margin: 20px auto;"
      >
        <!-- 设备ID（只读展示） -->
        <el-form-item label="设备ID" prop="device_id">
          <el-input v-model="formData.device_id" disabled placeholder="设备唯一标识" />
        </el-form-item>

        <!-- 安装位置（可编辑） -->
        <el-form-item label="安装位置" prop="location">
          <el-input 
            v-model="formData.location" 
            placeholder="请输入设备的安装位置" 
            clearable 
          />
        </el-form-item>

        <!-- 底部操作按钮 -->
        <el-form-item>
          <el-button type="primary" @click="handleSubmit" :loading="submitLoading">保存修改</el-button>
          <el-button @click="handleReset">重置</el-button>
        </el-form-item>
      </el-form>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { ElMessage, type FormInstance, type FormRules } from 'element-plus'
import axios from "@/axios"

const route = useRoute()
const router = useRouter()
const formRef = ref<FormInstance>()
const submitLoading = ref(false)

// 对应后端 UpdateDevice 结构体的前端表单接口
interface UpdateDeviceForm {
  device_id: string
  location: string
}

// 表单数据初始化
const formData = reactive<UpdateDeviceForm>({
  device_id: '',
  location: ''
})

// 表单校验规则
const rules = reactive<FormRules<UpdateDeviceForm>>({
  location: [
    { required: true, message: '请输入安装位置', trigger: 'blur' },
    { min: 2, max: 150, message: '长度在 2 到 150 个字符', trigger: 'blur' }
  ]
})

// 获取设备详情（用于回显数据）
const fetchDeviceDetail = async (deviceId: string) => {
  try {
    // 替换为真实的后端接口请求
    const res = await axios.get(`/device/getChangeDeviceLocationDetail/${deviceId}`)
    const data = res.data
    formData.device_id = data.device_id
    formData.location = data.location
    
  } catch (error) {
    ElMessage.error('获取设备详情失败')
    console.error(error)
  }
}

// 提交表单
const handleSubmit = async () => {
  if (!formRef.value) return
  
  await formRef.value.validate(async (valid) => {
    if (valid) {
      submitLoading.value = true
      try {
        // 替换为真实的后端更新接口请求
        await axios.post('/device/changeLocation', formData)
        
        // 模拟接口延迟
        await new Promise(resolve => setTimeout(resolve, 800))
        
        ElMessage.success('设备信息更新成功！')
        // 更新成功后返回列表页或上一页
        router.back()
      } catch (error) {
        ElMessage.error('更新失败，请重试')
        console.error(error)
      } finally {
        submitLoading.value = false
      }
    }
  })
}

// 重置表单
const handleReset = () => {
  if (!formRef.value) return
  formRef.value.resetFields()
}

// 页面加载时获取路由参数并拉取详情
onMounted(() => {
  // 动态获取 URL 后面的 device_id (例如 /edit/DEV-001)
  const deviceId = route.params.device_id as string
  
  if (deviceId) {
    fetchDeviceDetail(deviceId)
  } else {
    ElMessage.error('缺少设备ID参数')
    router.back()
  }
})
</script>

<style scoped>
.device-edit-container {
  padding: 20px;
  background-color: #f5f7fa;
  min-height: 100vh;
}
.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-weight: bold;
  font-size: 18px;
}
</style>