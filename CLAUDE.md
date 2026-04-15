# PhotoTrick - Claude Code 项目配置

## 项目概述

PhotoTrick 是一个基于 Qt6 的桌面应用，用于发票识别、行程单识别和表格提取。

## 技术栈

- **语言**: C++17
- **框架**: Qt 6.8.3 (Widgets)
- **构建**: CMake + MinGW Makefiles
- **OCR**:
  - 本地: PaddleOCR / RapidOCR (Python Flask 服务)
  - 在线: DeepSeek API, GLM API (OpenAI 兼容格式)

## 代码规范

- 使用 `tr()` 包装所有用户可见字符串
- 信号槽使用新式语法（函数指针）
- 文件编码: UTF-8
- 日志使用 `qDebug()`, `qWarning()`, `qCritical()`

## 项目结构

```
src/
├── main.cpp              # 程序入口，启动画面
├── MainWindow.*          # 主窗口，协调各模块
├── core/
│   ├── ConfigManager.*   # 配置管理（QSettings）
│   ├── FileConverter.*   # 格式转换（PDF/OFD/DOCX/XLSX → 图片）
│   └── OcrManager.*      # OCR 后端管理
├── models/
│   ├── InvoiceData.*     # 发票数据模型
│   ├── TableData.*       # 表格数据模型
│   └── ItineraryData.*   # 行程单数据模型
├── processors/
│   ├── InvoiceRecognizer.*   # 发票识别（正则提取）
│   ├── TableExtractor.*      # 表格提取
│   └── ItineraryRecognizer.* # 行程单识别
├── ocr/
│   ├── ClaudeClient.*    # Claude 格式 API 客户端
│   ├── OpenAIClient.*    # OpenAI 格式 API 客户端
│   └── PaddleOcr.*       # 本地 OCR 客户端
└── ui/
    ├── FileListView.*    # 文件列表视图
    ├── ProcessingPanel.* # 处理面板（模式选择、日志）
    └── ResultPreview.*   # 结果预览（表格显示）
```

## 构建脚本

```bash
# Windows
build.bat

# Linux
./scripts/build.sh
```

脚本功能：
1. 检查编译环境 (CMake, MinGW/g++, Qt)
2. 配置项目（首次）
3. 编译项目
4. 部署 Qt 依赖 (Windows)

### 手动构建

```bash
# 配置 (首次)
cmake -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=D:/Qt/6.8.3/mingw_64

# 编译
cmake --build build -j4

# 部署
D:/Qt/6.8.3/mingw_64/bin/windeployqt.exe build/PhotoTrick.exe
```

## 测试脚本

```bash
# Windows: 完整测试流程
test.bat

# 测试内容:
# 1. 检查 PhotoTrick.exe 是否存在
# 2. 检查 Python 依赖 (Flask, Pillow, PaddleOCR)
# 3. 自动安装缺失依赖
# 4. 启动 PaddleOCR 服务 (端口 5000)
# 5. 测试服务健康状态
# 6. 启动 PhotoTrick 应用
```

## OCR 服务脚本

| 脚本 | 用途 |
|------|------|
| `scripts/setup_ocr_env.sh/.bat` | 自动安装 RapidOCR/PaddleOCR、Flask 等 OCR 依赖 |
| `scripts/start_ocr_server.sh/.bat` | 启动本地 OCR 服务器（支持 start/stop/restart/status） |
| `scripts/stop_ocr_server.sh` | 停止本地 OCR 服务器 |
| `scripts/test_ocr_service.sh/.bat` | 测试 OCR 服务是否正常工作 |
| `scripts/paddle_server.py` | PaddleOCR HTTP 服务主程序 |
| `scripts/file_converter.py` | PDF/OFD/DOCX/XLSX 转图片服务 |

### 使用方式

```bash
# 首次使用，配置OCR环境
# Linux/macOS
./scripts/setup_ocr_env.sh

# Windows
scripts\setup_ocr_env.bat

# 启动OCR服务器
# Linux/macOS
./scripts/start_ocr_server.sh start

# Windows
scripts\start_ocr_server.bat

# 测试OCR服务
# Linux/macOS
./scripts/test_ocr_service.sh

# Windows
scripts\test_ocr_service.bat
```

## 关键实现细节

### 发票识别流程

1. OCR 获取原始文本
2. `InvoiceRecognizer::parseInvoiceText()` 正则提取字段
3. 验证发票号（至少 8 位数字）
4. 检测非发票关键词（登机牌、行程单、火车票等）
5. 金额提取：多候选评分 + 税额/不含税金额校验

### 文件处理流程

1. `FileListView` 管理文件列表（使用完整路径作为 key）
2. `MainWindow::processNextFile()` 依次处理
3. 格式转换（PDF/OFD/DOCX/XLSX → QImage）
4. 调用选择的 OCR 后端
5. 结果汇总到 `ResultPreview`
6. 生成处理报告（非发票/跳过文件）

### 注意事项

- `FileListView::m_filePaths` 使用完整路径作为 key，避免同名文件覆盖
- `MainWindow::currentProcessingFileName()` 返回完整路径用于追踪
- 日志显示仅文件名，内部追踪使用完整路径

## API 配置

### DeepSeek
- Base URL: `https://api.deepseek.com`
- 模型: `deepseek-chat`
- 文档: https://platform.deepseek.com/docs

### GLM (智谱)
- Base URL: `https://open.bigmodel.cn/api/paas/v4`
- 模型: `glm-4v`
- 文档: https://open.bigmodel.cn/dev/api

## 调试

- OCR 原始结果会记录到日志
- 处理报告保存在当前目录: `处理报告_YYYYMMDD_HHmmss.md`
- Invoice debug log: `build/invoice_debug.log`
