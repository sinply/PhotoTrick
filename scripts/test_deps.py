#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Test script to check dependencies"""

try:
    import flask
    print(f"Flask: {flask.__version__}")
except ImportError:
    print("Flask: NOT INSTALLED")

try:
    from PIL import Image
    import PIL
    print(f"Pillow: {PIL.__version__}")
except ImportError:
    print("Pillow: NOT INSTALLED")

try:
    from paddleocr import PaddleOCR
    print("PaddleOCR: OK")
except ImportError:
    print("PaddleOCR: NOT INSTALLED")
