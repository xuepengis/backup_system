# tests 目录单元测试统计

统计口径：
- 将每个 `TEST_CASE` 视为一个单元测试。
- 单元测试中的“测试用例数”按 Catch2 的 `SECTION` 场景数统计；对于没有 `SECTION` 的测试，按 1 个场景计。
- 对于使用循环枚举输入的测试，按循环中的显式输入数量统计场景数。

## 汇总

| 测试文件 | 功能说明 | 单元测试数 | 测试用例数 |
| --- | --- | ---: | ---: |
| `test_archive.cpp` | 归档文件头、条目类型、完整读写链路、编解码器不匹配、缺少根目录项等 | 5 | 5 |
| `test_cli_parser.cpp` | 命令行参数解析、帮助输出、参数校验 | 11 | 16 |
| `test_compression.cpp` | 压缩算法回环验证与边界输入 | 13 | 39 |
| `test_encryption.cpp` | 加密算法回环、空密码、错误密码检测 | 7 | 31 |
| `test_filter_spec.cpp` | 过滤配置判定与过滤规格构建 | 3 | 18 |
| `test_filters.cpp` | 路径/名称/大小/时间过滤规则及组合过滤 | 9 | 15 |
| `test_integration.cpp` | 备份与恢复的端到端集成验证 | 5 | 5 |
| `test_metadata.cpp` | 文件元数据采集与应用 | 5 | 5 |
| `test_path_utils.cpp` | 路径规范化与泛化字符串转换 | 7 | 24 |
| `test_registry.cpp` | 编解码器与过滤规则注册表 | 8 | 21 |
| `test_stream_processor.cpp` | 压缩+加密流水线处理器 | 3 | 8 |

## 逐文件说明

### `test_archive.cpp`

功能备注：测试二进制归档格式的文件头、目录/文件/结束标记、完整写入读取流程，以及归档中声明的编解码器是否匹配。

| 单元测试 | 测试用例数 | 说明 |
| --- | ---: | --- |
| `Write and read archive header` | 1 | 验证归档头魔数写入与读取。 |
| `Archive entry types directory/file/EOA validated` | 1 | 验证目录、普通文件、归档结束条目的类型与字段。 |
| `Binary archive full write+read cycle` | 1 | 验证多目录、多文件的完整写入读取链路。 |
| `Archive codec mismatch on read` | 1 | 验证读取时发现编解码器配置不一致会报错。 |
| `Archive finish without root entry throws` | 1 | 验证缺少根目录条目时结束写入会抛异常。 |

### `test_cli_parser.cpp`

功能备注：测试 CLI 解析器对 backup/restore 模式、可选参数、过滤参数、帮助信息和非法参数的处理。

| 单元测试 | 测试用例数 | 说明 |
| --- | ---: | --- |
| `Parse minimal valid backup` | 1 | 验证最小 backup 命令可正确解析。 |
| `Parse minimal valid restore` | 1 | 验证最小 restore 命令可正确解析。 |
| `Parse all options including filters` | 1 | 验证全部参数与过滤配置都能解析。 |
| `--help throws __print_usage__ sentinel` | 2 | 验证 `--help` 和 `-h` 两种形式。 |
| `Missing required arguments throws` | 3 | 验证缺少参数、缺少 `--dest`、缺少 `--mode` 三种情况。 |
| `Unknown argument throws` | 1 | 验证未知参数会报错。 |
| `Missing value for argument throws` | 1 | 验证参数缺少值会报错。 |
| `Invalid mode throws` | 1 | 验证非法 mode 会报错。 |
| `Password without encryption throws` | 1 | 验证密码单独出现时会报错。 |
| `Encryption without password throws` | 1 | 验证加密参数缺少密码时会报错。 |
| `usage returns formatted help string` | 1 | 验证帮助文本包含关键字段。 |

### `test_compression.cpp`

功能备注：测试 `none`、`rle`、`huffman`、`lz77`、`bwt` 等压缩算法的压缩/解压回环、边界输入和压缩效果。

