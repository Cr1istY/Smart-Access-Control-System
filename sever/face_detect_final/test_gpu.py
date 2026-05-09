import onnxruntime as ort

print("🔍 正在检测可用的执行提供程序...")
available_providers = ort.get_available_providers()
print(f"📋 系统检测到的提供程序: {available_providers}")

if 'CUDAExecutionProvider' in available_providers:
    print("🎉 成功！ONNX Runtime 已经准备好使用 GPU。")
else:
    print("❌ 失败！未检测到 CUDAExecutionProvider。")
    print("   可能原因：")
    print("   1. 显卡驱动未安装或版本过低 (请运行 nvidia-smi 检查)")
    print("   2. 安装的 onnxruntime-gpu 版本与你的 CUDA 版本不匹配")