# backup_system

一个运行在 Ubuntu 22.04 Server / Linux 命令行环境下的 C++20 数据备份原型系统。

当前版本聚焦于一个可扩展的 MVP：
- 递归备份目录树
- 从单文件归档中还原目录树
- 保存并恢复基础元数据
- 支持可扩展的压缩/解压与加密/解密
- 支持可扩展的备份过滤规则

整个项目坚持两个原则：
- 核心实现不依赖第三方库，仅使用 C++ 标准库和 Linux 原生系统调用
- 架构优先考虑后续扩展，避免把归档格式、过滤规则、压缩算法、加密算法写死在引擎里

---

## 1. 项目目标

本项目用于实现一个命令行备份系统原型，核心目标是：

1. 将一个源目录树递归扫描后写入归档文件。
2. 从归档文件完整恢复目录树。
3. 为后续扩展能力预留稳定接口，包括：
   - 更多过滤规则
   - 更多压缩算法
   - 更多加密算法
   - 更复杂的归档格式
   - 特殊文件支持
   - 定时与实时备份

---

## 2. 当前已实现功能

### 2.1 备份与还原

- 支持递归遍历普通目录树
- 支持备份普通文件和目录
- 支持从单文件归档还原目录结构和文件内容

### 2.2 元数据支持

当前已支持以下基础元数据：
- 权限位 `mode`
- 文件属主 `uid`
- 文件属组 `gid`
- 访问时间 `atime`
- 修改时间 `mtime`

说明：
- 目录元数据在还原完成后倒序应用，避免目录时间在创建子文件过程中被再次改写。
- `chown` 在无权限场景下允许 `EPERM`，不会导致普通用户恢复失败。

### 2.3 过滤功能

当前已实现 4 类备份过滤：
- 路径过滤 `--include-path`
- 文件名过滤 `--include-name`
- 尺寸过滤 `--min-size` / `--max-size`
- 时间过滤 `--modified-after` / `--modified-before`

规则语义：
- 多个不同类型规则之间采用 AND 关系
- 同一类型中多个模式采用 OR 关系
- 过滤只作用于 `backup` 模式，不作用于 `restore`

### 2.4 压缩 / 解压

当前已实现：
- `none`
- `rle`

说明：
- `rle` 是最基础的游程编码实现，主要用于打通可扩展压缩接口
- 它不是高压缩率算法，更多是原型阶段的结构验证

### 2.5 加密 / 解密

当前已实现：
- `none`
- `xor-stream`

说明：
- `xor-stream` 是手写轻量原型算法，只用于打通加密接口与归档链路
- 它不是强密码学安全方案，不能替代成熟加密算法

### 2.6 归档格式

当前归档为单文件二进制归档，具备：
- 文件头 magic
- 版本号
- flags
- 压缩算法名
- 加密算法名
- entry 类型约束
- 归档载荷校验
- 原始内容校验

---

## 3. 当前未实现或仅部分实现的内容

以下能力尚未完成，或仅完成接口预留：
- 特殊文件支持：管道、符号链接、设备文件等
- GUI 图形界面
- 定时备份
- 实时备份
- 强安全加密算法
- 高压缩率压缩算法
- 增量备份 / 去重 / 快照

---

## 4. 目录结构说明

当前项目核心目录如下：

```text
backup_system/
├── CMakeLists.txt
├── README.md
├── main.cpp
├── include/
│   ├── app/
│   │   └── cli_parser.hpp
│   ├── core/
│   │   └── backup_engine.hpp
│   ├── strategy/
│   │   ├── codec_registry.hpp
│   │   ├── filter_registry.hpp
│   │   ├── filter_spec_builder.hpp
│   │   ├── iarchive_strategy.hpp
│   │   ├── ifile_filter.hpp
│   │   └── istream_processor.hpp
│   └── utils/
│       ├── logger.hpp
│       ├── metadata_utils.hpp
│       └── path_utils.hpp
└── src/
    ├── app/
    │   └── cli_parser.cpp
    ├── core/
    │   └── backup_engine.cpp
    ├── strategy/
    │   ├── archive_strategy.cpp
    │   ├── codec_registry.cpp
    │   ├── file_filter.cpp
    │   ├── filter_registry.cpp
    │   ├── filter_spec_builder.cpp
    │   └── stream_processor.cpp
    └── utils/
        ├── logger.cpp
        ├── metadata_utils.cpp
        └── path_utils.cpp
```

### 4.1 顶层文件

#### `CMakeLists.txt`

作用：
- 使用现代 CMake 构建工程
- 指定 C++20
- 开启严格编译选项：
  - `-Wall`
  - `-Wextra`
  - `-Wpedantic`

#### `main.cpp`

