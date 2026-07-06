cmake_minimum_required(VERSION 3.16)
project(PhotoTrick VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt6 path - overridable via -DCMAKE_PREFIX_PATH=... or env var Qt6_DIR
if(NOT DEFINED CMAKE_PREFIX_PATH AND NOT DEFINED ENV{Qt6_DIR})
    set(CMAKE_PREFIX_PATH "D:/Qt/6.8.3/mingw_64")
endif()

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Concurrent
)

qt_standard_project_setup()

# Application sources
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/MainWindow.ui
    src/resources.qrc
)

# Core sources
set(CORE_SOURCES
    src/core/FileManager.cpp
    src/core/FileManager.h
    src/core/FileConverter.cpp
    src/core/FileConverter.h
    src/core/OcrManager.cpp
    src/core/OcrManager.h
    src/core/ConfigManager.cpp
    src/core/ConfigManager.h
)

# Model sources
set(MODEL_SOURCES
    src/models/InvoiceData.h
    src/models/InvoiceData.cpp
    src/models/ItineraryData.h
    src/models/ItineraryData.cpp
    src/models/TableData.h
    src/models/TableData.cpp
    src/models/ExportResult.h
    src/models/ExportResult.cpp
)

# Processor sources
set(PROCESSOR_SOURCES
    src/processors/TableExtractor.cpp
    src/processors/TableExtractor.h
    src/processors/InvoiceRecognizer.cpp
    src/processors/InvoiceRecognizer.h
    src/processors/ItineraryRecognizer.cpp
    src/processors/ItineraryRecognizer.h
)

# Utility sources
set(UTILITY_SOURCES
    src/utils/OcrParser.cpp
    src/utils/OcrParser.h
)

# Classifier sources
set(CLASSIFIER_SOURCES
    src/classifiers/InvoiceClassifier.cpp
    src/classifiers/InvoiceClassifier.h
)

# Exporter sources
set(EXPORTER_SOURCES
    src/exporters/MarkdownExporter.cpp
    src/exporters/MarkdownExporter.h
    src/exporters/CsvExporter.cpp
    src/exporters/CsvExporter.h
    src/exporters/JsonExporter.cpp
    src/exporters/JsonExporter.h
)

# OCR sources
set(OCR_SOURCES
    src/ocr/OcrInterface.h
    src/ocr/OcrServerManager.cpp
    src/ocr/OcrServerManager.h
    src/ocr/PaddleOcr.cpp
    src/ocr/PaddleOcr.h
    src/ocr/ClaudeClient.cpp
    src/ocr/ClaudeClient.h
    src/ocr/OpenAIClient.cpp
    src/ocr/OpenAIClient.h
)

# UI sources
set(UI_SOURCES
    src/ui/FileListView.cpp
    src/ui/FileListView.h
    src/ui/ProcessingPanel.cpp
    src/ui/ProcessingPanel.h
    src/ui/ResultPreview.cpp
    src/ui/ResultPreview.h
    src/ui/CategoryView.cpp
    src/ui/CategoryView.h
    src/ui/SettingsDialog.cpp
    src/ui/SettingsDialog.h
    src/ui/ApiConfigWidget.cpp
    src/ui/ApiConfigWidget.h
)

qt_add_executable(PhotoTrick WIN32
    ${SOURCES}
    ${CORE_SOURCES}
    ${MODEL_SOURCES}
    ${PROCESSOR_SOURCES}
    ${CLASSIFIER_SOURCES}
    ${UTILITY_SOURCES}
    ${EXPORTER_SOURCES}
    ${OCR_SOURCES}
    ${UI_SOURCES}
)

# Windows icon - 暂时禁用：msys2 windres 预处理反斜杠路径时被吞，
# 导致 cc1 找不到 app.rc。阶段三改用 qrc 嵌图标或切换到 MinGW 自带 windres 修复。
# if(WIN32)
#     set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app.rc")
#     target_sources(PhotoTrick PRIVATE ${APP_ICON_RESOURCE_WINDOWS})
# endif()

target_include_directories(PhotoTrick PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
)

target_link_libraries(PhotoTrick PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
)

# Windows deployment
if(WIN32)
    qt_generate_deploy_app_script(
        TARGET PhotoTrick
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()

# ---------- Tests ----------
option(PHOTOTRICK_BUILD_TESTS "Build unit tests" ON)
if(PHOTOTRICK_BUILD_TESTS)
    find_package(Qt6 QUIET COMPONENTS Test)
    if(Qt6Test_FOUND)
        enable_testing()
        qt_add_executable(test_invoice_recognizer
            tests/test_invoice_recognizer.cpp
            src/processors/InvoiceRecognizer.cpp
            src/processors/InvoiceRecognizer.h
            src/core/OcrManager.cpp
            src/core/OcrManager.h
            src/ocr/OcrInterface.h
            src/ocr/PaddleOcr.cpp
            src/ocr/PaddleOcr.h
            src/ocr/ClaudeClient.cpp
            src/ocr/ClaudeClient.h
            src/ocr/OpenAIClient.cpp
            src/ocr/OpenAIClient.h
            src/ocr/OcrServerManager.cpp
            src/ocr/OcrServerManager.h
            src/core/ConfigManager.cpp
            src/core/ConfigManager.h
            src/models/InvoiceData.cpp
            src/models/InvoiceData.h
            src/classifiers/InvoiceClassifier.cpp
            src/classifiers/InvoiceClassifier.h
            src/utils/OcrParser.cpp
            src/utils/OcrParser.h
        )
        target_include_directories(test_invoice_recognizer PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
        )
        target_link_libraries(test_invoice_recognizer PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Network
            Qt6::Concurrent
            Qt6::Test
        )
        add_test(NAME test_invoice_recognizer COMMAND test_invoice_recognizer)
        # 确保 console 输出（非 WIN32 子系统）
        if(WIN32)
            set_target_properties(test_invoice_recognizer PROPERTIES WIN32_EXECUTABLE FALSE)
        endif()
    else()
        message(STATUS "Qt6Test not found, skipping unit tests")
    endif()
endif()
