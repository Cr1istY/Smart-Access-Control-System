<template>
  <div class="device-list-container">
    <el-card class="box-card" shadow="hover">
      <!-- 顶部操作区 -->
      <template #header>
        <div class="card-header">
          <span class="header-title">已注册设备列表</span>
          <div class="header-actions">
            <el-button type="primary" :icon="User" @click="router.push('/listAllUserPermission')" :loading="loading">住户管理</el-button>
            <el-button type="primary" :icon="Box" @click="$router.push('/listAllAccessLogs')" :loading="loading">日志查看</el-button>
            <el-button type="primary" :icon="Refresh" @click="fetchDeviceList" :loading="loading">刷新数据</el-button>
          </div>
        </div>
      </template>

      <!-- 设备信息表格 -->
      <el-table 
        :data="tableData" 
        v-loading="loading" 
        border 
        stripe 
        style="width: 100%"
        :default-sort="{ prop: 'created_at', order: 'descending' }"
      >
        <!-- 设备ID (主键) -->
        <el-table-column prop="device_id" label="设备ID" min-width="180" show-overflow-tooltip sortable />

        <!-- 安装位置 -->
        <el-table-column prop="location" label="安装位置" min-width="200" show-overflow-tooltip />

        <!-- IP地址 -->
        <el-table-column prop="ip_address" label="IP地址" width="150" align="center">
          <template #default="{ row }">
            <span class="ip-text">{{ row.ip_address || '-' }}</span>
          </template>
        </el-table-column>

        <!-- 状态 (带颜色标签) -->
        <el-table-column prop="status" label="状态" width="120" align="center" sortable>
          <template #default="{ row }">
            <el-tag :type="getStatusType(row.status)" effect="dark">
              {{ row.status }}
            </el-tag>
          </template>
        </el-table-column>

        <!-- 固件版本 -->
        <el-table-column prop="firmware_version" label="固件版本" width="120" align="center" />

        <!-- 最后心跳时间 -->
        <el-table-column prop="last_heartbeat" label="最后心跳" width="180" sortable>
          <template #default="{ row }">
            {{ formatDateTime(row.last_heartbeat) }}
          </template>
        </el-table-column>

        <!-- 录入时间 -->
        <el-table-column prop="created_at" label="录入时间" width="180" sortable>
          <template #default="{ row }">
            {{ formatDateTime(row.created_at) }}
          </template>
        </el-table-column>

        <!-- 操作列 -->
        <el-table-column label="操作" width="160" fixed="right" align="center">
          <template #default="{ row }">
            <el-button link type="warning" size="small" @click="handleEdit(row.device_id)">编辑</el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { Refresh, User, Box } from '@element-plus/icons-vue'
import { useRouter } from 'vue-router'
import axios from "@/axios"

// 定义与后端 Go Device 结构体对应的 TS 接口
interface Device {
  device_id: string
  location: string
  ip_address: string | null
  status: string
  firmware_version: string
  last_heartbeat: string
  created_at: string
}

// 响应式数据
const tableData = ref<Device[]>([])
const loading = ref(false)
const router = useRouter()

// 获取设备列表数据
const fetchDeviceList = async () => {
  loading.value = true
  try {
    // 替换为真实的后端接口请求
    const res = await axios.get('/device/list')
    tableData.value = res.data
    loading.value = false
  } catch (error) {
    ElMessage.error('获取设备列表失败')
    console.error(error)
    loading.value = false
  }
}

// 状态样式映射
const getStatusType = (status: string) => {
  switch (status.toLowerCase()) {
    case 'online': return 'success'
    case 'offline': return 'info'
    case 'error': return 'danger'
    default: return 'warning'
  }
}

// 工具方法：格式化时间
const formatDateTime = (dateStr: string) => {
  if (!dateStr) return '-'
  const date = new Date(dateStr)
  return date.toLocaleString('zh-CN', { hour12: false })
}


const handleEdit = (deviceId: string) => {
  router.push(`/device/chagelocation/${deviceId}`)
}

// 页面加载时获取数据
onMounted(() => {
  fetchDeviceList()
})
</script>

<style scoped>
.device-list-container {
  padding: 20px;
  background-color: #f5f7fa;
  min-height: 100vh;
}
.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}
.header-title {
  font-weight: bold;
  font-size: 18px;
  color: #303133;
}
.header-actions {
  display: flex;
  gap: 10px;
}
.ip-text {
  font-family: monospace; /* 使用等宽字体展示 IP，更专业 */
  color: #409eff;
  font-weight: 500;
}
</style>