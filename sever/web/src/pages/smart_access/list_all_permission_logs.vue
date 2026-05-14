<template>
  <div class="log-list-container">
    <el-card class="box-card" shadow="hover">
      <!-- 顶部操作与筛选区 -->
      <template #header>
        <div class="card-header">
          <span class="header-title">门禁通行日志</span>
          <div class="header-actions">
            <el-button type="primary" :icon="Refresh" @click="fetchLogList" :loading="loading">刷新数据</el-button>
            <el-button type="success" :icon="Download">导出日志</el-button>
            <el-button type="primary" :icon="User" @click="$router.push('/listAllUserPermission')" :loading="loading">住户管理</el-button>
            <el-button type="primary" :icon="Folder" @click="$router.push('/listAllDevice')" :loading="loading">设备管理</el-button>

          </div>
        </div>
      </template>

      <!-- 搜索筛选表单 -->
      <el-form :inline="true" :model="queryParams" class="search-form">
        <el-form-item label="用户ID">
          <el-input v-model="queryParams.user_id" placeholder="请输入用户ID" clearable />
        </el-form-item>
        <el-form-item label="设备ID">
          <el-input v-model="queryParams.device_id" placeholder="请输入设备ID" clearable />
        </el-form-item>
        <el-form-item label="通行结果">
          <el-select v-model="queryParams.result" placeholder="全部" clearable style="width: 120px">
            <el-option label="成功" value="success" />
            <el-option label="失败" value="failure" />
          </el-select>
        </el-form-item>
        <el-form-item label="认证方式">
          <el-select v-model="queryParams.auth_method" placeholder="全部" clearable style="width: 120px">
            <el-option label="人脸识别" value="face" />
            <el-option label="RFID刷卡" value="rfid" />
          </el-select>
        </el-form-item>
        <el-form-item label="通行时间">
          <el-date-picker
            v-model="dateRange"
            type="datetimerange"
            range-separator="至"
            start-placeholder="开始时间"
            end-placeholder="结束时间"
            value-format="YYYY-MM-DD HH:mm:ss"
          />
        </el-form-item>
        <el-form-item>
          <el-button type="primary" :icon="Search" @click="handleSearch">查询</el-button>
          <el-button :icon="Delete" @click="resetQuery">重置</el-button>
        </el-form-item>
      </el-form>

      <!-- 日志信息表格 -->
      <el-table 
        :data="tableData" 
        v-loading="loading" 
        border 
        stripe 
        style="width: 100%"
        height="calc(100vh - 380px)"
      >
        <!-- 日志ID -->
        <el-table-column prop="log_id" label="日志ID" width="100" align="center" sortable />

        <!-- 用户ID -->
        <el-table-column prop="user_id" label="用户ID" min-width="150" show-overflow-tooltip align="center">
          <template #default="{ row }">
            {{ row.user_id || '-' }}
          </template>
        </el-table-column>

        <!-- 设备ID -->
        <el-table-column prop="device_id" label="设备ID" min-width="180" show-overflow-tooltip />

        <!-- 通行方式 -->
        <el-table-column prop="auth_method" label="认证方式" width="120" align="center">
          <template #default="{ row }">
            <el-tag effect="plain">{{ row.auth_method === 'face' ? '人脸识别' : row.auth_method }}</el-tag>
          </template>
        </el-table-column>

        <!-- 通行结果 (带颜色标签) -->
        <el-table-column prop="result" label="通行结果" width="120" align="center">
          <template #default="{ row }">
            <el-tag :type="getResultType(row.result)" effect="dark">
              {{ row.result === 'success' ? '成功' : '失败' }}
            </el-tag>
          </template>
        </el-table-column>

        <!-- 失败原因 -->
        <el-table-column prop="reason" label="失败原因" min-width="200" show-overflow-tooltip>
          <template #default="{ row }">
            <span :class="{ 'text-danger': row.result === 'failure' }">
              {{ row.reason || '-' }}
            </span>
          </template>
        </el-table-column>

        <!-- 抓拍照片 -->
        <el-table-column prop="photo_url" label="抓拍照片" width="100" align="center">
          <template #default="{ row }">
            <el-link v-if="row.photo_url" type="primary" :href="row.photo_url" target="_blank">查看</el-link>
            <span v-else>-</span>
          </template>
        </el-table-column>

        <!-- 通行时间 -->
        <el-table-column prop="access_time" label="通行时间" width="180" sortable>
          <template #default="{ row }">
            {{ formatDateTime(row.access_time) }}
          </template>
        </el-table-column>
      </el-table>

      <!-- 底部分页 -->
      <div class="pagination-container">
        <el-pagination
          v-model:current-page="queryParams.page"
          v-model:page-size="queryParams.page_size"
          :page-sizes="[10, 20, 50, 100]"
          layout="total, sizes, prev, pager, next, jumper"
          :total="total"
          @size-change="fetchLogList"
          @current-change="fetchLogList"
        />
      </div>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { Refresh, Search, Delete, Download, User, Folder } from '@element-plus/icons-vue'
import axios from "@/axios"

// 定义与后端 AccessLog 对应的 TS 接口
interface AccessLog {
  log_id: number
  user_id: string
  device_id: string
  access_time: string
  auth_method: string
  result: string
  photo_url: string
  reason: string
  created_at: string
}

// 响应式数据
const tableData = ref<AccessLog[]>([])
const loading = ref(false)
const total = ref(0)
const dateRange = ref<[string, string] | []>([])

// 查询参数（与后端 LogQueryRequest 对应）
const queryParams = reactive({
  user_id: '',
  device_id: '',
  result: '',
  auth_method: '',
  page: 1,
  page_size: 10
})

// 获取日志列表数据
const fetchLogList = async () => {
  loading.value = true
  try {
    // 处理时间范围参数
    const params = { ...queryParams }
    if (dateRange.value && dateRange.value.length === 2) {
      Object.assign(params, {
        start_time: dateRange.value,
        end_time: dateRange.value
      })
    }

    // 请求后端接口 (与你之前写的 Go Handler 路由对应)
    const res = await axios.get('/accessLog/query', { params })
    
    // 假设后端返回格式为 { code: 0, data: { list: [], total: 0 } }
    if (res.data.code === 0) {
      tableData.value = res.data.data.list
      total.value = res.data.data.total
    } else {
      ElMessage.error(res.data.message || '获取日志失败')
    }
    loading.value = false
  } catch (error) {
    ElMessage.error('网络请求异常，获取日志失败')
    console.error(error)
    loading.value = false
  }
}

// 状态样式映射
const getResultType = (result: string) => {
  return result === 'success' ? 'success' : 'danger'
}

// 工具方法：格式化时间
const formatDateTime = (dateStr: string) => {
  if (!dateStr) return '-'
  const date = new Date(dateStr)
  return date.toLocaleString('zh-CN', { hour12: false })
}

// 搜索与重置
const handleSearch = () => {
  queryParams.page = 1 // 搜索时重置到第一页
  fetchLogList()
}

const resetQuery = () => {
  queryParams.user_id = ''
  queryParams.device_id = ''
  queryParams.result = ''
  queryParams.auth_method = ''
  dateRange.value = []
  handleSearch()
}

// 页面加载时获取数据
onMounted(() => {
  fetchLogList()
})
</script>

<style scoped>
.log-list-container {
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
.search-form {
  margin-bottom: 20px;
}
.pagination-container {
  margin-top: 20px;
  display: flex;
  justify-content: flex-end;
}
.text-danger {
  color: #f56c6c;
}
</style>