作用：
- 当前入口文件
- 只负责高层调度：
  - 解析 CLI
  - 组装过滤器
  - 组装压缩/加密处理器
  - 组装备份引擎
  - 根据模式调用 `backup()` / `restore()`

---

## 5. 各模块作用与实现原理

### 5.1 `core/backup_engine`

核心类：
- `backup_system::core::BackupEngine`

职责：
- 不关心具体过滤规则
- 不关心具体归档格式布局
- 不关心具体压缩/加密算法
- 只负责任务编排

备份流程：

1. 校验源目录与目标归档路径
2. 创建归档 writer
3. 写入根目录 entry
4. 递归遍历目录树
5. 对每个 entry 调用过滤器
6. 目录写入目录 entry
7. 普通文件：
   - 计算原始文件大小
   - 计算原始内容 checksum
   - 将文件流交给 stream processor
   - 将处理后的载荷写入归档
8. 写入 end-of-archive 标记

还原流程：

1. 校验归档路径
2. 创建归档 reader
3. 顺序读取 entry
4. 遇到目录 entry：
   - 创建目录
   - 延迟记录目录元数据
5. 遇到文件 entry：
   - 打开目标文件
   - 通过 stream processor 恢复原文
   - 校验还原后文件大小
   - 校验还原后内容 checksum
   - 应用文件元数据
6. 所有文件恢复后，倒序应用目录元数据

设计意义：
- 引擎只依赖抽象接口，后续替换任何归档策略或算法时不需要重写主流程

### 5.2 `strategy/iarchive_strategy` 与 `strategy/archive_strategy`

接口：
- `IArchiveStrategy`
- `IArchiveWriter`
- `IArchiveReader`

当前实现：
- `BinaryArchiveStrategy`

职责：
- 定义归档文件格式
- 负责 entry 的写入与读取
- 校验归档头和 entry 合法性

当前归档头包含：
- magic: `BKS1`
- version: `3`
- flags
- compression name
- encryption name

当前 entry 包含：
- entry type
- relative path
- stored size
- original size
- payload checksum
- content checksum
- metadata

为什么要同时保存两类校验：

- `payload checksum`
  - 校验归档中实际存储的字节流
  - 用于发现归档载荷损坏

- `content checksum`
  - 校验还原后的原始文件内容
  - 用于发现错误密码、错误解码链路、解压错误等问题

### 5.3 `strategy/istream_processor` 与 `strategy/stream_processor`

接口：
- `ICompressionCodec`
- `IEncryptionCodec`
- `IStreamProcessor`

当前实现：
- `PipelineStreamProcessor`

职责：
- 把“文件原文”与“归档载荷”之间的转换抽象成处理流水线

当前处理顺序：
- 备份：`原文 -> 压缩 -> 加密 -> 写入归档`
- 还原：`归档载荷 -> 解密 -> 解压 -> 写回文件`

当某个阶段为 `none` 时，该阶段为空操作。

### 5.4 `strategy/codec_registry`

职责：
- 维护压缩算法注册表
- 维护加密算法注册表
- 根据算法名创建具体实例

当前已注册：
- compression:
  - `none`
  - `rle`
- encryption:
  - `none`
  - `xor-stream`

这样设计的好处：
- 新增算法时只需要在注册表登记
- `main.cpp` 不需要改算法分支

### 5.5 `strategy/ifile_filter`、`file_filter`、`filter_registry`、`filter_spec_builder`

这部分共同构成过滤子系统。

#### `IFileFilter`

职责：
- 面向引擎的统一过滤接口

#### `IFileFilterRule`

职责：
- 表示一条独立规则
- 例如路径规则、名字规则、尺寸规则、时间规则

#### `CompositeFileFilter`

职责：
- 持有多条 rule
- 用 AND 逻辑组合它们

#### `filter_registry`

职责：
- 维护规则类型注册表
- 根据 `FileFilterRuleSpec` 创建具体 rule

当前已注册规则：
- `path`
- `name`
- `size`
- `time`

#### `filter_spec_builder`

职责：
- 将 CLI 原始过滤参数解析成结构化 rule spec
- 负责过滤参数的格式校验与相互关系校验

#### 当前过滤语义

1. 不同过滤类型之间是 AND
2. 同一类型多个模式之间是 OR
3. 目录总是允许继续遍历
4. 只有普通文件参与当前过滤规则判断

这样设计的原因：
- 如果目录也被严格过滤，可能提前剪枝，导致子文件漏备份

### 5.6 `app/cli_parser`

职责：
- 解析命令行参数
- 生成 usage 文本
- 校验通用 CLI 参数
- 预触发过滤参数校验

它把 CLI 逻辑从 `main.cpp` 中分离出来，避免入口文件不断膨胀。

### 5.7 `utils/metadata_utils`

职责：
- 采集元数据
- 应用元数据

实现方式：
- `stat`
- `chmod`
- `chown`
- `utimensat`

