#!/usr/bin/env python3
"""生成备份系统测试文档 Word 文件"""

from docx import Document
from docx.shared import Inches, Pt, Cm, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT
from docx.oxml.ns import qn
from docx.oxml import OxmlElement
import os

def set_cell_shading(cell, color):
    """设置单元格背景色"""
    shading_elm = OxmlElement('w:shd')
    shading_elm.set(qn('w:fill'), color)
    shading_elm.set(qn('w:val'), 'clear')
    cell._tc.get_or_add_tcPr().append(shading_elm)

def add_styled_table(doc, headers, rows, col_widths=None):
    """添加带样式的表格"""
    table = doc.add_table(rows=1 + len(rows), cols=len(headers))
    table.style = 'Table Grid'
    table.alignment = WD_TABLE_ALIGNMENT.CENTER

    # Header row
    for i, header in enumerate(headers):
        cell = table.rows[0].cells[i]
        cell.text = header
        for paragraph in cell.paragraphs:
            paragraph.alignment = WD_ALIGN_PARAGRAPH.CENTER
            for run in paragraph.runs:
                run.bold = True
                run.font.size = Pt(10)
                run.font.color.rgb = RGBColor(255, 255, 255)
        set_cell_shading(cell, '4472C4')

    # Data rows
    for r, row in enumerate(rows):
        for c, val in enumerate(row):
            cell = table.rows[r + 1].cells[c]
            cell.text = str(val)
            for paragraph in cell.paragraphs:
                for run in paragraph.runs:
                    run.font.size = Pt(9)
            if r % 2 == 1:
                set_cell_shading(cell, 'D6E4F0')

    if col_widths:
        for i, width in enumerate(col_widths):
            for row in table.rows:
                row.cells[i].width = Cm(width)

    doc.add_paragraph()  # spacing
    return table

