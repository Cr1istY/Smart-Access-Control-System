<template>
  <div class="access-permission-container">
    <el-card header="创建门禁通行许可" class="form-card">
      <!-- 
        ref: 用于获取表单实例以调用验证方法
        model: 双向绑定的表单数据
        rules: 校验规则
        label-width: 统一设置标签宽度
      -->
      <el-form
        ref="permissionFormRef"
        :model="formData"
        :rules="formRules"
        label-width="100px"
        class="permission-form"
      >
        <!-- 姓名 -->
        <el-form-item label="用户姓名" prop="name">
          <el-input v-model="formData.name" placeholder="请输入用户姓名" />
        </el-form-item>

        <!-- 编号 -->
        <el-form-item label="用户编号" prop="number">
          <el-input v-model="formData.number" placeholder="请输入工号或学号" />
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
import { reactive, ref } from 'vue'
import type { FormInstance, FormRules } from 'element-plus'
import { ElMessage } from 'element-plus'
import axios from "@/axios";
import router from '@/router';

// 1. 定义与后端接口一致的 TS 类型
interface AccessPermission {
  name: string
  number: string
  valid_start: string | null // 初始化为 null，选中后会自动变为格式化后的字符串
  valid_end: string | null
}

// 2. 初始化表单数据
const formData = reactive<AccessPermission>({
  name: '',
  number: '',
  valid_start: null,
  valid_end: null,
})

// 3. 获取表单 DOM 引用
const permissionFormRef = ref<FormInstance>()

// 4. 自定义校验规则：确保结束时间不能早于开始时间
const validateValidEnd = (_rule: any, value: any, callback: any) => {
  if (!value) {
    callback(new Error('请选择结束时间'))
  } else if (formData.valid_start && value <= formData.valid_start) {
    callback(new Error('结束时间必须晚于开始时间'))
  } else {
    callback()
  }
}

// 5. 定义表单校验规则
const formRules = reactive<FormRules<AccessPermission>>({
  name: [
    { required: true, message: '请输入用户姓名', trigger: 'blur' },
    { min: 2, max: 20, message: '长度在 2 到 20 个字符', trigger: 'blur' }
  ],
  number: [
    { required: true, message: '请输入用户编号', trigger: 'blur' },
    { pattern: /^\d+$/, message: '编号只能包含数字', trigger: 'blur' }
  ],
  valid_start: [
    { required: true, message: '请选择开始时间', trigger: 'change' }
  ],
  valid_end: [
    { required: true, validator: validateValidEnd, trigger: 'change' }
  ]
})

// 6. 提交表单逻辑
const submitForm = async () => {
  if (!permissionFormRef.value) return
  
  await permissionFormRef.value.validate(async (valid) => {
    if (valid) {
      // 打印最终要提交给后端的数据
      console.log('准备提交的数据:', JSON.stringify(formData, null, 2))
      
      try {
        const res = await axios.post('/permission/add', formData)
        console.log(res)
        if (res.status === 200) {
            ElMessage.success('门禁许可创建成功！')
            router.push("/listAllUserPermission")
        }

        } catch (error) {
          ElMessage.error('门禁许可创建失败！')
        }
    } else {
      ElMessage.error('请完善必填信息后再提交')
      return
    }
  })
}

// 7. 重置表单逻辑
const resetForm = () => {
  if (!permissionFormRef.value) return
  permissionFormRef.value.resetFields()
}
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
</style>