| 单元测试 | 测试用例数 | 说明 |
| --- | ---: | --- |
| `NoCompression round-trip preserves data` | 6 | 验证空输入、单字节、全字节值、文本、长文本等场景。 |
| `RLE round-trip with varied data` | 5 | 验证 RLE 在不同数据形态下的回环正确性。 |
| `RLE round-trip with repeated bytes` | 3 | 验证重复字节、长重复串等场景。 |
| `RLE empty input produces minimal output` | 1 | 验证空输入输出最小化。 |
| `Huffman round-trip with text data` | 4 | 验证 Huffman 在多种文本和字节数据上的回环。 |
| `Huffman round-trip empty input` | 1 | 验证空输入。 |
| `Huffman round-trip single byte input` | 1 | 验证单字节特殊情况。 |
| `LZ77 round-trip with repeated patterns` | 4 | 验证重复模式、全相同、随机样式、短数据。 |
| `LZ77 round-trip empty input` | 1 | 验证空输入。 |
| `LZ77 round-trip data larger than window` | 1 | 验证超过滑动窗口的数据。 |
| `BWT round-trip with text data` | 4 | 验证 BWT 在多类文本与字节流上的回环。 |
| `BWT empty input round-trip` | 1 | 验证空输入。 |
| `BWT achieves compression on repetitive text` | 1 | 验证重复文本的压缩率与回环。 |

### `test_encryption.cpp`

功能备注：测试 `none`、`xor-stream`、`aes-256-gcm`、`chacha20-poly1305` 等加密算法的回环、随机化输出和错误密码检测。

| 单元测试 | 测试用例数 | 说明 |
| --- | ---: | --- |
| `NoEncryption round-trip is identity` | 4 | 验证无加密时输入保持不变。 |
| `XOR-stream encrypt+decrypt round-trip` | 7 | 验证空输入、单字节、文本、大二进制、密文不同性等场景。 |
| `XOR-stream empty password throws` | 1 | 验证空密码报错。 |
| `AES-256-GCM encrypt+decrypt round-trip` | 7 | 验证 AES-GCM 的多种输入和随机盐/IV 行为。 |
| `AES-256-GCM wrong password detected` | 1 | 验证错误密码会在解密时报错。 |
| `ChaCha20-Poly1305 encrypt+decrypt round-trip` | 6 | 验证 ChaCha20-Poly1305 的多种输入和随机 nonce 行为。 |
| `ChaCha20-Poly1305 wrong password detected` | 1 | 验证错误密码会在解密时报错。 |

### `test_filter_spec.cpp`

功能备注：测试过滤配置是否被识别为有过滤条件，以及如何把 CLI 配置构建成内部过滤规格，并校验非法值。

| 单元测试 | 测试用例数 | 说明 |
| --- | ---: | --- |
| `has_filters detects filter configuration` | 7 | 验证空配置和各类单独过滤条件。 |
| `build_specs generates correct specs from config` | 6 | 验证路径、名称、大小、时间与混合过滤规格的构建。 |
| `build_specs rejects invalid filter values` | 5 | 验证非法数字、非法时间与 restore 模式禁用过滤。 |

### `test_filters.cpp`

功能备注：测试路径、名称、大小、时间过滤规则，以及组合过滤器和透传过滤器。

| 单元测试 | 测试用例数 | 说明 |
| --- | ---: | --- |
| `PathPatternRule matches relative path` | 3 | 验证路径通配符匹配。 |
| `PathPatternRule empty patterns throws` | 1 | 验证空路径规则会报错。 |
| `NamePatternRule matches filename` | 3 | 验证文件名通配符匹配。 |
| `SizeRangeRule min-size boundary` | 1 | 验证最小大小边界。 |
| `SizeRangeRule max-size boundary` | 1 | 验证最大大小边界。 |
| `SizeRangeRule invalid ranges throw` | 2 | 验证无范围和 min > max 两种非法情况。 |
| `ModifiedTimeRule after/before` | 2 | 验证修改时间上下界。 |
| `CompositeFileFilter ANDs multiple rules` | 3 | 验证多规则 AND 组合行为。 |
| `PassThroughFileFilter always includes` | 2 | 验证普通文件和目录都通过。 |

### `test_integration.cpp`

