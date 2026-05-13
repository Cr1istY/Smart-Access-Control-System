<template>
  <div class="access-permission-container">
    <el-card header="创建门禁通行许可" class="form-card">
      <el-form
        ref="permissionFormRef"
        :model="formData"
        :rules="formRules"
        label-width="100px"
        class="permission-form"
      >
        <!-- 用户姓名 -->
        <el-form-item label="用户姓名" prop="name">
          <el-input v-model="formData.name" placeholder="请输入用户姓名" />
        </el-form-item>

        <!-- 用户编号 -->
        <el-form-item label="用户编号" prop="number">
          <el-input v-model="formData.number" placeholder="请输入工号或学号" />
        </el-form-item>

        <!-- 新增：允许通行设备 (多选) -->
        <el-form-item label="通行设备" prop="allowed_devices">
          <el-select
            v-model="formData.allowed_devices"
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
              <!-- 自定义选项内容：位置 + ID -->
              <div class="device-option-content">
                <span>{{ device.location }}</span>
                <span class="device-id-text">({{ device.device_id }})</span>
              </div>
            </el-option>
          </el-select>
          <div class="form-tip">支持直接输入设备ID并回车添加</div>
        </el-form-item>

        <!-- 有效期起始 -->
        <el-form-item label="开始时间" prop="valid_start">
          <el-date-picker
            v-model="formData.valid_start"
            type="datetime"
            placeholder="请选择许可生效时间"
            value-format="YYYY-MM-DDTHH:mm:ssZ"
            style="width: 100%"
          />
        </el-form-item>

        <!-- 有效期截止 -->
        <el-form-item label="结束时间" prop="valid_end">
          <el-date-picker
            v-model="formData.valid_end"
            type="datetime"
            placeholder="请选择许可失效时间"
            value-format="YYYY-MM-DDTHH:mm:ssZ"
            style="width: 100%"
          />
        </el-form-item>

        <!-- 操作按钮 -->
        <el-form-item>
          <el-button type="primary" @click="submitForm">创建许可</el-button>
          <el-button @click="resetForm">重置</el-button>
        </el-form-item>
      </el-form>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { reactive, ref, onMounted } from 'vue'
import type { FormInstance, FormRules } from 'element-plus'
import { ElMessage } from 'element-plus'
import axios from "@/axios";
import router from '@/router';

// 定义设备接口的 TS 类型
interface DeviceOption {
  device_id: string
  location: string
}

// 存储从后端获取的所有设备列表
const allDevices = ref<DeviceOption[]>([])

// 更新后的表单数据类型（对应 CreateUserPermission）
interface AccessPermission {
  name: string
  number: string
  allowed_devices: string[] // 新增字段，用于存放选中的设备ID数组
  valid_start: string | null
  valid_end: string | null
}

// 初始化表单数据
const formData = reactive<AccessPermission>({
  name: '',
  number: '',
  allowed_devices: [], // 初始化为空数组
  valid_start: null,
  valid_end: null,
})

const permissionFormRef = ref<FormInstance>()

// 获取设备列表的方法
const fetchAllDevices = async () => {
  try {
    const res = await axios.get('/device/listLocationAndId')
    allDevices.value = res.data || []
  } catch (error) {
    console.error('获取设备列表失败:', error)
    ElMessage.error('获取可选设备列表失败')
  }
}

// 自定义校验规则：确保结束时间不能早于开始时间
const validateValidEnd = (_rule: any, value: any, callback: any) => {
  if (!value) {
    callback(new Error('请选择结束时间'))
  } else if (formData.valid_start && value <= formData.valid_start) {
    callback(new Error('结束时间必须晚于开始时间'))
  } else {
    callback()
  }
}

// 表单校验规则（新增 allowed_devices 的必填校验）
const formRules = reactive<FormRules<AccessPermission>>({
  name: [
    { required: true, message: '请输入用户姓名', trigger: 'blur' },
    { min: 2, max: 20, message: '长度在 2 到 20 个字符', trigger: 'blur' }
  ],
  number: [
    { required: true, message: '请输入用户编号', trigger: 'blur' },
    { pattern: /^\d+$/, message: '编号只能包含数字', trigger: 'blur' }
  ],
  allowed_devices: [
    { type: 'array', required: true, message: '请至少选择一个通行设备', trigger: 'change' }
  ],
  valid_start: [
    { required: true, message: '请选择开始时间', trigger: 'change' }
  ],
  valid_end: [
    { required: true, validator: validateValidEnd, trigger: 'change' }
  ]
})

// 提交表单逻辑
const submitForm = async () => {
  if (!permissionFormRef.value) return
  
  await permissionFormRef.value.validate(async (valid) => {
    if (valid) {
      console.log('准备提交的数据:', JSON.stringify(formData, null, 2))
      
      try {
        const res = await axios.post('/permission/add', formData)
        if (res.status === 200) {
            ElMessage.success('门禁许可创建成功！')
            router.push("/listAllUserPermission")
        }
      } catch (error) {
        ElMessage.error('门禁许可创建失败！')
      }
    } else {
      ElMessage.error('请完善必填信息后再提交')
    }
  })
}

// 重置表单逻辑
const resetForm = () => {
  if (!permissionFormRef.value) return
  permissionFormRef.value.resetFields()
}

// 页面挂载时获取设备列表
onMounted(() => {
  fetchAllDevices()
})
</script>

<style scoped>
.access-permission-container {
  display: flex;
  justify-content: center;
  padding: 40px;
  background-color: #f5f7fa;
  min-height: 100vh;
}

.form-card {
  width: 100%;
  max-width: 500px;
}

.permission-form {
  margin-top: 20px;
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
}
.form-tip {
  font-size: 12px;
  color: #909399;
  line-height: 1.5;
  margin-top: 4px;
}
</style>