### 5.8 `utils/path_utils`

职责：
- 存储路径规范化
- 统一归档中使用的相对路径格式
- 防止路径逃逸

当前安全约束：
- 归档中的存储路径必须是相对路径
- 禁止出现 `..`

这可以避免恶意归档在恢复时写出目标根目录之外。

### 5.9 `utils/logger`

职责：
- 提供简单日志接口
- 当前主要用于输出备份与还原阶段的进度信息

---

## 6. 核心设计思想

### 6.1 高内聚、低耦合

每个模块只负责自己的一层职责：
- 入口负责调度
- 引擎负责流程
- 归档策略负责格式
- 流处理负责压缩/加密流水线
- 注册表负责实例创建
- 过滤器负责规则判断

### 6.2 策略模式

本项目大量使用策略模式：
- 归档策略可替换
- 压缩策略可替换
- 加密策略可替换
- 过滤规则可替换

### 6.3 管道-过滤器思想

在数据处理层，采用典型流水线模型：
- 输入文件流
- 压缩
- 加密
- 归档写入

还原时反向执行。

### 6.4 先做“可扩展正确架构”，再叠加更多算法

当前的 `rle` 和 `xor-stream` 都不是终局算法，但接口已经稳定。
后续替换更强算法时，基本不需要碰引擎主流程。

---

## 7. 构建方式

### 7.1 环境要求

- Ubuntu 22.04
- CMake >= 3.16
- 支持 C++20 的编译器
  - 例如 `g++-11` 或以上

### 7.2 构建命令

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

生成产物：

```bash
./build/backup_cli
```

---

## 8. 使用方法

### 8.1 基本命令格式

#### 备份

```bash
./build/backup_cli \
  --mode backup \
  --src <source_dir> \
  --dest <archive_file>
```

#### 还原

```bash
./build/backup_cli \
  --mode restore \
  --src <archive_file> \
  --dest <restore_dir>
```

### 8.2 通用参数

| 参数 | 说明 |
|---|---|
| `--mode` | `backup` 或 `restore` |
| `--src` | 备份时为源目录；还原时为归档文件 |
| `--dest` | 备份时为归档文件；还原时为目标目录 |
| `--compression` | 压缩算法，当前支持 `none` / `rle` |
| `--encryption` | 加密算法，当前支持 `none` / `xor-stream` |
| `--password` | 当启用加密时必须提供 |

### 8.3 过滤参数

仅 `backup` 模式支持：

| 参数 | 说明 |
|---|---|
| `--include-path <glob>` | 按相对路径模式过滤 |
| `--include-name <glob>` | 按文件名模式过滤 |
| `--min-size <bytes>` | 最小文件大小 |
| `--max-size <bytes>` | 最大文件大小 |
| `--modified-after <YYYY-MM-DDTHH:MM:SS>` | 修改时间下界 |
| `--modified-before <YYYY-MM-DDTHH:MM:SS>` | 修改时间上界 |

---

## 9. 使用示例

### 9.1 最基础备份

```bash
./build/backup_cli \
  --mode backup \
  --src ./data \
  --dest ./backup.bks
```

### 9.2 最基础还原

```bash
./build/backup_cli \
  --mode restore \
  --src ./backup.bks \
  --dest ./restore_out
```

### 9.3 使用 RLE 压缩

```bash
./build/backup_cli \
  --mode backup \
  --src ./data \
  --dest ./backup_rle.bks \
  --compression rle
```

### 9.4 使用 XOR 流加密

```bash
./build/backup_cli \
  --mode backup \
  --src ./data \
  --dest ./backup_enc.bks \
  --encryption xor-stream \
  --password secret123
```

还原：

```bash
./build/backup_cli \
  --mode restore \
  --src ./backup_enc.bks \
  --dest ./restore_enc \
  --encryption xor-stream \
  --password secret123
```

### 9.5 同时启用压缩和加密

```bash
./build/backup_cli \
  --mode backup \
  --src ./data \
  --dest ./backup_secure.bks \
  --compression rle \
  --encryption xor-stream \
  --password secret123
```

还原：

```bash
./build/backup_cli \
  --mode restore \
  --src ./backup_secure.bks \
  --dest ./restore_secure \
  --compression rle \
  --encryption xor-stream \
  --password secret123
```

### 9.6 路径过滤

只备份 `docs/` 目录下文件：

```bash
./build/backup_cli \
  --mode backup \
  --src ./data \
  --dest ./docs_only.bks \
  --include-path "docs/*"
```

### 9.7 文件名过滤

只备份日志文件：

```bash
./build/backup_cli \
  --mode backup \
  --src ./data \
  --dest ./logs_only.bks \
  --include-name "*.log"
```

### 9.8 尺寸过滤

只备份大小在 1KB 到 1MB 之间的文件：