功能备注：测试备份引擎的端到端流程，包括目录树备份恢复、压缩加密组合、过滤器生效和空目录场景。

| 单元测试 | 测试用例数 | 说明 |
| --- | ---: | --- |
| `Backup+restore directory tree (no compression/encryption)` | 1 | 验证无压缩无加密时整棵目录树可正确备份恢复。 |
| `Backup+restore with Huffman+AES-GCM` | 1 | 验证 Huffman + AES-GCM 的端到端恢复。 |
| `Backup+restore with BWT+ChaCha20-Poly1305` | 1 | 验证 BWT + ChaCha20-Poly1305 的端到端恢复。 |
| `Backup with path filter excludes correctly` | 1 | 验证过滤器只保留符合规则的文件。 |
| `Backup+restore empty directory` | 1 | 验证空目录也能正确归档和恢复。 |

### `test_metadata.cpp`

功能备注：测试文件元数据采集与回写，重点是权限位和修改时间。

| 单元测试 | 测试用例数 | 说明 |
| --- | ---: | --- |
| `Collect metadata from regular file` | 1 | 验证普通文件元数据可采集。 |
| `Collect metadata from directory` | 1 | 验证目录元数据可采集。 |
| `Apply metadata restores mode` | 1 | 验证权限位可恢复。 |
| `Apply metadata preserves modification time` | 1 | 验证修改时间可回写。 |
| `Metadata round-trip collect then apply` | 1 | 验证采集后再应用可近似恢复原元数据。 |

### `test_path_utils.cpp`

功能备注：测试路径工具对存储路径的规范化、泛化字符串转换以及非法路径拦截。

| 单元测试 | 测试用例数 | 说明 |
| --- | ---: | --- |
| `normalize_for_storage handles valid relative paths` | 6 | 验证合法相对路径的规范化。 |
| `normalize_for_storage rejects absolute paths` | 2 | 验证绝对路径会被拒绝。 |
| `normalize_for_storage rejects path escape` | 3 | 验证 `..` 越界路径会被拒绝。 |
| `normalize_for_storage handles empty path` | 2 | 验证空路径处理。 |
| `to_generic_string converts to forward-slash format` | 5 | 验证到通用字符串的转换与非法路径拒绝。 |
| `from_generic_string converts generic string to path` | 4 | 验证从通用字符串回转为路径。 |
| `to_generic_string and from_generic_string round-trip` | 2 | 验证双向往返转换。 |

### `test_registry.cpp`

功能备注：测试压缩/加密 codec 注册表、过滤规则注册表与枚举接口。

| 单元测试 | 测试用例数 | 说明 |
| --- | ---: | --- |
| `create_compression_codec valid names` | 5 | 验证 `none/rle/huffman/lz77/bwt` 五种压缩器都可创建。 |
| `create_compression_codec invalid name throws` | 2 | 验证未知和空名称会报错。 |
| `create_encryption_codec valid names` | 4 | 验证 `none/xor-stream/aes-256-gcm/chacha20-poly1305` 四种加密器都可创建。 |
| `create_encryption_codec invalid name throws` | 2 | 验证未知和空名称会报错。 |
| `list_compression_codecs returns all registered names` | 1 | 验证压缩器枚举包含所有注册项。 |
| `list_encryption_codecs returns all registered names` | 1 | 验证加密器枚举包含所有注册项。 |
| `list_filter_rule_types returns all registered types` | 1 | 验证过滤规则类型枚举完整。 |
| `create_filter_rule for all registered types` | 5 | 验证 path/name/size/time 与未知类型。 |

### `test_stream_processor.cpp`

功能备注：测试压缩+加密流水线处理器的构造参数校验，以及备份/恢复链路。

| 单元测试 | 测试用例数 | 说明 |
| --- | ---: | --- |
| `PipelineStreamProcessor rejects null codecs` | 2 | 验证空压缩器和空加密器都会报错。 |
| `PipelineStreamProcessor requires password for encryption` | 2 | 验证加密算法需要密码时的构造校验。 |
| `PipelineStreamProcessor backup+restore round-trip` | 4 | 验证仅压缩、仅加密、压缩+加密、完全透传四种链路。 |

