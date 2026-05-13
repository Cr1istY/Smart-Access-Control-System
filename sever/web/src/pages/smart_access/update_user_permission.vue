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
        <!-- user_id 从 URL 获取，设为只读 -->
        <el-form-item label="用户ID" prop="user_id">
          <el-input v-model="form.user_id" disabled placeholder="请从URL获取用户ID" />
        </el-form-item>

        <el-form-item label="姓名" prop="name">
          <el-input v-model="form.name" placeholder="请输入用户姓名" />
        </el-form-item>

        <el-form-item label="编号" prop="number">
          <el-input v-model="form.number" placeholder="请输入用户编号" />
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
          <el-button type="primary" :loading="loading" @click="handleSubmit">
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
import axios from '@/axios'

const route = useRoute()
const router = useRouter()
const formRef = ref<FormInstance>()
const loading = ref(false)
const submitLoading = ref(false)

// 表单数据绑定，字段对应后端的 CreateUserPermission 结构体
const form = reactive({
  user_id: '',
  name: '',
  number: '',
  valid_start: '',
  valid_end: ''
})

// 时间范围选择器的绑定值
const validRange = ref<[string, string] | []>([])

// 表单校验规则
const rules: FormRules = {
  user_id: [{ required: true, message: '用户ID不能为空', trigger: 'blur' }],
  name: [{ required: true, message: '请输入姓名', trigger: 'blur' }],
  number: [{ required: true, message: '请输入编号', trigger: 'blur' }]
}

const fetchUserPermission = async () => {
  const userId = route.params.id || route.query.id
  if (!userId) {
    ElMessage.error('URL中未找到用户ID！')
    return
  }

  loading.value = true
  try {
    const res = await axios.get(`/permission/getUserById/${userId}`)
    const data = res.data // 假设后端返回的数据结构在 data 字段中

    // 2. 将获取到的信息映射到表单中
    form.user_id = data.user_id
    form.name = data.name
    form.number = data.number
    form.valid_start = data.valid_start
    form.valid_end = data.valid_end

    // 3. 将时间映射到时间范围选择器进行回显
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

// 处理时间范围变化，拆分为 valid_start 和 valid_end
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

// 提交表单
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
        // 调用后端更新接口
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

// 重置表单
const handleReset = () => {
  formRef.value?.resetFields()
  validRange.value = []
  fetchUserPermission()
}

// 页面加载时获取 URL 中的 ID
onMounted(() => {
  fetchUserPermission()
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
</style>