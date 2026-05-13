<template>
  <div class="permission-container">
    <el-card class="permission-card">
      <template #header>
        <div class="card-header">
          <span>更新用户权限</span>
        </div>
      </template>

      <el-form
        ref="formRef"
        :model="form"
        :rules="rules"
        label-width="100px"
        style="max-width: 600px"
      >
        <!-- user_id -->
        <el-form-item label="用户ID" prop="user_id">
          <el-input v-model="form.user_id" disabled placeholder="请从URL获取用户ID" />
        </el-form-item>

        <el-form-item label="姓名" prop="name">
          <el-input v-model="form.name" placeholder="请输入用户姓名" />
        </el-form-item>

        <el-form-item label="编号" prop="number">
          <el-input v-model="form.number" placeholder="请输入用户编号" />
        </el-form-item>

        <!-- 允许通行设备 (带状态标识的多选框) -->
        <el-form-item label="通行设备" prop="allowed_devices">
          <el-select
            v-model="form.allowed_devices"
            multiple
            filterable
            placeholder="请选择需要授权的通行设备"
            style="width: 100%"
          >
            <el-option
              v-for="device in allDevices"
              :key="device.device_id"
              :label="device.location"
              :value="device.device_id"
            >
              <!-- 自定义下拉选项的内容，用于区分是否已授权 -->
              <div class="device-option-content">
                <span>{{ device.location }}</span>
                <span class="device-id-text">({{ device.device_id }})</span>
                <!-- 如果当前设备的ID在 form.allowed_devices 数组里，显示绿色对勾 -->
                <el-icon v-if="form.allowed_devices.includes(device.device_id)" color="#67C23A">
                  <Check />
                </el-icon>
              </div>
            </el-option>
          </el-select>
          <div class="form-tip">带有 ✔️ 标记的为已授权设备</div>
        </el-form-item>

        <!-- 有效期时间范围选择 -->
        <el-form-item label="有效期" required>
          <el-date-picker
            v-model="validRange"
            type="datetimerange"
            range-separator="至"
            start-placeholder="开始时间"
            end-placeholder="结束时间"
            value-format="YYYY-MM-DDTHH:mm:ssZ"
            @change="handleDateChange"
          />
        </el-form-item>

        <el-form-item>
          <el-button type="primary" :loading="submitLoading" @click="handleSubmit">
            提交更新
          </el-button>
          <el-button type="warning" :loading="loading" @click="handleEnrollFace(form.user_id)">
            录入人脸
          </el-button>
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
import { Check } from '@element-plus/icons-vue' // 引入对勾图标
import axios from '@/axios'

const route = useRoute()
const router = useRouter()
const formRef = ref<FormInstance>()
const loading = ref(false)
const submitLoading = ref(false)

// 定义设备接口的 TS 类型
interface DeviceOption {
  device_id: string
  location: string
}

// 存储从后端获取的所有设备列表
const allDevices = ref<DeviceOption[]>([])

// 表单数据绑定
const form = reactive({
  user_id: '',
  name: '',
  number: '',
  allowed_devices: [] as string[], 
  valid_start: '',
  valid_end: ''
})

const validRange = ref<[string, string] | []>([])

// 表单校验规则
const rules: FormRules = {
  user_id: [{ required: true, message: '用户ID不能为空', trigger: 'blur' }],
  name: [{ required: true, message: '请输入姓名', trigger: 'blur' }],
  number: [{ required: true, message: '请输入编号', trigger: 'blur' }],
  allowed_devices: [{ type: 'array', required: true, message: '请至少选择一个通行设备', trigger: 'change' }]
}

// 1. 获取所有设备列表 (listLocationAndId)
const fetchAllDevices = async () => {
  try {
    const res = await axios.get('/device/listLocationAndId')
    allDevices.value = res.data || []
  } catch (error) {
    console.error('获取设备列表失败:', error)
    ElMessage.error('获取可选设备列表失败')
  }
}

// 2. 获取当前用户的权限信息
const fetchUserPermission = async () => {
  const userId = route.params.id || route.query.id
  if (!userId) {
    ElMessage.error('URL中未找到用户ID！')
    return
  }

  loading.value = true
  try {
    const res = await axios.get(`/permission/getUserById/${userId}`)
    const data = res.data

    form.user_id = data.user_id
    form.name = data.name
    form.number = data.number
    form.valid_start = data.valid_start
    form.valid_end = data.valid_end
    
    // 解析已授权的设备 ID 数组
    if (data.allowed_devices) {
      form.allowed_devices = Array.isArray(data.allowed_devices) 
        ? data.allowed_devices 
        : JSON.parse(data.allowed_devices)
    } else {
      form.allowed_devices = []
    }

    if (data.valid_start && data.valid_end) {
      validRange.value = [data.valid_start, data.valid_end]
    }
  } catch (error) {
    console.error('获取用户权限失败:', error)
    ElMessage.error('获取用户权限数据失败')
  } finally {
    loading.value = false
  }
}

const handleDateChange = (val: [string, string] | []) => {
  if (val && val.length === 2) {
    form.valid_start = val[0]
    form.valid_end = val[1]
  } else {
    form.valid_start = ''
    form.valid_end = ''
  }
}

const handleEnrollFace = (userId: string) => {
  router.push(`/enrollface/${userId}`)
}

const handleSubmit = async () => {
  if (!formRef.value) return
  
  await formRef.value.validate(async (valid) => {
    if (valid) {
      if (!form.valid_start || !form.valid_end) {
        ElMessage.warning('请选择完整的有效期范围')
        return
      }

      submitLoading.value = true
      try {
        await axios.post('/permission/update', form)
        ElMessage.success('用户权限更新成功！')
        router.push('/listAllUserPermission')
      } catch (error: any) {
        ElMessage.error(error.response?.data?.error || '更新失败，请重试')
      } finally {
        submitLoading.value = false
      }
    }
  })
}

const handleReset = () => {
  formRef.value?.resetFields()
  validRange.value = []
  fetchUserPermission()
}

onMounted(() => {
  // 并行请求两个接口，提高效率
  Promise.all([fetchAllDevices(), fetchUserPermission()])
})
</script>

<style scoped>
.permission-container {
  padding: 20px;
  display: flex;
  justify-content: center;
}
.permission-card {
  width: 100%;
  max-width: 700px;
}
.card-header {
  font-weight: bold;
  font-size: 18px;
}
.form-tip {
  font-size: 12px;
  color: #909399;
  line-height: 1.5;
  margin-top: 4px;
}
/* 下拉框选项的自定义布局 */
.device-option-content {
  display: flex;
  align-items: center;
  justify-content: space-between;
}
.device-id-text {
  font-size: 12px;
  color: #909399;
  margin-left: 8px;
  margin-right: 8px;
}
</style>