#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
OCR Local Server
提供HTTP API接口进行OCR识别
支持RapidOCR（基于ONNX Runtime，无CPU指令集问题）
"""

import os
import base64
import io
import numpy as np
from flask import Flask, request, jsonify
from PIL import Image

try:
    from rapidocr_onnxruntime import RapidOCR
    HAS_RAPIDOCR = True
except ImportError:
    HAS_RAPIDOCR = False
    print("Warning: RapidOCR not installed. Install with: pip install rapidocr-onnxruntime")

app = Flask(__name__)

# 初始化OCR引擎
ocr_engine = None

def init_ocr():
    """初始化OCR引擎"""
    global ocr_engine
    if HAS_RAPIDOCR and ocr_engine is None:
        ocr_engine = RapidOCR()
    return ocr_engine


@app.route('/health', methods=['GET'])
def health_check():
    """健康检查"""
    return jsonify({
        'status': 'ok',
        'paddle_ocr': HAS_RAPIDOCR
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
    if not HAS_RAPIDOCR:
        return jsonify({'error': 'RapidOCR未安装'}), 500

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

        # 转换为numpy数组（RapidOCR需要）
        img_array = np.array(image)

        # 执行OCR
        options = data.get('options', {})
        detect_table = options.get('detect_table', False)

        # RapidOCR返回: (result, elapse)
        # result是列表，每个元素是 [box, text, score]
        result, elapse = ocr_engine(img_array)

        if result is None or len(result) == 0:
            return jsonify({
                'success': True,
                'text': '',
                'boxes': None
            })

        # 解析结果
        boxes = []
        texts = []
        scores = []
        for item in result:
            if len(item) >= 3:
                boxes.append(item[0])  # box坐标
                texts.append(item[1])  # 文本
                scores.append(item[2])  # 置信度

        if detect_table:
            # 表格识别
            tables = extract_tables_rapid(boxes, texts)
            return jsonify({
                'success': True,
                'tables': tables,
                'raw_result': '\n'.join(texts) if texts else ''
            })
        else:
            # 普通OCR
            return jsonify({
                'success': True,
                'text': '\n'.join(texts) if texts else '',
                'boxes': format_ocr_with_boxes_rapid(boxes, texts, scores) if options.get('return_boxes') else None
            })

    except Exception as e:
        import traceback
        traceback.print_exc()
        return jsonify({'error': str(e)}), 500


def format_ocr_with_boxes_rapid(boxes, texts, scores):
    """格式化RapidOCR结果，包含文字框坐标"""
    if not boxes or not texts:
        return []

    result = []
    for i, (box, text) in enumerate(zip(boxes, texts)):
        score = scores[i] if scores and i < len(scores) else 0.0
        result.append({
            'points': box.tolist() if hasattr(box, 'tolist') else box,
            'text': text,
            'confidence': float(score)
        })

    return result


def extract_tables_rapid(boxes, texts):
    """从OCR结果中提取表格结构"""
    if not boxes or not texts:
        return []

    # 简单的表格检测逻辑
    # 基于文字框的位置信息判断行列
    lines = []
    for box, text in zip(boxes, texts):
        # 计算中心点Y坐标
        y_center = (box[0][1] + box[2][1]) / 2
        x_left = box[0][0]
        lines.append({
            'text': text,
            'y': float(y_center),
            'x': float(x_left)
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
    使用OCR结果进行表格结构识别
    """
    if not HAS_RAPIDOCR:
        return jsonify({'error': 'RapidOCR未安装'}), 500

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

        img_array = np.array(image)

        # 执行OCR
        result, elapse = ocr_engine(img_array)

        if result is None or len(result) == 0:
            return jsonify({
                'success': True,
                'tables': []
            })

        # 解析结果
        boxes = []
        texts = []
        for item in result:
            if len(item) >= 2:
                boxes.append(item[0])
                texts.append(item[1])

        tables = extract_tables_rapid(boxes, texts)

        return jsonify({
            'success': True,
            'tables': tables
        })

    except Exception as e:
        import traceback
        traceback.print_exc()
        return jsonify({'error': str(e)}), 500


if __name__ == '__main__':
    print("初始化OCR引擎...")
    init_ocr()

    print("启动OCR服务器，端口: 5000")
    print("API端点:")
    print("  GET  /health - 健康检查")
    print("  POST /ocr    - OCR识别")
    print("  POST /table  - 表格识别")

    app.run(host='127.0.0.1', port=5000, debug=False)
