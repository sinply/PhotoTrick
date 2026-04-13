# PhotoTrick

基于Qt6的图片处理桌面应用，用于发票识别、行程单识别和表格提取。

## 功能特性

- **多格式支持**: JPG/PNG/BMP/GIF/TIFF/WebP/HEIC/PDF/OFD/DOCX/XLSX
- **发票识别**: 自动识别发票类型，提取金额、税额、税率、日期等信息
- **自动分类**: 按交通/住宿/餐饮自动分类发票
- **行程单识别**: 识别机票、火车票等行程单
- **表格提取**: 从图片中提取表格数据，支持多表格合并
- **多OCR后端**:
  - PaddleOCR 本地服务（离线可用）
  - DeepSeek API（推荐）
  - GLM 智谱 API
  - 自定义 OpenAI 兼容 API
- **多格式导出**: Markdown / CSV / JSON
- **批量处理**: 支持批量处理文件并生成处理报告

## 环境搭建

### 1. 系统要求

- Windows 10/11 (推荐) 或 Linux
- Qt 6.8.3 (MinGW 64-bit)
- CMake 3.16+
- C++17 编译器
- Python 3.8+ (可选，用于本地OCR和格式转换)

### 2. 安装 Qt

#### Windows

1. 下载 [Qt Online Installer](https://www.qt.io/download-qt-installer)
2. 安装时选择以下组件：
   - Qt 6.8.3 → MinGW 64-bit
   - CMake (如系统未安装)
   - Ninja (可选)
3. 将 Qt 添加到 PATH：
   ```cmd
   set PATH=%PATH%;C:\Qt\6.8.3\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin
   ```

#### Linux (Ubuntu/Debian)

```bash
# 安装依赖
sudo apt update
sudo apt install -y build-essential cmake git

# 安装 Qt6 (通过 apt 或官方安装器)
sudo apt install -y qt6-base-dev qt6-base-dev-tools
```

### 3. 克隆项目

```bash
git clone https://github.com/yourusername/PhotoTrick.git
cd PhotoTrick
```

### 4. 构建项目

#### Windows (MinGW)

```cmd
# 配置
cmake -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/mingw_64

# 编译
cmake --build build -j4

# 部署Qt依赖
C:\Qt\6.8.3\mingw_64\bin\windeployqt.exe build\PhotoTrick.exe
```

#### Linux

```bash
# 配置
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.3/gcc_64

# 编译
cmake --build build -j$(nproc)
```

### 5. 配置 OCR 后端

#### 方式一：使用在线 API（推荐，无需额外配置）

1. 启动应用后，点击「配置 API」
2. 选择服务商：
   - **DeepSeek** (推荐)
     - 获取 API Key: https://platform.deepseek.com
     - Base URL: `https://api.deepseek.com`
     - 模型: `deepseek-chat`
   - **GLM 智谱**
     - 获取 API Key: https://open.bigmodel.cn
     - Base URL: `https://open.bigmodel.cn/api/paas/v4`
     - 模型: `glm-4v`
3. 输入 API Key 并保存

#### 方式二：使用本地 OCR 服务

1. 安装 Python 依赖：
   ```bash
   # Windows
   pip install paddleocr flask pillow-heif pdf2image python-docx openpyxl

   # 或使用 RapidOCR (更轻量)
   pip install rapidocr-onnxruntime flask pillow numpy
   ```

2. 启动 OCR 服务器：
   ```bash
   # Linux/macOS
   ./scripts/setup_ocr_env.sh    # 首次运行，安装依赖
   ./scripts/start_ocr_server.sh start

   # Windows
   python scripts/paddle_server.py
   ```

3. 在应用中选择「PaddleOCR 本地」作为 OCR 后端

### 6. 运行应用

```bash
# Windows
cd build
PhotoTrick.exe

# Linux
cd build
./PhotoTrick
```

## 使用说明

1. 启动应用
2. 点击「添加文件」或「添加文件夹」导入文件
3. 选择处理模式：
   - **发票识别**: 识别增值税发票，提取金额、税额等
   - **行程单识别**: 识别机票、火车票等
   - **表格提取**: 从图片中提取表格数据
4. 选择 OCR 后端并配置（如使用在线 API 需配置 API Key）
5. 点击「开始处理」
6. 查看结果并导出

## 项目结构

```
PhotoTrick/
├── CMakeLists.txt          # CMake配置
├── README.md               # 项目说明
├── CLAUDE.md               # Claude Code 配置
├── src/
│   ├── main.cpp            # 程序入口
│   ├── MainWindow.*        # 主窗口
│   ├── core/               # 核心模块
│   │   ├── ConfigManager.* # 配置管理
│   │   ├── FileConverter.* # 格式转换 (PDF/OFD/DOCX/XLSX)
│   │   └── OcrManager.*    # OCR管理
│   ├── models/             # 数据模型
│   │   ├── InvoiceData.*   # 发票数据
│   │   ├── TableData.*     # 表格数据
│   │   └── ItineraryData.* # 行程单数据
│   ├── processors/         # 处理器
│   │   ├── InvoiceRecognizer.*   # 发票识别
│   │   ├── TableExtractor.*      # 表格提取
│   │   └── ItineraryRecognizer.* # 行程单识别
│   ├── ocr/                # OCR客户端
│   │   ├── OcrManager.*    # OCR管理器
│   │   ├── ClaudeClient.*  # Claude格式API
│   │   ├── OpenAIClient.*  # OpenAI格式API
│   │   └── PaddleOcr.*     # 本地PaddleOCR
│   └── ui/                 # UI组件
│       ├── FileListView.*  # 文件列表
│       ├── ProcessingPanel.* # 处理面板
│       └── ResultPreview.* # 结果预览
├── scripts/
│   ├── paddle_server.py    # PaddleOCR HTTP服务
│   ├── file_converter.py   # 文件转换服务
│   ├── setup_ocr_env.sh    # OCR环境配置脚本
│   ├── start_ocr_server.sh # 启动OCR服务
│   ├── stop_ocr_server.sh  # 停止OCR服务
│   └── test_ocr_service.sh # 测试OCR服务
├── resources/
│   ├── icons/              # 应用图标
│   └── styles/             # 样式表
└── thirdparty/             # 第三方库
```

## 脚本说明

| 脚本 | 用途 |
|------|------|
| `setup_ocr_env.sh` | 自动安装 RapidOCR、Flask 等 OCR 依赖 |
| `start_ocr_server.sh` | 启动本地 OCR 服务器（支持 start/stop/restart/status） |
| `stop_ocr_server.sh` | 停止本地 OCR 服务器 |
| `test_ocr_service.sh` | 测试 OCR 服务是否正常工作 |
| `paddle_server.py` | PaddleOCR HTTP 服务主程序 |
| `file_converter.py` | PDF/OFD/DOCX/XLSX 转图片服务 |

## 常见问题

### Q: OCR 服务器启动失败？

检查 Python 依赖是否安装完整：
```bash
python -c "from rapidocr_onnxruntime import RapidOCR; print('OK')"
python -c "from flask import Flask; print('OK')"
```

### Q: 无法识别发票金额？

1. 确保使用 DeepSeek 或 GLM API（识别准确率更高）
2. 检查图片是否清晰，建议 300 DPI 以上
3. 查看日志中的 OCR 原始结果

### Q: PDF/OFD 转换失败？

安装额外的 Python 依赖：
```bash
pip install pdf2image pillow-heif python-docx openpyxl

# Windows 还需要安装 poppler
# 下载: https://github.com/oschwartz10612/poppler-windows/releases
```

## 许可证

MIT License
