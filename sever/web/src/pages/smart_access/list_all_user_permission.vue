<template>
  <div class="user-list-container">
    <el-card class="box-card">
      <template #header>
        <div class="card-header">
          <span>已注册住户列表</span>
          <el-button type="primary" @click="fetchUserList" :loading="loading">刷新数据</el-button>
        </div>
      </template>

      <!-- 用户信息表格 -->
      <el-table 
        :data="tableData" 
        v-loading="loading" 
        border 
        stripe 
        style="width: 100%"
        :default-sort="{ prop: 'updated_at', order: 'descending' }"
      >
        <!-- 点击用户名跳转 -->
        <el-table-column label="姓名" min-width="120">
          <template #default="{ row }">
            <el-link type="primary" :underline="false" @click="handleViewDetail(row.user_id)">
              {{ row.name }}
            </el-link>
          </template>
        </el-table-column>

        <el-table-column prop="user_id" label="用户ID" min-width="200" show-overflow-tooltip />
        <el-table-column prop="number" label="学号" width="120" />
        
        <!-- 状态列 -->
        <el-table-column label="状态" width="100" align="center">
          <template #default="{ row }">
            <el-tag :type="row.is_active ? 'success' : 'danger'">
              {{ row.is_active ? '启用' : '禁用' }}
            </el-tag>
          </template>
        </el-table-column>

        <!-- 有效期 -->
        <el-table-column label="有效期" min-width="220">
          <template #default="{ row }">
            <div class="date-range">
              <span>{{ formatDate(row.valid_start) }}</span>
              <span class="date-separator">至</span>
              <span>{{ formatDate(row.valid_end) }}</span>
            </div>
          </template>
        </el-table-column>

        <!-- 允许通行的设备 (JSON处理) -->
        <el-table-column label="允许通行设备" min-width="180">
          <template #default="{ row }">
            <el-tag 
              v-for="(device, index) in parseDevices(row.allowed_devices)" 
              :key="index" 
              size="small" 
              style="margin-right: 5px;"
            >
              {{ device }}
            </el-tag>
            <span v-if="parseDevices(row.allowed_devices).length === 0" class="text-muted">无</span>
          </template>
        </el-table-column>

        <!-- 特征值展示 (二进制转长度展示) -->
        <el-table-column label="特征数据" width="180">
          <template #default="{ row }">
            <div class="feature-info">
              <el-tooltip content="人脸特征字节长度" placement="top">
                <el-tag size="small" type="info">人脸: {{ row.face_feature?.length || 0 }}B</el-tag>
              </el-tooltip>
              <el-tooltip content="指纹特征字节长度" placement="top">
                <el-tag size="small" type="info" style="margin-left: 5px;">指纹: {{ row.fingerprint_feature?.length || 0 }}B</el-tag>
              </el-tooltip>
            </div>
          </template>
        </el-table-column>

        <el-table-column prop="updated_at" label="更新时间" width="180" sortable>
          <template #default="{ row }">
            {{ formatDate(row.updated_at) }}
          </template>
        </el-table-column>

        <el-table-column label="操作" width="120" fixed="right" align="center">
          <template #default="{ row }">
            <el-button link type="primary" @click="handleViewDetail(row.user_id)">
              查看详情
            </el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import axios from "@/axios"

// 1. 定义与后端对应的 TS 类型
interface UserPermission {
  user_id: string
  name: string
  number: string
  face_feature: number[] | null // 对应后端的 []byte，前端接收通常是数字数组或base64
  fingerprint_feature: number[] | null // 对应后端的 []byte
  allowed_devices: string | any[] // 对应后端的 datatypes.JSON
  valid_start: string
  valid_end: string
  is_active: boolean
  updated_at: string
}

// 响应式数据
const tableData = ref<UserPermission[]>([])
const loading = ref(false)

// 3. 获取列表数据
const fetchUserList = async () => {
  loading.value = true
  try {
    // 替换为真实的 axios 请求: const res = await axios.get('/api/user/list')
    const res = await axios.get('/permission/list')
    if (res.status !== 200) {
        ElMessage.error('获取用户列表失败')
        return
    }
    const data = res.data
    tableData.value = data
  } catch (error) {
    ElMessage.error('获取用户列表失败')
    console.error(error)
  } finally {
    loading.value = false
  }
}

// 4. 点击跳转详情 (模拟接口)
const handleViewDetail = (userId: string) => {
  const url = `http://localhost:8080/getUserById/${userId}`
  // 在新标签页打开
  window.open(url, '_blank')
}

// 5. 工具方法：格式化时间
const formatDate = (dateStr: string) => {
  if (!dateStr) return '-'
  const date = new Date(dateStr)
  return date.toLocaleString('zh-CN', { hour12: false })
}

// 6. 工具方法：解析设备 JSON
const parseDevices = (devices: string | any[]) => {
  if (!devices) return []
  if (Array.isArray(devices)) return devices
  try {
    return JSON.parse(devices)
  } catch {
    return []
  }
}

// 页面加载时获取数据
onMounted(() => {
  fetchUserList()
})
</script>

<style scoped>
.user-list-container {
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
.date-range {
  display: flex;
  flex-direction: column;
  font-size: 12px;
  color: #606266;
}
.date-separator {
  color: #909399;
  margin: 2px 0;
}
.feature-info {
  display: flex;
  flex-wrap: wrap;
}
.text-muted {
  color: #909399;
  font-size: 12px;
}
</style>