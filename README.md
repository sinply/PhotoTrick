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
  - Claude 兼容格式 API（DeepSeek/GLM/通义千问等）
  - OpenAI 兼容格式 API
- **多格式导出**: Markdown / CSV / JSON

## 技术栈

- Qt 6.8.3 (MinGW)
- CMake 3.16+
- C++17
- Python 3.x (用于文件转换和本地OCR服务)

## 构建

```bash
# 配置
cmake -B build -G "MinGW Makefiles" \
    -DCMAKE_PREFIX_PATH=D:/Qt/6.8.3/mingw_64

# 编译
cmake --build build

# 部署Qt依赖
D:/Qt/6.8.3/mingw_64/bin/windeployqt.exe build/PhotoTrick.exe
```

## 依赖

### 运行时依赖
- Qt6 (Core, Gui, Widgets, Network, Concurrent)
- Python 3.x (可选，用于格式转换和本地OCR)

### Python依赖（可选）
```bash
pip install paddleocr flask pillow-heif pdf2image python-docx openpyxl
```

## 使用说明

1. 启动应用
2. 点击"添加文件"或"添加文件夹"导入文件
3. 选择处理模式：
   - 表格提取
   - 发票识别
   - 行程单识别
4. 选择OCR后端并配置（如使用在线API需配置API Key）
5. 点击"开始处理"
6. 查看结果并导出

## 项目结构

```
PhotoTrick/
├── CMakeLists.txt          # CMake配置
├── src/
│   ├── main.cpp            # 程序入口
│   ├── MainWindow.*        # 主窗口
│   ├── core/               # 核心模块
│   │   ├── FileManager.*   # 文件管理
│   │   ├── FileConverter.* # 格式转换
│   │   ├── OcrManager.*    # OCR管理
│   │   └── ConfigManager.* # 配置管理
│   ├── models/             # 数据模型
│   ├── processors/         # 处理器
│   ├── classifiers/        # 分类器
│   ├── exporters/          # 导出器
│   ├── ocr/                # OCR客户端
│   └── ui/                 # UI组件
├── scripts/
│   ├── paddle_server.py    # PaddleOCR服务
│   └── file_converter.py   # 文件转换服务
└── thirdparty/             # 第三方库
```

## 许可证

MIT License
