#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
PaddleOCR Local Server
提供HTTP API接口进行OCR识别
"""

import os
import base64
import io
from flask import Flask, request, jsonify
from PIL import Image

try:
    from paddleocr import PaddleOCR
    HAS_PADDLE = True
except ImportError:
    HAS_PADDLE = False
    print("Warning: PaddleOCR not installed. Install with: pip install paddleocr")

app = Flask(__name__)

# 初始化OCR引擎
ocr_engine = None

def init_ocr():
    """初始化PaddleOCR引擎"""
    global ocr_engine
    if HAS_PADDLE and ocr_engine is None:
        # use_angle_cls=True 用于识别旋转文字
        # lang='ch' 支持中英文
        ocr_engine = PaddleOCR(use_angle_cls=True, lang='ch', show_log=False)
    return ocr_engine


@app.route('/health', methods=['GET'])
def health_check():
    """健康检查"""
    return jsonify({
        'status': 'ok',
        'paddle_ocr': HAS_PADDLE
    })


@app.route('/ocr', methods=['POST'])
def ocr_recognize():
    """
    OCR识别接口
    请求格式:
    {
        "image": "base64编码的图片数据",
        "options": {
            "detect_table": false,  # 是否检测表格
            "return_boxes": false   # 是否返回文字框坐标
        }
    }
    """
    if not HAS_PADDLE:
        return jsonify({'error': 'PaddleOCR未安装'}), 500

    try:
        data = request.get_json()
        if not data or 'image' not in data:
            return jsonify({'error': '缺少图片数据'}), 400

        # 解码图片
        image_data = data['image']
        if image_data.startswith('data:image'):
            # 移除data:image/png;base64,前缀
            image_data = image_data.split(',', 1)[1]

        image_bytes = base64.b64decode(image_data)
        image = Image.open(io.BytesIO(image_bytes))

        # 转换为RGB模式（处理RGBA等格式）
        if image.mode != 'RGB':
            image = image.convert('RGB')

        # 保存为临时文件（PaddleOCR需要文件路径）
        import tempfile
        with tempfile.NamedTemporaryFile(suffix='.jpg', delete=False) as tmp:
            image.save(tmp.name, 'JPEG')
            tmp_path = tmp.name

        try:
            # 执行OCR
            options = data.get('options', {})
            detect_table = options.get('detect_table', False)

            if detect_table:
                # 表格识别
                result = ocr_engine.ocr(tmp_path, cls=True)
                tables = extract_tables(result)
                return jsonify({
                    'success': True,
                    'tables': tables,
                    'raw_result': format_ocr_result(result)
                })
            else:
                # 普通OCR
                result = ocr_engine.ocr(tmp_path, cls=True)
                return jsonify({
                    'success': True,
                    'text': format_ocr_result(result),
                    'boxes': format_ocr_with_boxes(result) if options.get('return_boxes') else None
                })
        finally:
            # 清理临时文件
            os.unlink(tmp_path)

    except Exception as e:
        return jsonify({'error': str(e)}), 500


def format_ocr_result(result):
    """格式化OCR结果为纯文本"""
    if not result or not result[0]:
        return ""

    lines = []
    for line in result[0]:
        if line and len(line) >= 2:
            text = line[1][0]  # 文本内容
            lines.append(text)

    return '\n'.join(lines)


def format_ocr_with_boxes(result):
    """格式化OCR结果，包含文字框坐标"""
    if not result or not result[0]:
        return []

    boxes = []
    for line in result[0]:
        if line and len(line) >= 2:
            box = {
                'points': line[0],  # 四个角坐标
                'text': line[1][0],  # 文本
                'confidence': line[1][1]  # 置信度
            }
            boxes.append(box)

    return boxes


def extract_tables(result):
    """从OCR结果中提取表格结构"""
    if not result or not result[0]:
        return []

    # 简单的表格检测逻辑
    # 基于文字框的位置信息判断行列
    lines = []
    for line in result[0]:
        if line and len(line) >= 2:
            points = line[0]
            text = line[1][0]
            # 计算中心点Y坐标
            y_center = (points[0][1] + points[2][1]) / 2
            x_left = points[0][0]
            lines.append({
                'text': text,
                'y': y_center,
                'x': x_left
            })

    if not lines:
        return []

    # 按Y坐标分组（相近的Y为同一行）
    lines.sort(key=lambda x: x['y'])
    rows = []
    current_row = []
    last_y = None
    y_threshold = 10  # Y坐标差异阈值

    for line in lines:
        if last_y is None or abs(line['y'] - last_y) < y_threshold:
            current_row.append(line)
        else:
            if current_row:
                # 按X坐标排序
                current_row.sort(key=lambda x: x['x'])
                rows.append([item['text'] for item in current_row])
            current_row = [line]
        last_y = line['y']

    if current_row:
        current_row.sort(key=lambda x: x['x'])
        rows.append([item['text'] for item in current_row])

    if len(rows) < 2:
        return []

    # 返回表格数据
    return [{
        'headers': rows[0] if rows else [],
        'rows': rows[1:] if len(rows) > 1 else []
    }]


@app.route('/table', methods=['POST'])
def table_recognize():
    """
    表格识别专用接口
    使用PP-Structure进行表格结构识别
    """
    if not HAS_PADDLE:
        return jsonify({'error': 'PaddleOCR未安装'}), 500

    try:
        data = request.get_json()
        if not data or 'image' not in data:
            return jsonify({'error': '缺少图片数据'}), 400

        # 解码图片
        image_data = data['image']
        if image_data.startswith('data:image'):
            image_data = image_data.split(',', 1)[1]

        image_bytes = base64.b64decode(image_data)
        image = Image.open(io.BytesIO(image_bytes))

        if image.mode != 'RGB':
            image = image.convert('RGB')

        # 保存临时文件
        import tempfile
        with tempfile.NamedTemporaryFile(suffix='.jpg', delete=False) as tmp:
            image.save(tmp.name, 'JPEG')
            tmp_path = tmp.name

        try:
            # 使用PP-Structure进行表格识别
            # 注意：需要安装 paddleocr[full] 或单独安装 table-engine
            from paddleocr import PPStructure

            table_engine = PPStructure(show_log=False, use_angle_cls=True, lang='ch')
            result = table_engine(tmp_path)

            tables = []
            for region in result:
                if region['type'] == 'table':
                    # 解析HTML表格
                    html = region.get('res', {}).get('html', '')
                    if html:
                        tables.append(parse_html_table(html))

            return jsonify({
                'success': True,
                'tables': tables
            })

        finally:
            os.unlink(tmp_path)

    except Exception as e:
        return jsonify({'error': str(e)}), 500


def parse_html_table(html):
    """解析HTML表格为结构化数据"""
    from bs4 import BeautifulSoup

    soup = BeautifulSoup(html, 'html.parser')
    table = soup.find('table')

    if not table:
        return {'headers': [], 'rows': []}

    rows = []
    for tr in table.find_all('tr'):
        cells = []
        for cell in tr.find_all(['th', 'td']):
            cells.append(cell.get_text(strip=True))
        if cells:
            rows.append(cells)

    if not rows:
        return {'headers': [], 'rows': []}

    return {
        'headers': rows[0],
        'rows': rows[1:]
    }


if __name__ == '__main__':
    print("初始化PaddleOCR引擎...")
    init_ocr()

    print("启动OCR服务器，端口: 5000")
    print("API端点:")
    print("  GET  /health - 健康检查")
    print("  POST /ocr    - OCR识别")
    print("  POST /table  - 表格识别")

    app.run(host='127.0.0.1', port=5000, debug=False)