```bash
./build/backup_cli \
  --mode backup \
  --src ./data \
  --dest ./size_filtered.bks \
  --min-size 1024 \
  --max-size 1048576
```

### 9.9 时间过滤

只备份 2025 年 1 月 1 日之后修改的文件：

```bash
./build/backup_cli \
  --mode backup \
  --src ./data \
  --dest ./recent_files.bks \
  --modified-after 2025-01-01T00:00:00
```

### 9.10 多条件组合过滤

只备份：
- 位于 `docs/` 下
- 文件名包含 `report`
- 大于 4KB
- 2025 年后修改过

```bash
./build/backup_cli \
  --mode backup \
  --src ./data \
  --dest ./filtered_backup.bks \
  --include-path "docs/*" \
  --include-name "*report*" \
  --min-size 4096 \
  --modified-after 2025-01-01T00:00:00
```

---

## 10. 常见行为说明

### 10.1 为什么 restore 时也要指定压缩/加密算法？

因为当前设计中：
- 归档头会记录算法名
- reader 会校验你提供的算法是否与归档头一致

这样可以避免：
- 用错解压算法
- 用错解密算法
- 误把明文归档当成加密归档处理

### 10.2 如果密码错误会怎样？

当前通常会在恢复阶段触发内容 checksum 不匹配，从而失败。

### 10.3 为什么目录默认不过滤掉？

因为目录如果被提前剪枝，可能导致其子文件永远不会被遍历，从而漏备份。
所以当前策略是：
- 目录始终允许进入遍历
- 文件再根据规则做精确判断

---

## 11. 当前实现细节补充

### 11.1 校验算法

当前内部大量使用 FNV-1a 风格的 64 位校验值，用于：
- 文件内容 checksum
- 归档载荷 checksum
- 路径 checksum

它的定位是轻量一致性校验，而不是密码学哈希。

### 11.2 路径安全

归档中的路径会被规范化，并禁止：
- 绝对路径
- `..` 路径逃逸

这样可以降低还原到目标目录之外的风险。

### 11.3 临时文件

当同时启用压缩和加密时，当前 `PipelineStreamProcessor` 使用临时文件串联两个阶段：
- 先压缩到临时文件
- 再从临时文件读出并加密

还原时反向：
- 先解密到临时文件
- 再从临时文件读出并解压

这是一种简单可靠的原型实现，后续可进一步优化为纯流式无落盘管线。

---

## 12. 扩展指南

### 12.1 新增压缩算法

步骤：

1. 新建一个实现 `ICompressionCodec` 的类
2. 在 [src/strategy/codec_registry.cpp](/data/users/pennxue/backup_system/src/strategy/codec_registry.cpp) 注册

### 12.2 新增加密算法

步骤：

1. 新建一个实现 `IEncryptionCodec` 的类
2. 在 [src/strategy/codec_registry.cpp](/data/users/pennxue/backup_system/src/strategy/codec_registry.cpp) 注册

### 12.3 新增过滤规则

步骤：

1. 新建一个实现 `IFileFilterRule` 的类
2. 在 [src/strategy/filter_registry.cpp](/data/users/pennxue/backup_system/src/strategy/filter_registry.cpp) 注册规则类型
3. 如需 CLI 暴露，再扩展：
   - [src/app/cli_parser.cpp](/data/users/pennxue/backup_system/src/app/cli_parser.cpp)
   - [src/strategy/filter_spec_builder.cpp](/data/users/pennxue/backup_system/src/strategy/filter_spec_builder.cpp)

### 12.4 新增归档格式

步骤：

1. 实现新的 `IArchiveStrategy`
2. 实现对应的 writer / reader
3. 在入口装配时切换策略实例

---

## 13. 已知限制

当前版本的已知限制：

1. 仅支持普通文件和目录，不支持符号链接、设备文件、FIFO 等特殊文件。
2. 当前加密算法 `xor-stream` 仅为原型实现，不具备强安全性。
3. 当前压缩算法 `rle` 仅为原型实现，压缩效果有限。
4. 同时启用压缩和加密时依赖临时文件，性能和空间效率仍有优化空间。
5. 当前没有增量备份、版本管理、去重和快照功能。

---

## 14. 总结

当前这个项目已经不是一个简单的“文件复制脚本”，而是一个具备明确分层和扩展能力的备份系统原型。

它已经完成了以下关键基础设施：
- 备份 / 还原闭环
- 二进制单文件归档
- 元数据保存与恢复
- 可扩展压缩 / 加密架构
- 可扩展过滤规则架构
- 分层 CLI 解析与 builder / registry 体系

如果后续继续推进，这个代码基础已经足够支持：
- 更强压缩算法
- 更强加密算法
- 更复杂过滤器
- 特殊文件支持
- 更成熟的归档协议
- 定时 / 实时备份系统