def main():
    doc = Document()

    # ========== 页面设置 ==========
    section = doc.sections[0]
    section.page_width = Cm(21)
    section.page_height = Cm(29.7)
    section.left_margin = Cm(2.0)
    section.right_margin = Cm(2.0)
    section.top_margin = Cm(2.0)
    section.bottom_margin = Cm(2.0)

    # ========== 标题页 ==========
    doc.add_paragraph()
    doc.add_paragraph()
    title = doc.add_paragraph()
    title.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = title.add_run('备份系统测试文档')
    run.bold = True
    run.font.size = Pt(28)
    run.font.color.rgb = RGBColor(0, 51, 102)

    subtitle = doc.add_paragraph()
    subtitle.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = subtitle.add_run('Backup System Test Documentation')
    run.font.size = Pt(16)
    run.font.color.rgb = RGBColor(100, 100, 100)

    doc.add_paragraph()
    info = doc.add_paragraph()
    info.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = info.add_run('测试框架: Catch2\n文档生成日期: 2026-07-13\n测试文件数: 10\n命名空间: backup_system')
    run.font.size = Pt(11)
    run.font.color.rgb = RGBColor(80, 80, 80)

    doc.add_page_break()

    # ========== 目录页 ==========
    doc.add_heading('目录', level=1)
    toc_items = [
        '1. 概述',
        '2. 测试文件总览',
        '3. 详细测试用例',
        '   3.1 test_compression — 压缩算法测试',
        '   3.2 test_encryption — 加密算法测试',
        '   3.3 test_path_utils — 路径工具测试',
        '   3.4 test_filter_spec — 过滤器规格构建测试',
        '   3.5 test_cli_parser — CLI 解析器测试',
        '   3.6 test_metadata — 元数据测试',
        '   3.7 test_registry — 注册表测试',
        '   3.8 test_integration — 集成测试',
        '   3.9 test_stream_processor — 流处理器测试',
        '   3.10 test_filters — 文件过滤器测试',
        '   3.11 test_archive — 归档格式测试',
        '4. 测试统计汇总',
        '5. 测试覆盖矩阵',
    ]
    for item in toc_items:
        p = doc.add_paragraph(item)
        p.paragraph_format.space_after = Pt(2)
        for run in p.runs:
            run.font.size = Pt(11)

    doc.add_page_break()

    # ========== 1. 概述 ==========
    doc.add_heading('1. 概述', level=1)
    doc.add_paragraph(
        '本文档对备份系统（Backup System）的完整测试套件进行系统化分析。'
        '测试套件基于 Catch2 单元测试框架，覆盖了压缩、加密、路径处理、过滤器、'
        'CLI 解析、元数据、注册表、流处理器、归档格式以及端到端集成测试。'
    )
    doc.add_paragraph(
        '测试二进制文件: build/tests/backup_tests\n'
        '测试框架: Catch2 v3\n'
        '编程语言: C++20\n'
        '命名空间: backup_system:: (含子命名空间 app, core, strategy, utils)'
    )

    # ========== 2. 测试文件总览 ==========
    doc.add_heading('2. 测试文件总览', level=1)
    doc.add_paragraph('下表列出了所有测试文件及其测试目标：')

    overview_headers = ['序号', '测试文件', '行数', '测试目标', 'Tag 标签']
    overview_rows = [
        ['1', 'test_compression.cpp', '312', '4种压缩算法(none/RLE/Huffman/LZ77/BWT)的往返正确性', '[compression]'],
        ['2', 'test_encryption.cpp', '252', '3种加密算法(none/XOR/AES-GCM/ChaCha20)的往返正确性与安全性', '[encryption]'],
        ['3', 'test_path_utils.cpp', '161', '路径规范化、序列化/反序列化、路径穿越防护', '[path]'],
        ['4', 'test_filter_spec.cpp', '154', 'FilterSpecBuilder 规格构建与校验', '[spec-builder]'],
        ['5', 'test_cli_parser.cpp', '179', '命令行参数解析、帮助输出、错误处理', '[cli]'],
        ['6', 'test_metadata.cpp', '139', '文件元数据采集、应用与往返一致性', '[metadata]'],
        ['7', 'test_registry.cpp', '172', '编解码器与过滤器注册表的创建与列表功能', '[registry]'],
        ['8', 'test_integration.cpp', '278', '端到端备份→恢复流程（含压缩/加密/过滤组合）', '[integration]'],
        ['9', 'test_stream_processor.cpp', '146', 'PipelineStreamProcessor 管道组合与校验', '[stream]'],
        ['10', 'test_filters.cpp', '313', '4种过滤规则(路径/名称/大小/时间)及组合过滤器', '[filter]'],
        ['11', 'test_archive.cpp', '256', '二进制归档格式读写、头验证、条目类型、完整性检查', '[archive]'],
    ]
    add_styled_table(doc, overview_headers, overview_rows, col_widths=[1.0, 3.5, 1.0, 6.5, 2.5])

    doc.add_page_break()

    # ========== 3. 详细测试用例 ==========
    doc.add_heading('3. 详细测试用例', level=1)

    # --- 3.1 test_compression ---
    doc.add_heading('3.1 test_compression — 压缩算法测试', level=2)
    doc.add_paragraph(
        '测试文件: tests/test_compression.cpp (312 行)\n'
        '测试目标: 验证全部 5 种压缩编解码器（none / RLE / Huffman / LZ77 / BWT）的压缩-解压缩往返正确性，'
        '确保任意输入数据经 compress → decompress 后完全还原。'
    )

    doc.add_heading('3.1.1 NoCompression (none)', level=3)
    comp1_headers = ['TEST_CASE', 'SECTION', '输入/场景', '预期结果']
    comp1_rows = [
        ['NoCompression round-trip preserves data', 'name', '查询编解码器名称', '返回 "none"'],
        ['', 'empty input', '空字符串', '压缩后为空，解压后为空'],
        ['', 'single byte', '"\\x42" (单字节)', '解压后 == "\\x42"'],
        ['', 'all byte values', '0x00~0xFF 全部 256 个字节', '解压后完全一致'],
        ['', 'text data', '"Hello, World! This is a test."', '解压后完全一致'],
        ['', 'large data (64KB)', '1024 次重复 "ABCDEFGH" (8KB 模式)', '解压后完全一致'],
    ]
    add_styled_table(doc, comp1_headers, comp1_rows, col_widths=[4.5, 2.5, 5.0, 3.5])

    doc.add_heading('3.1.2 RLE 压缩', level=3)
    comp2_headers = ['TEST_CASE', 'SECTION', '输入/场景', '预期结果']
    comp2_rows = [
        ['RLE round-trip with varied data', 'empty input', '空字符串', '解压后为空'],
        ['', 'single byte', '"X"', '解压后 == "X"'],
        ['', 'mixed bytes', '"AABBBCCCC" (混合短行程)', '解压后完全一致'],
        ['', 'all unique bytes (no runs)', '0x00~0xFF 全部不同字节', '解压后完全一致'],
        ['', 'all byte values repeated', '256 种字节各重复 3 次 (768 字节)', '解压后完全一致'],
        ['RLE round-trip with repeated bytes', '256 A\'s (2 runs: 255+1)', '256 个 \'A\'', '压缩后 4 字节; 解压正确'],
        ['', '500 X\'s', '500 个 \'X\'', '解压后完全一致'],
        ['', 'single repeated byte', '42 个 \'Z\'', '解压后完全一致'],
        ['RLE empty input produces minimal output', 'compress empty', '空输入流', '压缩输出为空'],
        ['', 'decompress empty', '空压缩流', '解压输出为空'],
    ]
    add_styled_table(doc, comp2_headers, comp2_rows, col_widths=[4.5, 3.0, 5.0, 3.5])

    doc.add_heading('3.1.3 Huffman 压缩', level=3)
    comp3_headers = ['TEST_CASE', 'SECTION', '输入/场景', '预期结果']
    comp3_rows = [
        ['Huffman round-trip with text data', 'simple text', '"hello hello hello world"', '解压后完全一致'],
        ['', 'repeated pattern', '200 次 "The quick brown fox "', '解压后完全一致'],
        ['', 'all byte values', '0x00~0xFF 全部 256 字节', '解压后完全一致'],
        ['', 'large text (100KB)', '10000 次 "ABCDEFGHIJ" (10 字节模式)', '解压后完全一致'],
        ['Huffman round-trip empty input', '-', '空字符串', '解压后为空'],
        ['Huffman round-trip single byte', '-', '"\\xAB" (单字节, 仅头信息)', '解压后 == "\\xAB"'],
    ]
    add_styled_table(doc, comp3_headers, comp3_rows, col_widths=[4.5, 3.0, 5.5, 3.5])

    doc.add_heading('3.1.4 LZ77 压缩', level=3)
    comp4_headers = ['TEST_CASE', 'SECTION', '输入/场景', '预期结果']
    comp4_rows = [
        ['LZ77 round-trip with repeated patterns', 'repeating 10-byte string 100 times', '100 次 "0123456789"', '解压后完全一致'],
        ['', 'all same byte', '4096 个 \'X\'', '解压后完全一致'],
        ['', 'literals only (random-like)', '0x00~0xFF 全部字节', '解压后完全一致'],
        ['', 'short data (< min match)', '"AB" (短于最小匹配长度)', '解压后 == "AB"'],
        ['LZ77 round-trip empty input', '-', '空字符串', '解压后为空'],
        ['LZ77 round-trip data larger than window', '-', '500×"ABCDEFGH" + 200×\'Z\' (>4KB 窗口)', '解压后完全一致'],
    ]
    add_styled_table(doc, comp4_headers, comp4_rows, col_widths=[4.5, 3.5, 4.5, 3.5])

    doc.add_heading('3.1.5 BWT 压缩', level=3)
    comp5_headers = ['TEST_CASE', 'SECTION', '输入/场景', '预期结果']
    comp5_rows = [
        ['BWT round-trip with text data', 'repeated text', '50 次 "banana"', '解压后完全一致'],
        ['', 'english-like text', '100 次 "The quick brown fox..."', '解压后完全一致'],
        ['', 'all byte values', '0x00~0xFF 全部字节', '解压后完全一致'],
        ['', 'short text', '"Hello, BWT!"', '解压后完全一致'],
        ['BWT empty input round-trip', '-', '空字符串', '解压后为空'],
        ['BWT achieves compression', '-', '500 次 "banana " (3500 字节)', '压缩后 < 原始大小; 解压正确'],
    ]
    add_styled_table(doc, comp5_headers, comp5_rows, col_widths=[4.5, 2.5, 5.5, 3.5])

    doc.add_page_break()

    # --- 3.2 test_encryption ---
    doc.add_heading('3.2 test_encryption — 加密算法测试', level=2)
    doc.add_paragraph(
        '测试文件: tests/test_encryption.cpp (252 行)\n'
        '测试目标: 验证全部 4 种加密编解码器（none / XOR-stream / AES-256-GCM / ChaCha20-Poly1305）的'
        '加密-解密往返正确性、密码校验、以及安全性特性（密文≠明文、不同密码不同密文、认证失败检测）。'
    )

    doc.add_heading('3.2.1 NoEncryption (none)', level=3)
    enc1_headers = ['TEST_CASE', 'SECTION', '输入/场景', '预期结果']
    enc1_rows = [
        ['NoEncryption round-trip is identity', 'name and password requirement', '查询名称与密码需求', 'name="none", requires_password=false'],
        ['', 'empty input', '空字符串 + 空密码', '解密后为空'],
        ['', 'data with any password (ignored)', '"secret data" + "ignored"', '解密后 == 原始数据 (密码被忽略)'],
        ['', 'encrypt is identity', '"hello world" + "ignored"', '加密输出 == 原始输入 (恒等)'],
    ]
    add_styled_table(doc, enc1_headers, enc1_rows, col_widths=[4.0, 3.5, 4.5, 4.5])

    doc.add_heading('3.2.2 XOR-stream 加密', level=3)
    enc2_headers = ['TEST_CASE', 'SECTION', '输入/场景', '预期结果']
    enc2_rows = [
        ['XOR-stream encrypt+decrypt round-trip', 'name and password requirement', '查询名称与密码需求', 'name="xor-stream", requires_password=true'],
        ['', 'empty input', '空字符串 + "password"', '解密后为空'],
        ['', 'single byte', '"X" + "secret"', '解密后 == "X"'],
        ['', 'text data', '"Hello, World!..." + "mypassword"', '解密后完全一致'],
        ['', 'large binary data', '100KB 循环字节 (i%256)', '解密后完全一致'],
        ['', 'encrypt != plaintext', '"sensitive data here" + "key123"', '密文 ≠ 明文'],
        ['', 'different passwords → different ciphertexts', '同一明文 + "alice" vs "bob"', '两种密文不相同'],
        ['XOR-stream empty password throws', '-', '空密码调用 encrypt()', '抛出 std::invalid_argument'],
    ]
    add_styled_table(doc, enc2_headers, enc2_rows, col_widths=[4.0, 3.5, 4.5, 4.5])

    doc.add_heading('3.2.3 AES-256-GCM 加密', level=3)
    enc3_headers = ['TEST_CASE', 'SECTION', '输入/场景', '预期结果']
    enc3_rows = [
        ['AES-256-GCM encrypt+decrypt round-trip', 'name and password requirement', '查询名称与密码需求', 'name="aes-256-gcm", requires_password=true'],
        ['', 'empty input', '空字符串 + "strong-password"', '解密后为空'],
        ['', 'single byte', '"X" + "pass123"', '解密后 == "X"'],
        ['', 'text data', '"The quick brown fox..." + "secret123"', '解密后完全一致'],
        ['', 'binary data (all byte values)', '0x00~0xFF 全部 256 字节', '解密后完全一致'],
        ['', 'large data (100KB)', '100KB 伪随机数据', '解密后完全一致'],
        ['', 'random salt/IV → different ciphertext', '同一明文+同一密码调用 2 次', '两次密文不同 (随机盐)'],
        ['AES-256-GCM wrong password detected', '-', '正确密码加密→错误密码解密', '抛出 std::runtime_error (认证失败)'],
    ]
    add_styled_table(doc, enc3_headers, enc3_rows, col_widths=[4.0, 3.5, 4.5, 4.5])

    doc.add_heading('3.2.4 ChaCha20-Poly1305 加密', level=3)
    enc4_headers = ['TEST_CASE', 'SECTION', '输入/场景', '预期结果']
    enc4_rows = [
        ['ChaCha20-Poly1305 encrypt+decrypt round-trip', 'name and password requirement', '查询名称与密码需求', 'name="chacha20-poly1305", requires_password=true'],
        ['', 'empty input', '空字符串 + "password"', '解密后为空'],
        ['', 'single byte', '"X" + "testkey"', '解密后 == "X"'],
        ['', 'text data', '"ChaCha20 is a modern..." + "secure-pw"', '解密后完全一致'],
        ['', 'binary data', '0x00~0xFF 全部字节', '解密后完全一致'],
        ['', 'random salt/nonce → different ciphertext', '同一明文+同一密码调用 2 次', '两次密文不同 (随机 Nonce)'],
        ['ChaCha20-Poly1305 wrong password detected', '-', '正确密码加密→错误密码解密', '抛出 std::runtime_error (认证失败)'],
    ]
    add_styled_table(doc, enc4_headers, enc4_rows, col_widths=[4.0, 3.5, 4.5, 4.5])

    doc.add_page_break()

    # --- 3.3 test_path_utils ---
    doc.add_heading('3.3 test_path_utils — 路径工具测试', level=2)
    doc.add_paragraph(
        '测试文件: tests/test_path_utils.cpp (161 行)\n'
        '测试目标: 验证 PathUtils 的路径规范化（去点、去双斜杠）、绝对路径/路径穿越拒绝、'
        '正斜杠格式转换、以及 to_generic_string ↔ from_generic_string 往返一致性。'
    )

    doc.add_heading('3.3.1 normalize_for_storage', level=3)
    pu1_headers = ['TEST_CASE', 'SECTION', '输入', '预期结果']
    pu1_rows = [
        ['normalize_for_storage handles valid relative paths', 'simple path unchanged', '"foo/bar"', '返回 "foo/bar"'],
        ['', 'dot segments removed', '"foo/./bar"', '返回 "foo/bar"'],
        ['', 'single dot', '"."', '返回 "."'],
        ['', 'trailing slash normalized', '"foo/bar/"', '返回 "foo/bar/"'],
        ['', 'double separator removed', '"foo//bar"', '返回 "foo/bar"'],
        ['', 'simple filename', '"file.txt"', '返回 "file.txt"'],
        ['normalize_for_storage rejects absolute paths', 'Unix absolute path', '"/etc/passwd"', '抛出 std::invalid_argument'],
        ['', 'root path', '"/"', '抛出 std::invalid_argument'],
        ['normalize_for_storage rejects path escape', 'parent directory escape', '"../escape"', '抛出 std::invalid_argument'],
        ['', 'mid-path escape', '"foo/../../etc"', '抛出 std::invalid_argument'],
        ['', 'trailing parent', '"foo/bar/.."', '抛出 std::invalid_argument'],
        ['normalize_for_storage handles empty path', 'empty string', '""', '返回空字符串'],
        ['', 'default constructed path', 'fs::path{}', '返回空字符串'],
    ]
    add_styled_table(doc, pu1_headers, pu1_rows, col_widths=[4.0, 3.0, 3.5, 4.0])

    doc.add_heading('3.3.2 to_generic_string / from_generic_string', level=3)
    pu2_headers = ['TEST_CASE', 'SECTION', '输入', '预期结果']
    pu2_rows = [
        ['to_generic_string converts to forward-slash', 'simple path', '"foo/bar"', '返回 "foo/bar"'],
        ['', 'single component', '"file.txt"', '返回 "file.txt"'],
        ['', 'dot path', '"."', '返回 "."'],
        ['', 'nested path', '"a/b/c/d"', '返回 "a/b/c/d"'],
        ['', 'rejects invalid path', '"/absolute"', '抛出 std::invalid_argument'],
        ['from_generic_string converts to path', 'simple path', '"foo/bar"', '路径 == "foo/bar"'],
        ['', 'empty string', '""', '返回空路径'],
        ['', 'single file', '"readme.md"', '路径 == "readme.md"'],
        ['', 'rejects parent dir escape', '"../escape"', '抛出 std::invalid_argument'],
        ['Round-trip to/from generic string', 'nested relative path', '"projects/backup/src/main.cpp"', '往返后完全一致'],
        ['', 'dot path', '"."', '往返后完全一致'],
    ]
    add_styled_table(doc, pu2_headers, pu2_rows, col_widths=[4.0, 3.0, 3.5, 4.0])

    doc.add_page_break()

    # --- 3.4 test_filter_spec ---
    doc.add_heading('3.4 test_filter_spec — 过滤器规格构建测试', level=2)
    doc.add_paragraph(
        '测试文件: tests/test_filter_spec.cpp (154 行)\n'
        '测试目标: 验证 FilterSpecBuilder 从 FilterCliConfig 构建 FileFilterRuleSpec 列表的功能，'
        '包括 has_filters 检测、各类过滤器的规格生成、参数校验、以及恢复模式下禁止过滤器。'
    )

    doc.add_heading('3.4.1 has_filters 检测', level=3)
    fs1_headers = ['TEST_CASE', 'SECTION', '配置', '预期结果']
    fs1_rows = [
        ['has_filters detects filter configuration', 'empty config returns false', 'FilterCliConfig{} (空)', 'false'],
        ['', 'config with include_paths returns true', 'include_paths = {"*.cpp"}', 'true'],
        ['', 'config with include_names returns true', 'include_names = {"*.h"}', 'true'],
        ['', 'config with min_size returns true', 'min_size_text = "1024"', 'true'],
        ['', 'config with max_size returns true', 'max_size_text = "1048576"', 'true'],
        ['', 'config with modified_after returns true', 'modified_after_text = "2025-01-01T00:00:00"', 'true'],
        ['', 'config with modified_before returns true', 'modified_before_text = "2025-12-31T23:59:59"', 'true'],
    ]
    add_styled_table(doc, fs1_headers, fs1_rows, col_widths=[4.5, 3.5, 4.5, 2.0])

    doc.add_heading('3.4.2 build_specs 规格生成', level=3)
    fs2_headers = ['TEST_CASE', 'SECTION', '配置', '预期结果']
    fs2_rows = [
        ['build_specs generates correct specs', 'empty config', 'FilterCliConfig{} (空)', 'specs.empty() == true'],
        ['', 'path filters', 'include_paths = {"docs/*", "src/*.cpp"}', 'type="path", 2 个值'],
        ['', 'name filters', 'include_names = {"*.log", "*.txt"}', 'type="name", 2 个值'],
        ['', 'size filters', 'min_size="1024", max_size="1048576"', 'type="size", min=1024, max=1048576'],
        ['', 'time filters', 'modified_after_text = "2025-01-01T00:00:00"', 'type="time", modified_after 有值'],
        ['', 'mixed filters', 'include_paths + min_size', '2 个 spec'],
        ['build_specs rejects invalid values', 'non-numeric min_size', 'min_size_text = "abc"', '抛出 std::invalid_argument'],
        ['', 'negative min_size', 'min_size_text = "-100"', '抛出 std::invalid_argument'],
        ['', 'non-numeric max_size', 'max_size_text = "xyz"', '抛出 std::invalid_argument'],
        ['', 'invalid timestamp', 'modified_after_text = "not-a-date"', '抛出 std::invalid_argument'],
        ['', 'filters in restore mode', 'include_paths + is_backup=false', '抛出 std::invalid_argument'],
    ]
    add_styled_table(doc, fs2_headers, fs2_rows, col_widths=[3.5, 3.0, 5.0, 4.0])

    doc.add_page_break()

    # --- 3.5 test_cli_parser ---
    doc.add_heading('3.5 test_cli_parser — CLI 解析器测试', level=2)
    doc.add_paragraph(
        '测试文件: tests/test_cli_parser.cpp (179 行)\n'
        '测试目标: 验证 CliParser 的命令行参数解析能力，包括最小有效参数、完整参数组合、'
        '帮助输出、以及各类错误场景（缺失参数、未知参数、参数值缺失、非法模式、密码-加密不匹配）。'
    )

    doc.add_heading('3.5.1 有效解析', level=3)
    cli1_headers = ['TEST_CASE', 'SECTION', '命令行参数', '预期结果']
    cli1_rows = [
        ['Parse minimal valid backup', '-', '--mode backup --src /data --dest /backup.bks', 'mode="backup", compression="none", encryption="none"'],
        ['Parse minimal valid restore', '-', '--mode restore --src /backup.bks --dest /restore_out', 'mode="restore"'],
        ['Parse all options including filters', '-', '--mode backup ... --compression huffman --encryption aes-256-gcm --password secret --include-path "docs/*" --include-path "src/*.cpp" --include-name "*.log" --min-size 1024 --max-size 1048576 --modified-after 2025-01-01T00:00:00', '所有字段正确解析，filter_config 包含完整配置'],
    ]
    add_styled_table(doc, cli1_headers, cli1_rows, col_widths=[3.5, 1.5, 5.5, 5.0])

    doc.add_heading('3.5.2 帮助与错误处理', level=3)
    cli2_headers = ['TEST_CASE', 'SECTION', '输入', '预期结果']
    cli2_rows = [
        ['--help throws __print_usage__ sentinel', 'long form --help', '--help', '抛出 __print_usage__'],
        ['', 'short form -h', '-h', '抛出 __print_usage__'],
        ['Missing required arguments throws', 'no arguments at all', '(仅程序名)', '抛出 std::invalid_argument'],
        ['', 'missing --dest', '--mode backup --src /a', '抛出 std::invalid_argument'],
        ['', 'missing --mode', '--src /a --dest /b', '抛出 std::invalid_argument'],
        ['Unknown argument throws', '-', '--invalid-flag', '抛出 std::invalid_argument'],
        ['Missing value for argument throws', '-', '--mode (无后续值)', '抛出 std::invalid_argument'],
        ['Invalid mode throws', '-', '--mode sync', '抛出 std::invalid_argument'],
        ['Password without encryption throws', '-', '--password secret (无 --encryption)', '抛出 std::invalid_argument'],
        ['Encryption without password throws', '-', '--encryption aes-256-gcm (无 --password)', '抛出 std::invalid_argument'],
        ['usage returns formatted help string', '-', 'CliParser::usage("backup_cli")', '返回包含 Usage:/--mode/backup/restore 的字符串'],
    ]
    add_styled_table(doc, cli2_headers, cli2_rows, col_widths=[4.0, 3.0, 4.0, 4.5])

    doc.add_page_break()

    # --- 3.6 test_metadata ---
    doc.add_heading('3.6 test_metadata — 元数据测试', level=2)
    doc.add_paragraph(
        '测试文件: tests/test_metadata.cpp (139 行)\n'
        '测试目标: 验证 MetadataUtils 对文件元数据（mode、uid、gid、atime、mtime）的采集（collect）和应用（apply），'
        '以及 collect→apply 往返一致性。使用 TempFileFixture 创建临时文件隔离测试。'
    )

    meta_headers = ['TEST_CASE', 'SECTION', '操作', '预期结果']
    meta_rows = [
        ['Collect metadata from regular file', '-', 'collect(临时文件)', 'mode != 0, mtime > 0, uid > 0, gid > 0'],
        ['Collect metadata from directory', '-', 'collect(临时目录)', 'mode != 0, mtime > 0'],
        ['Apply metadata restores mode', '-', 'collect→修改 mode=0600→apply→collect', 'restored.mode & 07777 == 0600'],
        ['Apply metadata preserves modification time', '-', 'apply(mtime = now-1h)→collect', 'mtime 在设定值 ±1 秒内'],
        ['Metadata round-trip collect then apply', '-', 'collect→修改文件内容→apply(原始)→collect', 'mode 恢复, mtime 在原始值 ±1 秒内'],
    ]
    add_styled_table(doc, meta_headers, meta_rows, col_widths=[4.0, 2.0, 4.5, 5.0])

    # --- 3.7 test_registry ---
    doc.add_heading('3.7 test_registry — 注册表测试', level=2)
    doc.add_paragraph(
        '测试文件: tests/test_registry.cpp (172 行)\n'
        '测试目标: 验证三个注册表的工厂函数和列表函数——压缩编解码器 (5 种)、加密编解码器 (4 种)、'
        '过滤器规则 (4 种)。包括有效/无效名称的创建和完整的枚举列表。'
    )

    doc.add_heading('3.7.1 压缩编解码器注册表', level=3)
    reg1_headers = ['TEST_CASE', 'SECTION', '输入', '预期结果']
    reg1_rows = [
        ['create_compression_codec valid names', '遍历 none/rle/huffman/lz77/bwt', '各名称', 'codec != nullptr, codec->name() == 输入名称'],
        ['create_compression_codec invalid name', 'unknown algorithm', '"gzip"', '抛出 std::invalid_argument'],
        ['', 'empty string', '""', '抛出 std::invalid_argument'],
    ]
    add_styled_table(doc, reg1_headers, reg1_rows, col_widths=[4.0, 3.0, 3.0, 5.5])

    doc.add_heading('3.7.2 加密编解码器注册表', level=3)
    reg2_headers = ['TEST_CASE', 'SECTION', '输入', '预期结果']
    reg2_rows = [
        ['create_encryption_codec valid names', '遍历 none/xor-stream/aes-256-gcm/chacha20-poly1305', '各名称', 'codec != nullptr, codec->name() == 输入名称'],
        ['create_encryption_codec invalid name', 'unknown algorithm', '"rot13"', '抛出 std::invalid_argument'],
        ['', 'empty string', '""', '抛出 std::invalid_argument'],
    ]
    add_styled_table(doc, reg2_headers, reg2_rows, col_widths=[4.0, 3.0, 3.0, 5.5])

    doc.add_heading('3.7.3 列表函数与过滤器规则注册表', level=3)
    reg3_headers = ['TEST_CASE', 'SECTION', '场景', '预期结果']
    reg3_rows = [
        ['list_compression_codecs', '-', '获取全部压缩编解码器名称', '≥5 个, 包含 none/rle/huffman/lz77/bwt'],
        ['list_encryption_codecs', '-', '获取全部加密编解码器名称', '≥4 个, 包含 none/xor-stream/aes-256-gcm/chacha20-poly1305'],
        ['list_filter_rule_types', '-', '获取全部过滤器规则类型', '==4 个, 包含 path/name/size/time'],
        ['create_filter_rule for all types', 'path rule', 'type="path", string_values={"*.cpp"}', 'rule != nullptr, name()="path"'],
        ['', 'name rule', 'type="name", string_values={"*.h"}', 'rule != nullptr, name()="name"'],
        ['', 'size rule', 'type="size", min_size_bytes=1024', 'rule != nullptr, name()="size"'],
        ['', 'time rule', 'type="time", modified_after=now', 'rule != nullptr, name()="time"'],
        ['', 'unknown type throws', 'type="unknown"', '抛出 std::invalid_argument'],
    ]
    add_styled_table(doc, reg3_headers, reg3_rows, col_widths=[4.0, 2.0, 4.5, 5.0])

    doc.add_page_break()

    # --- 3.8 test_integration ---
    doc.add_heading('3.8 test_integration — 集成测试', level=2)
    doc.add_paragraph(
        '测试文件: tests/test_integration.cpp (278 行)\n'
        '测试目标: 端到端验证 BackupEngine 的核心流程——创建目录树→备份到 .bks 归档→恢复到新目录→'
        '验证文件内容和目录结构。覆盖无压缩/加密、Huffman+AES-GCM 组合、BWT+ChaCha20-Poly1305 组合、'
        '路径过滤器排除、以及空目录场景。使用 IntegrationFixture 管理临时目录和文件。'
    )

    int_headers = ['TEST_CASE', '压缩', '加密', '过滤器', '测试数据', '验证点']
    int_rows = [
        ['Backup+restore directory tree (no compression/encryption)', 'none', 'none', 'PassThrough', 'subdir/, empty_dir/, file_a.txt, file_b.txt(10KB), nested/file_c.txt', '4 个文件存在, 3 个目录存在, 文件内容完全一致, 归档文件 > 0 字节'],
        ['Backup+restore with Huffman+AES-GCM', 'huffman', 'aes-256-gcm', 'PassThrough', 'secret.txt, data.bin(5KB)', '2 个文件内容完全一致'],
        ['Backup+restore with BWT+ChaCha20-Poly1305', 'bwt', 'chacha20-poly1305', 'PassThrough', 'report.txt, log.txt(3KB)', '2 个文件内容完全一致'],
        ['Backup with path filter excludes correctly', 'none', 'none', 'NameFilter(*.cpp)', 'include_me.cpp, exclude_me.txt, readme.md, src/main.cpp', '.cpp 文件存在, 目录(src)存在'],
        ['Backup+restore empty directory', 'none', 'none', 'PassThrough', '(空源目录)', '恢复目录存在且为目录类型'],
    ]
    add_styled_table(doc, int_headers, int_rows, col_widths=[3.5, 1.5, 2.0, 2.0, 3.5, 3.5])

    # --- 3.9 test_stream_processor ---
    doc.add_heading('3.9 test_stream_processor — 流处理器测试', level=2)
    doc.add_paragraph(
        '测试文件: tests/test_stream_processor.cpp (146 行)\n'
        '测试目标: 验证 PipelineStreamProcessor 的构造校验（空指针拒绝、密码需求校验）和'
        '4 种管道组合的 backup→restore 往返正确性。'
    )

    doc.add_heading('3.9.1 构造校验', level=3)
    sp1_headers = ['TEST_CASE', 'SECTION', '场景', '预期结果']
    sp1_rows = [
        ['PipelineStreamProcessor rejects null codecs', 'null compression codec', 'compression=nullptr, encryption=valid', '抛出 std::invalid_argument'],
        ['', 'null encryption codec', 'compression=valid, encryption=nullptr', '抛出 std::invalid_argument'],
        ['PipelineStreamProcessor requires password for encryption', 'empty password when encryption requires it', 'xor-stream + 空密码', '抛出 std::invalid_argument'],
        ['', 'password provided works', 'xor-stream + "secret"', '构造成功 (REQUIRE_NOTHROW)'],
    ]
    add_styled_table(doc, sp1_headers, sp1_rows, col_widths=[4.5, 3.0, 4.0, 4.0])

    doc.add_heading('3.9.2 管道往返测试', level=3)
    sp2_headers = ['TEST_CASE', 'SECTION', '管道组合', '测试数据', '预期结果']
    sp2_rows = [
        ['PipelineStreamProcessor backup+restore round-trip', 'huffman only', 'huffman + none', '含重复文本的字符串', 'descriptor 正确; 解压后完全一致'],
        ['', 'xor-stream only', 'none + xor-stream', '"Secret message for pipeline test."', '解密后完全一致'],
        ['', 'huffman + xor-stream chained', 'huffman + xor-stream', '2000A+2000B+2000C', '解密解压后完全一致'],
        ['', 'no-op pipeline (both none)', 'none + none', '"Pass-through data."', '输出 == 输入 (恒等管道)'],
    ]
    add_styled_table(doc, sp2_headers, sp2_rows, col_widths=[4.0, 2.0, 2.5, 3.5, 3.5])

    doc.add_page_break()

    # --- 3.10 test_filters ---
    doc.add_heading('3.10 test_filters — 文件过滤器测试', level=2)
    doc.add_paragraph(
        '测试文件: tests/test_filters.cpp (313 行)\n'
        '测试目标: 验证 4 种过滤器规则（PathPatternRule、NamePatternRule、SizeRangeRule、ModifiedTimeRule）的匹配逻辑，'
        '以及 CompositeFileFilter 的 AND 组合逻辑和 PassThroughFileFilter 的全通过行为。'
        '使用 TempFileFixture 和 NestedTempFixture 创建真实的临时文件系统条目。'
    )

    doc.add_heading('3.10.1 PathPatternRule (路径模式规则)', level=3)
    flt1_headers = ['TEST_CASE', 'SECTION', '模式/场景', '预期结果']
    flt1_rows = [
        ['PathPatternRule matches relative path', 'exact path match with *', '"docs/*" 匹配 "docs/report.txt"', 'matches() == true'],
        ['', 'non-matching path', '"*.log" 不匹配 "docs/report.txt"', 'matches() == false'],
        ['', 'wildcard prefix match', '"docs/*.txt" 匹配 "docs/report.txt"', 'matches() == true'],
        ['PathPatternRule empty patterns throws', '-', 'string_values = {}', '抛出 std::invalid_argument'],
    ]
    add_styled_table(doc, flt1_headers, flt1_rows, col_widths=[4.0, 3.0, 4.5, 4.0])

    doc.add_heading('3.10.2 NamePatternRule (名称模式规则)', level=3)
    flt2_headers = ['TEST_CASE', 'SECTION', '模式/场景', '预期结果']
    flt2_rows = [
        ['NamePatternRule matches filename', 'matches *.log', '"*.log" 匹配 "errors.log"', 'matches() == true'],
        ['', '', '"*.log" 不匹配 "data.txt"', 'matches() == false'],
        ['', 'matches exact name', '"errors.log" 匹配 "errors.log"', 'matches() == true'],
        ['', 'wildcard with ?', '"data.???" 匹配 "data.txt"', 'matches() == true'],
    ]
    add_styled_table(doc, flt2_headers, flt2_rows, col_widths=[4.0, 3.0, 4.5, 4.0])

    doc.add_heading('3.10.3 SizeRangeRule (大小范围规则)', level=3)
    flt3_headers = ['TEST_CASE', 'SECTION', '规则', '测试文件 (大小)', '预期结果']
    flt3_rows = [
        ['SizeRangeRule min-size boundary', '1024 ≤ size', 'min=1024', 'small.bin (1023)', 'matches() == false'],
        ['', '', '', 'exact.bin (1024)', 'matches() == true'],
        ['', '', '', 'large.bin (2048)', 'matches() == true'],
        ['SizeRangeRule max-size boundary', 'size ≤ 1MB', 'max=1048576', 'small.bin (512)', 'matches() == true'],
        ['', '', '', 'exact.bin (1MB)', 'matches() == true'],
        ['', '', '', 'large.bin (1MB+1)', 'matches() == false'],
        ['SizeRangeRule invalid ranges throw', 'no min or max', 'min=nullopt, max=nullopt', '-', '抛出 std::invalid_argument'],
        ['', 'min > max', 'min=200, max=100', '-', '抛出 std::invalid_argument'],
    ]
    add_styled_table(doc, flt3_headers, flt3_rows, col_widths=[3.5, 2.5, 2.0, 3.0, 4.0])

    doc.add_heading('3.10.4 ModifiedTimeRule (修改时间规则)', level=3)
    flt4_headers = ['TEST_CASE', 'SECTION', '规则', '文件时间', '预期结果']
    flt4_rows = [
        ['ModifiedTimeRule after/before', 'modified_after (2024-01-01)', 'after=2024 cutoff', 'old.txt (2023)', 'matches() == false'],
        ['', '', '', 'new.txt (2025)', 'matches() == true'],
        ['', 'modified_before (2024-01-01)', 'before=2024 cutoff', 'old.txt (2023)', 'matches() == true'],
        ['', '', '', 'new.txt (2025)', 'matches() == false'],
    ]
    add_styled_table(doc, flt4_headers, flt4_rows, col_widths=[3.5, 3.0, 3.0, 2.5, 4.0])

    doc.add_heading('3.10.5 CompositeFileFilter & PassThroughFileFilter', level=3)
    flt5_headers = ['TEST_CASE', 'SECTION', '场景', '预期结果']
    flt5_rows = [
        ['CompositeFileFilter ANDs multiple rules', 'both rules match', 'PathRule(src/*) + SizeRule(≥1024), 文件 src/main.cpp (5KB)', 'should_include() == true'],
        ['', 'file matching path but not size', '同上规则, 文件 src/small.cpp (10 字节)', 'should_include() == false'],
        ['PassThroughFileFilter always includes', 'regular file', '任意文件', 'should_include() == true'],
        ['', 'directory', '任意目录', 'should_include() == true'],
    ]
    add_styled_table(doc, flt5_headers, flt5_rows, col_widths=[3.5, 3.5, 5.5, 3.5])

    doc.add_page_break()

    # --- 3.11 test_archive ---
    doc.add_heading('3.11 test_archive — 归档格式测试', level=2)
    doc.add_paragraph(
        '测试文件: tests/test_archive.cpp (256 行)\n'
        '测试目标: 验证 BinaryArchiveStrategy 的二进制归档格式——文件头魔数验证 (BKS1)、'
        '目录/文件/EOA 三种条目类型的读写、完整多条目写入-读取周期、编解码器不匹配检测、'
        '以及缺少根条目的错误检测。'
    )

    arch_headers = ['TEST_CASE', 'SECTION', '操作/场景', '预期结果']
    arch_rows = [
        ['Write and read archive header', '-', '创建 writer→write_directory(".")→finish()', '文件前 4 字节为 "BKS1"; reader 创建成功'],
        ['Archive entry types dir/file/EOA validated', 'root entry', '读取第 1 个条目', 'type=directory, path=".", stored_size=0'],
        ['', 'file entry', '读取第 2 个条目 (test.txt, 12 字节)', 'type=regular_file, path="test.txt", stored=12, original=12'],
        ['', 'EOA entry', '读取第 3 个条目', 'type=end_of_archive, path="", size=0, checksum=0'],
        ['Binary archive full write+read cycle', '-', '写入 root + subdir + 2 文件 + EOA → 读取', 'entry_count=5, dir_count=2, file_count=2'],
        ['Archive codec mismatch on read', '-', '以 huffman 描述符创建 reader 读取 none 归档', '抛出 std::runtime_error'],
        ['Archive finish without root entry throws', '-', 'writer 未写根条目直接 finish()', '抛出 std::runtime_error'],
    ]
    add_styled_table(doc, arch_headers, arch_rows, col_widths=[4.0, 2.0, 5.0, 4.5])

    doc.add_page_break()

    # ========== 4. 测试统计汇总 ==========
    doc.add_heading('4. 测试统计汇总', level=1)

    stats_headers = ['指标', '数值']
    stats_rows = [
        ['测试文件总数', '10'],
        ['测试总行数', '~2362 行'],
        ['TEST_CASE 总数', '54'],
        ['SECTION 总数', '78'],
        ['压缩算法覆盖', '5 种 (none / RLE / Huffman / LZ77 / BWT)'],
        ['加密算法覆盖', '4 种 (none / XOR-stream / AES-256-GCM / ChaCha20-Poly1305)'],
        ['过滤器规则覆盖', '4 种 (path / name / size / time)'],
        ['过滤器组合', '2 种 (CompositeFileFilter / PassThroughFileFilter)'],
        ['归档条目类型覆盖', '3 种 (directory / regular_file / end_of_archive)'],
        ['集成测试场景', '5 种 (基础/压缩+加密组合×2/过滤器/空目录)'],
        ['错误路径覆盖', '30+ 个异常场景 (无效参数、密码错误、路径穿越等)'],
        ['边界条件测试', '空输入、单字节、全部字节值、大数据 (100KB)、窗口边界'],
        ['测试辅助夹具', '6 个 (TempFileFixture / NestedTempFixture / IntegrationFixture / ArchiveTestFixture / ArgvBuilder)'],
    ]
    add_styled_table(doc, stats_headers, stats_rows, col_widths=[5.0, 10.0])

    # ========== 5. 测试覆盖矩阵 ==========
    doc.add_heading('5. 测试覆盖矩阵', level=1)
    doc.add_paragraph('下表展示了各测试文件对主要功能模块的覆盖情况：')

    matrix_headers = ['测试文件', '压缩', '加密', '路径', '过滤器', 'CLI', '元数据', '注册表', '流处理器', '归档', '集成']
    matrix_rows = [
        ['test_compression', '●', '-', '-', '-', '-', '-', '-', '-', '-', '-'],
        ['test_encryption', '-', '●', '-', '-', '-', '-', '-', '-', '-', '-'],
        ['test_path_utils', '-', '-', '●', '-', '-', '-', '-', '-', '-', '-'],
        ['test_filter_spec', '-', '-', '-', '●', '-', '-', '-', '-', '-', '-'],
        ['test_cli_parser', '-', '-', '-', '-', '●', '-', '-', '-', '-', '-'],
        ['test_metadata', '-', '-', '-', '-', '-', '●', '-', '-', '-', '-'],
        ['test_registry', '○', '○', '-', '○', '-', '-', '●', '-', '-', '-'],
        ['test_integration', '○', '○', '-', '○', '-', '-', '-', '○', '○', '●'],
        ['test_stream_processor', '○', '○', '-', '-', '-', '-', '-', '●', '-', '-'],
        ['test_filters', '-', '-', '-', '●', '-', '-', '-', '-', '-', '-'],
        ['test_archive', '-', '-', '-', '-', '-', '-', '-', '-', '●', '-'],
    ]

    table = doc.add_table(rows=1 + len(matrix_rows), cols=len(matrix_headers))
    table.style = 'Table Grid'
    table.alignment = WD_TABLE_ALIGNMENT.CENTER

    for i, header in enumerate(matrix_headers):
        cell = table.rows[0].cells[i]
        cell.text = header
        for paragraph in cell.paragraphs:
            paragraph.alignment = WD_ALIGN_PARAGRAPH.CENTER
            for run in paragraph.runs:
                run.bold = True
                run.font.size = Pt(9)
                run.font.color.rgb = RGBColor(255, 255, 255)
        set_cell_shading(cell, '4472C4')

    for r, row in enumerate(matrix_rows):
        for c, val in enumerate(row):
            cell = table.rows[r + 1].cells[c]
            cell.text = val
            for paragraph in cell.paragraphs:
                paragraph.alignment = WD_ALIGN_PARAGRAPH.CENTER
                for run in paragraph.runs:
                    run.font.size = Pt(9)
                    if val == '●':
                        run.font.color.rgb = RGBColor(0, 128, 0)
                        run.bold = True
                    elif val == '○':
                        run.font.color.rgb = RGBColor(0, 102, 204)
            if r % 2 == 1:
                set_cell_shading(cell, 'D6E4F0')

    doc.add_paragraph()
    p = doc.add_paragraph('图例: ● = 主要覆盖 (该文件的核心测试目标)    ○ = 辅助覆盖 (作为依赖或组合测试涉及)')
    for run in p.runs:
        run.font.size = Pt(9)
        run.font.color.rgb = RGBColor(100, 100, 100)

    # ========== 保存 ==========
    output_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'docs', '备份系统测试文档.docx')
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    doc.save(output_path)
    print(f'测试文档已生成: {output_path}')

if __name__ == '__main__':
    main()
