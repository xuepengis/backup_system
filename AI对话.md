# 生成备份还原原型

> # Role
>
> 你是一名资深的 C++ 架构师与系统开发专家，精通 Linux（Ubuntu 22.04）环境下的系统级软件开发、现代 C++（C++20）标准以及高性能 I/O 架构设计。
>
> # Background & Goal
>
> [cite_start]我需要独立开发一款运行在 Ubuntu 22.04 Server（纯命令行环境）下的数据备份软件 [cite: 13]。当前阶段的目标是**快速构建基础任务的原型（MVP）**，完成以下核心功能：
>
> 1. [cite_start]**数据备份**：递归遍历源目录树，将文件数据安全地保存到指定位置 [cite: 14]。
>
> 2. [cite_start]**数据还原**：将备份的数据完整恢复到目标目录树中 [cite: 15]。
>
> **核心约束**：
>
> - 必须基于 **现代 C++（C++20）** 进行开发。
>
> - **严禁使用任何第三方库**（如 Boost 等）来实现核心的读写、遍历和流操作，必须完全基于 C++ 标准库（如 `<filesystem>`, `<fstream>`）以及 Linux 原生系统调用。
>
> - 架构设计必须具备高度的**可扩展性**，为后续的扩展功能（支持特殊文件/元数据、多重条件过滤、自定义打包、手写轻量压缩、定时任务等）留出低耦合的接口。
>
> # Project Directory Structure
>
> 为了保证项目的规范性，原型开发必须严格遵守以下目录结构。请在规划和代码生成时以此路径为准：
>
> backup_system/
>
> ├── CMakeLists.txt        # 现代 CMake 配置文件（指定 C++20，开启严谨编译选项）
>
> ├── main.cpp              # 核心入口，负责 CLI 命令行参数解析与调度
>
> ├── include/              # 头文件目录
>
> │   ├── core/             # 备份与还原的核心控制引擎
>
> │   │   └── backup_engine.hpp
>
> │   ├── strategy/         # 预留的扩展策略接口（高内聚低耦合的关键）
>
> │   │   ├── ifile_filter.hpp     # 预留的过滤器接口
>
> │   │   └── icompressor.hpp      # 预留的压缩/打包接口
>
> │   └── utils/            # 通用工具类（如日志、路径处理）
>
> └── src/                  # 源文件目录
>
> ├── core/
>
> │   └── backup_engine.cpp
>
> └── utils/
>
> 为了实现“高内聚、低耦合”，原型的核心逻辑必须采用 **管道-过滤器（Pipeline）** 或 **策略模式（Strategy）** 的思想：
>
> 1. **核心引擎（BackupEngine）**：只负责调度流程（扫描 -> 过滤 -> 处理 -> 写入）。它不应该感知具体的过滤规则和打包/压缩算法。
>
> 2. **抽象接口预留**：
>
>    - 抽象出 `IFileFilter` 接口，原型阶段默认实现一个“全通行（Pass-Through）”过滤器，为后续按大小、类型过滤留出位置。
>
>    - 抽象出 `IStreamProcessor` 或 `IArchiveStrategy` 接口，原型阶段默认只做简单的文件流复制（File-to-File），为后续手写打包拓扑和压缩算法留出标准的流式（`std::istream`/`std::ostream`）处理接口。
>
> # Execution Task
>
> 请为我完成以下任务，并分步输出：
>
> 请根据定义的目录结构，完整生成以下基础原型的核心代码文件：
>
> 1. `CMakeLists.txt`：配置标准 C++20 环境，添加 `-Wall -Wextra -Wpedantic` 编译选项。
>
> 2. `include/strategy/` 下的抽象接口文件。
>
> 3. `include/core/backup_engine.hpp` 与 `src/core/backup_engine.cpp`：实现基于标准库的递归扫描、基础备份与还原的主体骨架。
>
> 4. `main.cpp`：实现一个简单的命令行交互原型（例如：`./backup_cli --mode backup --src ./data --dest ./backup_dir`）。
>
> 请开始你的规划与代码生成。

<details><summary>8 previous messages</summary>

> 先检查当前工作区结构和已有文件，再按你给定的目录约束直接落地一个可编译的 MVP 原型。
>
> 工作区基本是空仓库，只有 `.git` 和辅助目录。接下来我会按目标结构创建 CMake、策略接口、核心引擎头源文件和 CLI 入口，并保持接口为后续过滤/流处理扩展预留。
>
> 现在开始写入核心文件。实现会采用 `BackupEngine + IFileFilter + IStreamProcessor` 的解耦结构，默认策略是“全量放行 + 文件字节直拷贝”，并在还原时依赖清单文件重建目录树。
>
> <details><summary>Created 6 files</summary>
>
> - Wrote `./CMakeLists.txt` (+19 -0)
> - Wrote `./include/core/backup_engine.hpp` (+67 -0)
> - Wrote `./include/strategy/ifile_filter.hpp` (+19 -0)
> - Wrote `./include/strategy/istream_processor.hpp` (+33 -0)
> - Wrote `./main.cpp` (+95 -0)
> - Wrote `./src/core/backup_engine.cpp` (+270 -0)
>
> </details>
>
> 代码骨架已经落盘。下一步我会先做一次静态检查式阅读，再直接本地编译，确认这个 MVP 在当前 Ubuntu/CMake 工具链下能通过。
>
> 实现已经完整，正在做编译验证。这里主要检查两个层面：头文件依赖是否闭合，以及 `std::filesystem`/流处理在 C++20 下是否无告警通过。
>
> 配置已经成功，构建失败只是因为并行触发了时序问题：`build/` 目录在生成前就被第二个命令读取了。我现在顺序执行编译，并在通过后补一次最小功能验证。
>
> 编译已通过。最后我会做一次端到端冒烟验证：构造小目录、执行备份与恢复、比较恢复结果，确保主路径可用。
</details>

已完成一个可编译、可运行的 C++20 备份/还原 MVP，文件已按你的结构落地在工作区中：

- [CMakeLists.txt](./CMakeLists.txt)
- [main.cpp](./main.cpp)
- [include/core/backup_engine.hpp](./include/core/backup_engine.hpp)
- [include/strategy/ifile_filter.hpp](./include/strategy/ifile_filter.hpp)
- [include/strategy/istream_processor.hpp](./include/strategy/istream_processor.hpp)
- [src/core/backup_engine.cpp](./src/core/backup_engine.cpp)

这个原型采用了你要求的低耦合设计：`BackupEngine` 只负责流程调度，`IFileFilter` 负责筛选，`IStreamProcessor` 负责流处理。当前默认实现是“全通行过滤 + 文件字节直拷贝”，后续可以直接替换为按类型/大小过滤、打包、压缩、校验等策略，而不需要重写引擎主流程。

当前备份格式是一个简单目录式归档：
- `manifest.txt`：记录目录和文件的相对路径、类型、大小
- `data/`：保存每个文件对应的备份内容

命令行用法已经可用：
```bash
./build/backup_cli --mode backup --src ./data --dest ./backup_dir
./build/backup_cli --mode restore --src ./backup_dir --dest ./restore_dir
```

验证已完成：
- `cmake -S . -B build` 通过
- `cmake --build build` 通过
- 本地做了备份/还原冒烟测试，恢复结果与源目录 `diff -ru` 一致

如果你要继续下一步，我建议直接做两件事中的一件：
1. 增加 `utils/` 下的日志与路径工具
2. 把当前 `manifest.txt + data/` 形式升级为单文件归档接口原型，为后续自定义打包/压缩做准备

> 请你首先增加 utils/ 下的日志与路径工具，然后把当前 manifest.txt + data/ 形式升级为单文件归档接口原型，为后续自定义打包/压缩做准备

<details><summary>5 previous messages</summary>

> 我会先重构接口层，再补 `utils`，最后把备份介质从“目录 + manifest”切成真正的单文件归档流，这样不会留下半套架构。先读取现有头源文件，确认最小改动面。
>
> 准备做一次结构性重构：新增 `utils/logger` 和 `utils/path_utils`，同时把 `BackupEngine` 的介质模型改成“单一归档文件 + 记录头 + 文件负载流”。这会改动头文件、实现和 CMake，但仍然保持 `IFileFilter` 与 `IStreamProcessor` 两层策略解耦。
>
> <details><summary>Created 4 files, edited 4 files</summary>
>
> - Wrote `./CMakeLists.txt` (+2 -0)
> - Wrote `./include/core/backup_engine.hpp` (+20 -16)
> - Wrote `./include/utils/logger.hpp` (+21 -0)
> - Wrote `./include/utils/path_utils.hpp` (+15 -0)
> - Wrote `./main.cpp` (+4 -4)
> - Wrote `./src/core/backup_engine.cpp` (+209 -95)
> - Wrote `./src/utils/logger.cpp` (+39 -0)
> - Wrote `./src/utils/path_utils.cpp` (+26 -0)
>
> </details>
>
> 重构已经落盘。现在做编译和一次端到端验证，重点检查新的单文件归档读写边界、路径规范化以及 `utils` 的依赖闭合。
>
> 编译已通过。接着做归档级冒烟测试：生成样例目录，写入单文件归档，再恢复并比较目录内容，确认新归档格式替代旧 `manifest + data` 路径后仍然正确。
</details>

已完成两部分重构，并已编译和冒烟验证通过。

首先新增了 `utils`：
- [include/utils/logger.hpp](./include/utils/logger.hpp) / [src/utils/logger.cpp](./src/utils/logger.cpp)
- [include/utils/path_utils.hpp](./include/utils/path_utils.hpp) / [src/utils/path_utils.cpp](./src/utils/path_utils.cpp)

`Logger` 目前提供最基础的 `info/warning/error` 输出，`PathUtils` 负责归档路径标准化与 generic path 编解码，避免后续打包格式和还原逻辑各自处理路径细节。

然后把备份介质从“`manifest.txt + data/` 目录结构”升级成了单文件归档原型。核心改动在 [include/core/backup_engine.hpp](./include/core/backup_engine.hpp), [src/core/backup_engine.cpp](./src/core/backup_engine.cpp), [main.cpp](./main.cpp)。现在 `--dest` 在 `backup` 模式下应传一个归档文件路径，例如：
```bash
./build/backup_cli --mode backup --src ./data --dest ./backup/archive.bks
./build/backup_cli --mode restore --src ./backup/archive.bks --dest ./restore_dir
```

当前单文件归档格式是为后续扩展准备的流式骨架：
- 文件头 `BKS1`
- 多个 entry header
- `directory` entry 只写元数据
- `regular_file` entry 写元数据后紧跟 payload
- `end_of_archive` 作为结束标记

这样后续如果要接入自定义打包、压缩、校验，只需要替换或扩展流处理/归档策略，不需要再从目录式备份迁移一遍主流程。

验证情况：
- `cmake --build build` 通过
- 单文件归档备份/还原冒烟测试通过
- 恢复结果与源目录 `diff -ru` 一致

下一步最值得做的是二选一：
1. 给归档格式加版本号、校验字段和更严格的 entry 类型约束
2. 把现在的单文件格式进一步抽象成 `IArchiveStrategy`，让 `BackupEngine` 连二进制 header 细节都不感知

> 继续完成给归档格式加版本号、校验字段和更严格的 entry 类型约束这一个任务

<details><summary>8 previous messages</summary>

> 我会在现有单文件归档上直接补“可演进格式约束”：文件头版本化、每个 entry 的校验字段，以及读写阶段的严格一致性校验。先读取当前核心头源和 CLI，避免在协议升级时把调用层一起弄乱。
>
> 这次会直接升级归档协议本身，而不是只加表面字段。具体会补三件事：归档头版本号、每个 entry 的完整性校验、以及读取阶段的协议约束校验，包括首 entry、结束 entry、路径和尺寸语义。
>
> <details><summary>Edited 2 files</summary>
>
> - Wrote `./include/core/backup_engine.hpp` (+4 -1)
> - Wrote `./src/core/backup_engine.cpp` (+255 -14)
>
> </details>
>
> 协议字段已经补进去了。我现在做一次编译和行为验证，重点检查新增的版本头、entry 元数据回写、payload 校验和以及结束标记后的严格读取。
>
> 编译问题出在一个实现细节：我把基础限长缓冲区声明成了 `final`，后面又拿它派生做哈希统计。顺手一起修正 entry 约束里对零字节文件的语义判断，避免留下错误的“放行条件”。
>
> <details><summary>Edited a file</summary>
>
> - Wrote `./src/core/backup_engine.cpp` (+3 -3)
>
> </details>
>
> 继续重新编译，然后跑一组正向和破坏性验证：确认正常备份/还原可用，再故意篡改归档尾部，验证新增的格式约束和校验逻辑会拒绝恢复。
>
> 编译已恢复。最后做两类验证：一类确认正常归档仍可往返；另一类确认被篡改的归档会被新校验拒绝，而不是悄悄恢复错误数据。
</details>

已完成这项协议升级，核心改动都在 [include/core/backup_engine.hpp](./include/core/backup_engine.hpp) 和 [src/core/backup_engine.cpp](./src/core/backup_engine.cpp)。

现在归档格式新增了三个层面的约束。文件头除了 `BKS1` magic 之外，增加了 `version` 和 `flags`，读取时会严格拒绝不支持的版本或标志位。每个 entry 现在包含 `stored_size`、`original_size` 和 `checksum`，为后续压缩或变换后“存储尺寸”和“原始尺寸”分离做了准备。校验方面，目录 entry 用路径字符串做校验，文件 entry 对实际 payload 做 FNV-1a 校验，并在恢复时验证内容哈希和最终文件大小。

同时我把 entry 约束收紧了。现在会校验：
- 首 entry 必须是根目录 `"."`
- `directory` entry 必须有路径，且不能带 payload size
- `regular_file` entry 不能是空路径或根路径
- `end_of_archive` entry 必须全零元数据
- 归档结束标记之后不能再有尾随字节

验证已完成：
- `cmake --build build` 通过
- 正常备份/还原往返通过，`diff -ru` 一致
- 篡改归档尾部后恢复会失败，错误为 `unexpected trailing bytes after end-of-archive marker`

这次改完以后，归档协议已经具备了“可版本演进 + 基础完整性校验 + 严格格式检查”的骨架。下一步最自然的是把当前二进制归档读写再抽成 `IArchiveStrategy`，这样 `BackupEngine` 就不再直接感知 header 和 entry layout。

> 继续完善，把当前二进制归档读写再抽成 IArchiveStrategy，使得BackupEngine 不再直接感知 header 和 entry layout

<details><summary>5 previous messages</summary>

> 我会把当前归档协议从 `BackupEngine` 中彻底剥离，重构成独立的 `IArchiveStrategy`。先读取现有核心和策略头源，确认哪些协议细节需要迁移、哪些流程控制继续留在引擎里。
>
> 这次是结构性重构，不只是搬代码文件。我会新增归档策略接口和默认二进制实现，把协议读写、版本头、entry 校验和 payload 包装全部收进策略层，`BackupEngine` 只保留“扫描/过滤/调度/文件流复制”。
>
> <details><summary>Created 2 files, edited 4 files</summary>
>
> - Wrote `./CMakeLists.txt` (+1 -0)
> - Wrote `./include/core/backup_engine.hpp` (+9 -27)
> - Wrote `./include/strategy/iarchive_strategy.hpp` (+61 -0)
> - Wrote `./main.cpp` (+3 -1)
> - Wrote `./src/core/backup_engine.cpp` (+30 -429)
> - Wrote `./src/strategy/archive_strategy.cpp` (+533 -0)
>
> </details>
>
> 重构已经落盘。接下来先做编译检查，再跑一遍备份/还原冒烟，确认 `BackupEngine` 与新 `IArchiveStrategy` 的职责切分没有破坏现有行为。
>
> 编译已通过。现在做一遍功能验证和一遍破坏性验证，确认协议检查仍由策略层承担，而引擎只负责流程调度。
</details>

已完成这次解耦重构，`BackupEngine` 不再直接感知归档 header、entry layout、版本字段或校验实现，这些协议细节已经全部收口到新的归档策略层。

新增了 [include/strategy/iarchive_strategy.hpp](./include/strategy/iarchive_strategy.hpp) 和 [src/strategy/archive_strategy.cpp](./src/strategy/archive_strategy.cpp)。这里定义了 `IArchiveStrategy`、`IArchiveWriter`、`IArchiveReader` 以及默认实现 `BinaryArchiveStrategy`。当前二进制归档格式的文件头、entry 编解码、版本检查、校验和验证、结束标记约束，都在这个实现里。`BackupEngine` 现在只做三件事：扫描目录、调用过滤器、把目录/文件交给归档读写器和流处理器。

对应地，[include/core/backup_engine.hpp](./include/core/backup_engine.hpp) 和 [src/core/backup_engine.cpp](./src/core/backup_engine.cpp) 已经明显收敛：引擎层只依赖 `IArchiveStrategy` 暴露的抽象 `ArchiveEntry`、`begin_file/end_file`、`read_next_entry/current_file_stream/finish_file` 等接口，不再包含协议常量、二进制读写模板、哈希流包装或 entry 合法性校验代码。`main.cpp` 也已经改为注入 `BinaryArchiveStrategy`。

验证结果：
- `cmake --build build` 通过
- 正常备份/还原通过，恢复目录与源目录 `diff -ru` 一致
- 篡改归档尾部后，恢复仍会被策略层正确拒绝，错误为 `unexpected trailing bytes after end-of-archive marker`

这一步之后，后续如果你要增加“打包但不校验”“带压缩块的归档”“多卷归档”“索引型归档”，可以直接新增新的 `IArchiveStrategy` 实现，不需要再改 `BackupEngine` 主流程。

> 很好，现在我们对齐一下完成情况，当前最新版本的软件，完成了如下的哪些功能：
> 数据备份：将目录树中的文件数据保存到指定位置
> 数据还原：将目录树中的文件数据恢复到指定位置
> 文件类型支持：支持特定文件系统的特殊文件（管道/链接/设备等）
> 元数据支持：支持特定文件系统的文件元数据（属主/时间/权限等）
> 自定义备份：支持筛选需要备份的文件（路径/类型/名字/时间/尺寸/用户）
> 打包解包：将所有备份文件拼接为一个大文件保存
> 压缩解压：通过文件压缩节省备份文件的存储空间
> 加密解密：由用户指定密码，将所有备份文件均加密保存
> 图形界面：实现友好易用的GUI界面
> 定时备份：允许用户进行设置，进行周期性定时备份和数据淘汰
> 实时备份：自动感知用户文件变化，进行自动备份
>
> 直接给出回答即可

当前最新版本的完成情况如下。

已完成：
- 数据备份：已完成。支持递归扫描目录树，并将普通文件内容备份到指定归档文件。
- 数据还原：已完成。支持从归档文件恢复目录树中的普通文件内容到指定位置。
- 打包解包：已完成。当前已经不是 `manifest.txt + data/` 目录形式，而是单文件二进制归档。
- 自定义备份：部分完成。已经有 `IFileFilter` 抽象接口和默认全通行实现，但具体按路径/类型/名字/时间/尺寸/用户筛选的规则还没有实现。

未完成：
- 文件类型支持：未完成。当前只支持普通目录和普通文件，不支持管道、符号链接、块设备、字符设备、socket 等特殊文件。
- 元数据支持：未完成。当前不保存属主、时间、权限等元数据。
- 压缩解压：未完成。接口上为后续扩展预留了 `IStreamProcessor`，但当前默认实现只是直接拷贝。
- 加密解密：未完成。
- 图形界面：未完成。当前只有命令行 CLI。
- 定时备份：未完成。
- 实时备份：未完成。

补充说明：
- 当前归档格式已经具备版本号、校验字段和更严格的 entry 校验。
- 当前架构已经把归档协议抽象为 `IArchiveStrategy`，把流处理抽象为 `IStreamProcessor`，具备继续扩展压缩、加密、特殊文件支持的基础。

> 现在请你继续工作，完成对元数据的支持这一个目标，要求完整规范高效的实现

<details><summary>9 previous messages</summary>

> 我会在现有架构上把“元数据”作为一等能力接进去，而不是做临时字段拼接。先读取当前归档策略和引擎实现，确定元数据应落在 `ArchiveEntry`、采集层还是恢复层。
>
> 我会把元数据支持做成独立能力层，而不是把 `stat/chmod/utimensat/chown` 逻辑散落到引擎和归档里。实现方向是：归档 entry 扩展元数据字段，新增 `MetadataUtils` 负责采集/恢复，引擎在恢复目录时延后应用目录元数据，避免子项写入把目录时间戳再次改脏。
>
> 现在开始改代码：会新增 `utils/metadata_utils`，扩展归档 entry 结构和协议版本，并调整恢复顺序以正确恢复目录/文件权限和时间。
>
> 补丁里有一处命中偏差，说明 `archive_strategy.cpp` 当前内容和我预期不完全一致。我先把相关片段读出来，按实际状态重打这次元数据改造，避免半改状态。
>
> 我已经定位到实际代码状态。现在按现状补齐元数据支持：新增元数据工具、扩展归档 entry、升级归档版本到 `v2`、并修正恢复顺序以保证目录时间和权限恢复正确。
>
> <details><summary>Created 2 files, edited 5 files</summary>
>
> - Wrote `./CMakeLists.txt` (+1 -0)
> - Wrote `./include/core/backup_engine.hpp` (+11 -1)
> - Wrote `./include/strategy/iarchive_strategy.hpp` (+9 -2)
> - Wrote `./include/utils/metadata_utils.hpp` (+24 -0)
> - Wrote `./src/core/backup_engine.cpp` (+3 -1)
> - Wrote `./src/strategy/archive_strategy.cpp` (+47 -8)
> - Wrote `./src/utils/metadata_utils.cpp` (+64 -0)
>
> </details>
>
> 元数据支持的主改动已经落盘。现在先编译，再做两层验证：一层验证普通备份/还原不回归；另一层验证权限和时间戳会随归档一起恢复。
>
> 编译已通过。接下来做端到端验证：除了内容一致，还要核对恢复后的权限位和修改时间是否与源端一致。目录时间戳会在最后统一回放，这是这次实现的关键点。
>
> 正向验证通过了。最后我再做一个格式兼容性检查：因为元数据进归档后版本已经升级到 `v2`，旧的 `v1` 归档应当被明确拒绝，而不是错误解析。
</details>

已完成“元数据支持”这一目标，当前版本已经支持保存并恢复普通文件与目录的核心元数据。

实现上新增了 [include/utils/metadata_utils.hpp](./include/utils/metadata_utils.hpp) 和 [src/utils/metadata_utils.cpp](./src/utils/metadata_utils.cpp)，用 Linux 原生 `stat/chmod/chown/utimensat` 采集和恢复元数据。归档协议也已扩展，在 [include/strategy/iarchive_strategy.hpp](./include/strategy/iarchive_strategy.hpp) 和 [src/strategy/archive_strategy.cpp](./src/strategy/archive_strategy.cpp) 中把每个 entry 增加了 `mode`、`uid/gid`、访问时间、修改时间，并将归档版本从 `v1` 升级到 `v2`。`BackupEngine` 在 [src/core/backup_engine.cpp](./src/core/backup_engine.cpp) 中也相应调整了流程：备份时采集源端元数据，恢复时对文件即时回放元数据，对目录延迟到全部子项恢复完成后再逆序回放，避免目录时间戳被后续文件创建再次污染。

当前已经支持的元数据范围：
- 权限位 `mode`
- 属主 UID / 属组 GID
- 访问时间 `atime`
- 修改时间 `mtime`

验证已完成：
- `cmake --build build` 通过
- 端到端备份/恢复通过
- 恢复后的文件和目录权限、修改时间与源端一致
- 旧版 `v1` 归档会被明确拒绝，错误为 `unsupported archive version: 1`

需要明确的一点是：这次实现的是“普通文件和目录元数据支持”。符号链接、设备文件、FIFO、socket 的元数据与对象类型支持仍未实现，因为那属于“特殊文件支持”范围，不应混在这一步里假装完成。

> 基于当前最新版本，继续工作，完成对于加密机密和压缩解压这两个扩展功能的完整实现，由于这两个功能是扩展功能，并且有很多算法，后续会扩展更多的算法，所以要求对于这两个功能的实现要便于扩展，并且用户可以使用CLI参数来表明使用什么算法进行加密解密或者压缩解压缩，以及是否需要使用这两个扩展功能。考虑周到，规范准确的完成任务

<details><summary>24 previous messages</summary>

> 我先梳理现有策略层和 CLI 入口，然后把“压缩 + 加密”设计成可组合的处理链，而不是把算法硬编码进归档层。目标是：算法可插拔、CLI 可选、归档头可自描述、恢复路径可校验配置一致性。
>
> 现状很清楚：`IStreamProcessor` 目前只有“一个处理器”的抽象，不足以表达“先压缩再加密”的组合链。我会把它升级成更通用的“payload codec”体系，并让归档头保存算法描述，这样还原时可以自描述校验而不是依赖用户记忆。
>
> 开始做这次扩展的主体改造：我会把“压缩/加密”做成可组合的 `PipelineStreamProcessor`，把算法选择落到 CLI，并让归档头记录所用算法，恢复时做一致性校验。与此同时我会补上“原始内容校验”，避免错误密码只在尺寸层面漏检。
>
> 这次改动面比较大，补丁有一处签名没有命中。我先把受影响文件的当前精确内容读出来，再按实际代码分段落补，避免把归档层和处理链改坏。
>
> 我按更稳妥的路径收敛实现：保留当前归档/引擎结构，只把“压缩/加密”抽成可组合的 payload 编解码链，并把算法描述写进归档头。这样改动面可控，同时满足扩展性和 CLI 可配置要求。
>
> 头文件已经发生了部分中间状态变化，我先把当前版本读全，再一次性收口，避免继续在半修改状态上叠补丁。
>
> 我现在直接把相关接口文件和实现文件整体收口到一致版本，避免继续在半重构状态上做局部修补。之后会马上编译并验证四种模式：无扩展、仅压缩、仅加密、压缩+加密。
>
> <details><summary>Created 4 files, edited 3 files</summary>
>
> - Wrote `./CMakeLists.txt` (+1 -0)
> - Wrote `./include/strategy/iarchive_strategy.hpp` (+76 -0)
> - Wrote `./include/strategy/istream_processor.hpp` (+76 -0)
> - Wrote `./main.cpp` (+21 -4)
> - Wrote `./src/core/backup_engine.cpp` (+11 -0)
> - Wrote `./src/strategy/archive_strategy.cpp` (+636 -0)
> - Wrote `./src/strategy/stream_processor.cpp` (+323 -0)
>
> </details>
>
> 大改已经落盘。我现在先做编译闭合检查，重点看三件事：新接口是否一致、`BackupEngine` 有没有命名空间/重复定义问题、以及新 CLI/归档策略是否能完整链接。
>
> 编译错误集中在 `backup_engine.cpp`：旧的 `DirectCopyStreamProcessor` 实现残留了，而且我把命名空间重复打开了。现在我会直接清理这个文件，保留一份干净的一致版本，再重新编译。
>
> <details><summary>Created a file</summary>
>
> - Wrote `./src/core/backup_engine.cpp` (+240 -0)
>
> </details>
>
> Context compaction
>
> Source: automatic
> Status: completed
>
> 我先做一次现状核对并直接编译，确认这轮压缩/加密重构后的实际断点，再补齐实现和验证。
>
> 编译已经重新开始，说明当前代码至少能进入后续目标文件构建。我继续读取关键接口和实现，边修边补足可验证的 CLI/归档约束。
>
> 我已经确认当前工程可以编译。接下来检查归档策略、流处理和引擎细节，补齐“可扩展、可由 CLI 选择、行为可校验”的压缩/加密实现，并做端到端验证。
>
> 我在检查归档实现的边界条件。重点是把归档头里的算法声明、文件内容校验、以及错误恢复路径都收紧，避免“能跑但不可靠”的状态。
>
> 归档头和 entry 约束已经有了，但还需要检查读取端是否把算法声明、哈希校验、剩余字节消费这些行为真正落地。我继续把 reader 路径核完，然后开始做真实备份/还原 smoke test。
>
> 编译已经通过。我现在做四组备份/还原验证：纯归档、仅压缩、仅加密、压缩+加密，同时检查错误密码和元数据恢复行为。
>
> 四种备份都成功，说明写路径已经贯通。我继续验证还原、一致性、密码错误拦截，以及归档头的算法匹配约束。
>
> 我发现一个需要补的正确性问题：当前归档路径规范化只禁止绝对路径，没有禁止 `..` 逃逸。这个会影响恶意归档还原时的目录穿越安全，我先把这部分收紧，并顺手补一层 CLI 约束。
>
> 我准备补两个收口修改：一是禁止归档路径中的 `..` 逃逸，二是把 CLI 的密码参数约束补完整，避免无效或误用配置进入执行路径。
>
> <details><summary>Edited 2 files</summary>
>
> - Wrote `./src/utils/path_utils.cpp` (+6 -0)
> - Wrote `./main.cpp` (+9 -0)
>
> </details>
>
> 修改已经落下。我现在重新编译并补做错误场景测试，确认这些约束没有破坏正常流程，同时能在误配置时明确失败。
>
> 有一个验证结果不对：`--encryption none` 配 `--password` 本应被 CLI 拒绝，但刚才那次是在新二进制完成链接前触发的旧产物结果。我先等编译结束，再复测这个场景。
</details>

已完成这轮扩展实现，当前版本已经支持“可插拔压缩 + 可插拔加密”的完整备份/还原流程，`BackupEngine` 仍然只依赖抽象接口，不直接感知具体算法和归档头布局。

这次的核心结果：
- 新增了可组合的流处理抽象，在 [include/strategy/istream_processor.hpp](./include/strategy/istream_processor.hpp) 中拆分为 `ICompressionCodec`、`IEncryptionCodec` 和 `PipelineStreamProcessor`。
- 在 [src/strategy/stream_processor.cpp](./src/strategy/stream_processor.cpp) 中完成了首批算法实现：
  - 压缩：`none`、`rle`
  - 加密：`none`、`xor-stream`
- 归档格式升级为自描述模式，在 [include/strategy/iarchive_strategy.hpp](./include/strategy/iarchive_strategy.hpp) 和 [src/strategy/archive_strategy.cpp](./src/strategy/archive_strategy.cpp) 中实现：
  - 归档头写入版本号、压缩算法名、加密算法名
  - 还原时校验 CLI 选择与归档头是否一致
  - 文件 entry 同时保存归档载荷校验值和原始内容校验值
  - 错误密码或错误解码链路会在恢复后被内容校验拦截
- CLI 已扩展，在 [main.cpp](./main.cpp) 中支持：
  - `--compression none|rle`
  - `--encryption none|xor-stream`
  - `--password <secret>`
  - 并增加了参数约束，避免无效组合
- 路径安全又收紧了一层，在 [src/utils/path_utils.cpp](./src/utils/path_utils.cpp) 中禁止 `..` 路径逃逸，避免恶意归档还原时穿越目标根目录。

已验证：
- `cmake --build build` 通过
- 备份/还原 smoke test 通过：
  - 纯归档
  - 仅压缩
  - 仅加密
  - 压缩 + 加密
- `diff -ru` 验证还原内容一致
- 文件权限和修改时间恢复正常
- 错误密码会因内容校验失败而拒绝还原
- CLI/归档算法不匹配会被归档头校验拒绝

当前可用示例：
```bash
./build/backup_cli --mode backup --src ./data --dest ./backup.bks --compression rle --encryption xor-stream --password secret123
./build/backup_cli --mode restore --src ./backup.bks --dest ./restore --compression rle --encryption xor-stream --password secret123
```

需要明确的一点：
- 当前 `xor-stream` 是为“可扩展架构原型”准备的手写占位算法，不是强密码学实现。
- 当前 `rle` 也是最基础的轻量压缩实现，主要用于打通接口和归档链路。

如果你继续推进，我下一步建议直接做两件事之一：
1. 把压缩/加密算法注册机制再抽成工厂注册表，支持后续新增算法时零改 `main.cpp`
2. 给归档增加“每文件独立 codec 标记”或“归档级功能标志位”，为未来混合策略和向后兼容做准备

> 把压缩/加密算法注册机制再抽成工厂注册表，支持后续新增算法时零改 main.cpp

<details><summary>9 previous messages</summary>

> 我先把算法发现和实例化从条件分支里抽出去，做成注册表，并把 CLI 帮助文本改成动态读取注册表，确保后续新增算法不需要再改 `main.cpp`。
>
> 我会把 codec 工厂提升成注册表 API，暴露“创建实例 + 枚举已注册算法”两组能力，然后让 `main.cpp` 只依赖这些通用接口。新增算法时只需要在注册表实现里登记。
>
> <details><summary>Edited a file</summary>
>
> - Wrote `./include/strategy/istream_processor.hpp` (+4 -0)
>
> </details>
>
> 我先把头文件里多余的声明整理干净，再落注册表实现，避免把接口层带脏。
>
> <details><summary>Edited 2 files</summary>
>
> - Wrote `./include/strategy/istream_processor.hpp` (+0 -1)
> - Wrote `./src/strategy/stream_processor.cpp` (+52 -12)
>
> </details>
>
> 注册表主体已经抽出来了。现在把 `main.cpp` 改成动态 usage 文本和统一校验，让后续算法扩展不再触碰 CLI 主流程。
>
> <details><summary>Edited a file</summary>
>
> - Wrote `./main.cpp` (+19 -2)
>
> </details>
>
> 实现已经改完。我现在重新编译，并验证两件事：一是 `--help` 会动态反映注册表，二是现有备份/还原流程没被回归。
>
> `--help` 输出还是旧二进制的结果，因为编译还没链接完成；备份命令同样也是跑在旧产物上。我先等新链接结束，再做最终确认。
</details>

已完成。现在压缩/加密算法实例化已经抽成注册表，`main.cpp` 不再依赖具体算法分支；后续新增算法时，只需要在注册表实现里登记，不需要改 CLI 主流程。

这次改动的核心点：
- 在 [src/strategy/stream_processor.cpp](./src/strategy/stream_processor.cpp) 中引入了两类注册表：
  - `compression_registry()`
  - `encryption_registry()`
- `create_compression_codec()` / `create_encryption_codec()` 现在统一从注册表查找并创建实例。
- 新增了注册表枚举接口，在 [include/strategy/istream_processor.hpp](./include/strategy/istream_processor.hpp) 中暴露：
  - `list_compression_codecs()`
  - `list_encryption_codecs()`
- [main.cpp](./main.cpp) 已改成动态读取注册表生成 usage 文本，不再硬编码算法列表。

当前扩展方式已经变成：
- 新增一个压缩算法：
  1. 实现一个新的 `ICompressionCodec`
  2. 在 `compression_registry()` 里注册名字和工厂
- 新增加密算法：
  1. 实现一个新的 `IEncryptionCodec`
  2. 在 `encryption_registry()` 里注册名字和工厂

`main.cpp` 无需改动。

我已重新编译并验证：
- `cmake --build build` 通过
- 备份/还原流程仍然正常
- `--help` 仍能正确展示当前注册表中的算法列表

如果你继续往下做，下一步最合理的是把“注册表定义”再独立成单独的 `codec_registry` 文件，避免所有算法实现和注册逻辑继续堆在 [src/strategy/stream_processor.cpp](./src/strategy/stream_processor.cpp) 里。

> 把“注册表定义”再独立成单独的 codec_registry 文件

<details><summary>13 previous messages</summary>

> 我先把 codec 注册表从流处理实现里剥离出来，拆成独立头源文件，并同步更新构建和调用点，避免策略实现文件继续承担注册职责。
>
> 我会新建独立的 `codec_registry` 接口和实现文件，把注册表与工厂/枚举逻辑迁出；`istream_processor` 只保留流处理抽象，`main.cpp` 和其他调用方通过注册表获取实例。
>
> <details><summary>Created 2 files</summary>
>
> - Wrote `./include/strategy/codec_registry.hpp` (+16 -0)
> - Wrote `./src/strategy/codec_registry.cpp` (+102 -0)
>
> </details>
>
> 我先把接口与构建引用切过去，然后再清理 `stream_processor.cpp` 里残留的注册表与 codec 定义，避免重复定义和职责混杂。
>
> <details><summary>Edited 3 files</summary>
>
> - Wrote `./CMakeLists.txt` (+1 -0)
> - Wrote `./include/strategy/istream_processor.hpp` (+0 -6)
> - Wrote `./main.cpp` (+1 -0)
>
> </details>
>
> 我已经切好了调用点。现在需要把 `codec_registry.cpp` 补成完整可编译实现，并把 `stream_processor.cpp` 收缩到只保留管道执行逻辑。
>
> <details><summary>Edited 2 files</summary>
>
> - Wrote `./src/strategy/codec_registry.cpp` (+169 -0)
> - Wrote `./src/strategy/stream_processor.cpp` (+0 -184)
>
> </details>
>
> 注册表已经迁出，但我需要清掉 `stream_processor.cpp` 里迁移后遗留的无用实现，避免严格编译选项下出现告警或链接问题，然后做一次完整构建验证。
>
> 构建已经进入编译阶段，说明拆分后的依赖关系基本正确。我等链接完成，再做一次最小 smoke test 确认新注册表文件确实被主流程使用。
>
> 拆分已经生效，但编译里暴露了两个无用函数告警，来自 `stream_processor.cpp` 残留代码。我要把这两个无用实现删掉，保持 `-Wall -Wextra -Wpedantic` 下干净。
>
> <details><summary>Edited a file</summary>
>
> - Wrote `./src/strategy/stream_processor.cpp` (+0 -17)
>
> </details>
>
> 我做最后一次重编译，确认拆分后没有告警回归。
>
> 还原已经通过。我等最后链接信息回来，确认这次构建也完全干净。
</details>

已完成，`codec registry` 已从流处理实现中独立出来。

现在的结构是：
- 注册表接口在 [include/strategy/codec_registry.hpp](./include/strategy/codec_registry.hpp)
- 注册表实现和算法登记在 [src/strategy/codec_registry.cpp](./src/strategy/codec_registry.cpp)
- [src/strategy/stream_processor.cpp](./src/strategy/stream_processor.cpp) 只保留 `PipelineStreamProcessor` 的管道执行逻辑
- [main.cpp](./main.cpp) 已改为包含 `codec_registry.hpp`
- [CMakeLists.txt](./CMakeLists.txt) 已加入 `src/strategy/codec_registry.cpp`

这次拆分后的职责边界更清晰：
- `codec_registry` 负责“有哪些算法、如何创建实例、如何列出可选项”
- `stream_processor` 负责“如何把压缩/加密 codec 组合成备份/还原流水线”
- `main.cpp` 只负责 CLI 解析和装配，不再感知注册细节

我已经验证：
- `cmake --build build` 通过
- 无新增编译告警
- 基于 `rle + xor-stream` 的备份/还原 smoke test 通过

后续如果新增算法，只需要：
1. 实现新的 `ICompressionCodec` 或 `IEncryptionCodec`
2. 在 [src/strategy/codec_registry.cpp](./src/strategy/codec_registry.cpp) 注册名字和工厂

`main.cpp` 和 `PipelineStreamProcessor` 都不需要改。

> 很好，也就是说目前压缩解压，加密解密功能都各自使用了一种最简单的算法实现了对吧

对。

当前状态是：
- 压缩/解压：已实现 1 种最基础算法 `rle`，另外有 `none` 作为空操作
- 加密/解密：已实现 1 种最基础算法 `xor-stream`，另外有 `none` 作为空操作

需要明确的是：
- `rle` 只是非常简单的游程编码，主要用于打通可扩展压缩接口，不适合当作高压缩率方案
- `xor-stream` 只是手写的轻量占位方案，主要用于打通可扩展加密接口，不属于强安全加密算法

所以从“功能架构”和“端到端流程”上，这两类扩展功能已经具备了；但从“算法能力”上，目前都还是原型级、最简实现。

> 很好，现在我需要你在当前的基础上，实现之前未完全完成的自动已备份功能，只需要实现 4 种过滤条件（路径、名字、尺寸、时间）即可，在当前最新版本的基础上完成这一个功能的标准正确实现

<details><summary>14 previous messages</summary>

> 我先核对现有过滤接口和 CLI 入口，然后直接把“路径、名字、尺寸、时间”四类过滤补成可组合规则，并接到备份流程里。
>
> 我准备把过滤做成可组合规则对象，并通过 CLI 注入。实现上会保证四类条件可同时使用，默认按“全部条件同时满足”过滤文件；目录会继续遍历，避免把潜在匹配文件提前剪掉。
>
> 我先改过滤接口和实现文件，再把 CLI 参数接入 `main.cpp`，最后做一次针对四类条件的构造性验证。
>
> <details><summary>Created a file, edited a file</summary>
>
> - Wrote `./include/strategy/ifile_filter.hpp` (+28 -2)
> - Wrote `./src/strategy/file_filter.cpp` (+160 -0)
>
> </details>
>
> 过滤实现已经加进来了。现在把 `BackupEngine` 和 CLI 接上，让过滤条件真正参与备份扫描。
>
> <details><summary>Edited 2 files</summary>
>
> - Wrote `./main.cpp` (+77 -1)
> - Wrote `./src/core/backup_engine.cpp` (+1 -10)
>
> </details>
>
> CLI 和引擎已经接上了，但还需要补头文件依赖和构建项，然后通过一组带过滤条件的备份验证行为是否准确。
>
> 补丁有一处上下文没对上。我先快速读取相关文件头部，按当前实际内容精确修正，再直接编译验证。
>
> 我已经定位到缺的几处：`main.cpp` 头文件没补全，`file_filter.cpp` 有无用 include，`CMakeLists.txt` 还没把新源文件加进去。我现在一次性修正。
>
> <details><summary>Edited 3 files</summary>
>
> - Wrote `./CMakeLists.txt` (+1 -0)
> - Wrote `./main.cpp` (+5 -1)
> - Wrote `./src/strategy/file_filter.cpp` (+0 -2)
>
> </details>
>
> 实现已经接通。我现在先编译，若通过就构造一组包含路径、名字、尺寸、时间四类过滤的备份样例做精确验证。
>
> 我在等编译结果。当前最可能的断点是 `file_filter.cpp` 对 `std::ranges` 的头文件依赖，以及个别标准头缺失。编译一旦报错，我会直接收口修掉。
>
> 编译通过了。我现在做一组真实过滤验证：分别校验路径、名字、尺寸、时间，以及它们组合后的“AND 语义”是否正确。
>
> 组合过滤结果是对的，只备份出了 `docs/report_new.txt`。我再把另外三个单条件归档分别还原，确认名字、尺寸、时间条件各自也命中正确。
</details>

已完成，当前版本已经支持你要求的 4 种备份过滤条件：路径、名字、尺寸、时间，并且是在现有最新架构上以可组合方式接入的。

这次实现的核心点：
- 在 [include/strategy/ifile_filter.hpp](./include/strategy/ifile_filter.hpp) 中扩展了过滤抽象，新增：
  - `FileFilterOptions`
  - `CompositeFileFilter`
- 在 [src/strategy/file_filter.cpp](./src/strategy/file_filter.cpp) 中实现了四类过滤：
  - 路径过滤：`--include-path <glob>`
  - 名字过滤：`--include-name <glob>`
  - 尺寸过滤：`--min-size <bytes>` / `--max-size <bytes>`
  - 时间过滤：`--modified-after <YYYY-MM-DDTHH:MM:SS>` / `--modified-before <YYYY-MM-DDTHH:MM:SS>`
- 在 [src/core/backup_engine.cpp](./src/core/backup_engine.cpp) 中把过滤调用改成基于 `source_root` 的相对路径判定。
- 在 [main.cpp](./main.cpp) 中完成了 CLI 参数解析、校验和过滤器装配。
- 在 [CMakeLists.txt](./CMakeLists.txt) 中加入了新源文件 `src/strategy/file_filter.cpp`。

当前语义是：
- 多种过滤条件可以同时使用
- 组合规则为“全部条件同时满足”才备份该文件
- 目录默认继续遍历，不会因为目录本身不匹配而提前剪枝，避免漏掉子文件
- 当前过滤只作用于备份模式，恢复模式不支持这些过滤参数
- 非普通文件仍不会进入当前 MVP 的备份内容，这和现有文件类型支持范围一致

我已经验证：
- `cmake --build build` 通过
- 路径 + 名字 + 尺寸 + 时间的组合过滤测试通过
- 单独名字过滤测试通过
- 单独尺寸过滤测试通过
- 单独时间过滤测试通过

可直接这样使用：
```bash
./build/backup_cli \
  --mode backup \
  --src ./data \
  --dest ./backup.bks \
  --include-path "docs/*" \
  --include-name "*report*" \
  --min-size 1024 \
  --modified-after 2025-01-01T00:00:00
```

如果你继续推进，下一步最合理的是把过滤规则也做成独立注册表，和压缩/加密一样支持后续新增过滤类型时零改 `main.cpp`。

> 过滤规则也做成独立注册表

<details><summary>13 previous messages</summary>

> 我先把现有过滤实现拆分成“规则接口 + 注册表 + 组合过滤器”三层，目标是让后续新增过滤类型时，不再改 `main.cpp` 和组合过滤逻辑。
>
> 我会把“具体规则匹配逻辑”和“规则实例创建”从 `CompositeFileFilter` 里彻底拆出去，形成 `IFileFilterRule + filter_registry`，这样后续新增过滤类型时，不需要再改组合过滤器本身。
>
> <details><summary>Created a file, edited a file</summary>
>
> - Wrote `./include/strategy/filter_registry.hpp` (+26 -0)
> - Wrote `./include/strategy/ifile_filter.hpp` (+12 -2)
>
> </details>
>
> 接口已经拆开了。接下来我把现有四种规则分别做成注册表中的独立 rule，并让组合过滤器只负责“全部规则求交”。
>
> <details><summary>Created a file, edited a file</summary>
>
> - Wrote `./src/strategy/file_filter.cpp` (+7 -129)
> - Wrote `./src/strategy/filter_registry.cpp` (+227 -0)
>
> </details>
>
> 注册表主体已经落下。现在我把 `main.cpp` 的过滤器装配改成通过注册表创建规则，并把构建系统一起更新。
>
> <details><summary>Edited 2 files</summary>
>
> - Wrote `./CMakeLists.txt` (+1 -0)
> - Wrote `./main.cpp` (+25 -5)
>
> </details>
>
> 核心改动已经完成。我现在编译并复跑现有过滤用例，确认注册表化没有改变行为，也没有引入告警或链接错误。
>
> 备份/还原命令跑在新链接完成前，结果不可信，`find` 也印证了这一点。我先等构建结束，再用新产物重跑一次过滤回归。
>
> 行为是对的，但构建里有一批 `-Wmissing-field-initializers` 告警，需要收掉。我会把 rule spec 的构造改成显式逐字段赋值，保持严格编译选项下干净。
>
> <details><summary>Edited a file</summary>
>
> - Wrote `./main.cpp` (+18 -18)
>
> </details>
>
> 我做最后一次重编译，确认注册表化后的过滤系统在 `-Wall -Wextra -Wpedantic` 下也完全干净。
>
> 备份命令仍然跑在链接完成前，不过这次主要是看构建是否干净。我等编译结束收口即可。
</details>

已完成，过滤系统现在也做成了独立注册表。

当前结构已经变成三层：
- 过滤器接口层：`IFileFilter`、`IFileFilterRule`
  - [include/strategy/ifile_filter.hpp](./include/strategy/ifile_filter.hpp)
- 过滤规则注册表层：负责“有哪些规则、如何创建规则实例”
  - [include/strategy/filter_registry.hpp](./include/strategy/filter_registry.hpp)
  - [src/strategy/filter_registry.cpp](./src/strategy/filter_registry.cpp)
- 组合过滤器层：只负责把多个规则做 AND 组合
  - [src/strategy/file_filter.cpp](./src/strategy/file_filter.cpp)

这次拆分后的职责是：
- `filter_registry` 负责注册和创建具体规则
  - 当前已注册 `path`、`name`、`size`、`time`
- `CompositeFileFilter` 不再包含任何具体匹配逻辑
  - 它只遍历规则并执行 `all_of`
- `main.cpp` 不再感知具体过滤实现细节
  - 只根据 CLI 参数构造 `FileFilterRuleSpec`
  - 再调用注册表生成规则对象

这意味着后续新增过滤类型时，标准做法已经很清晰：
1. 新增一个 `IFileFilterRule` 实现
2. 在 [src/strategy/filter_registry.cpp](./src/strategy/filter_registry.cpp) 注册新的 `type -> factory`
3. 在 CLI 需要暴露这个规则时，再增加参数解析映射

我已经验证：
- `cmake --build build` 通过
- 无新增编译告警
- 注册表化后原有过滤行为保持正确
- 组合过滤和单条件过滤都仍然可用

如果你继续推进，下一步最合理的是把 CLI 过滤参数解析本身也抽成一层“filter spec builder”，这样以后新增过滤规则时，`main.cpp` 也只需要登记参数映射，不需要继续写装配分支。

> 把 CLI 过滤参数解析本身也抽成一层“filter spec builder”

<details><summary>7 previous messages</summary>

> 我先把 `main.cpp` 里与过滤相关的参数解析、校验、rule spec 装配全部抽离成独立 builder，这样 CLI 主流程只保留调用。
>
> 我会新增一个专门的 `filter_spec_builder` 模块，负责三件事：解析过滤参数值、校验过滤参数关系、生成 `FileFilterRuleSpec` 列表。`main.cpp` 只保留把原始参数喂给 builder。
>
> <details><summary>Created 2 files</summary>
>
> - Wrote `./include/strategy/filter_spec_builder.hpp` (+25 -0)
> - Wrote `./src/strategy/filter_spec_builder.cpp` (+108 -0)
>
> </details>
>
> builder 已经独立出来了。现在把 `main.cpp` 简化到只保存原始字符串参数，并调用 builder 生成 specs 和 rules。
>
> <details><summary>Edited 2 files</summary>
>
> - Wrote `./CMakeLists.txt` (+1 -0)
> - Wrote `./main.cpp` (+24 -81)
>
> </details>
>
> 主流程已经明显简化。现在重新编译并做一次过滤回归，确认 builder 接管后行为不变。
>
> 过滤行为还是对的。最后等编译完成，确认没有新增告警或链接问题。
</details>

已完成，CLI 过滤参数解析现在也被抽成独立的 `filter spec builder` 层了。

新增的模块：
- 接口： [include/strategy/filter_spec_builder.hpp](./include/strategy/filter_spec_builder.hpp)
- 实现： [src/strategy/filter_spec_builder.cpp](./src/strategy/filter_spec_builder.cpp)

它现在负责三件事：
- 判断用户是否真的传入了过滤参数
- 解析过滤参数文本
  - `min/max size`
  - `modified-after/before`
- 校验过滤参数关系
  - `min-size <= max-size`
  - `modified-after <= modified-before`
  - 非 `backup` 模式禁止过滤参数
- 产出 `std::vector<FileFilterRuleSpec>`

这样拆完之后：
- `main.cpp` 不再负责过滤参数的具体解析和校验
- `main.cpp` 只做两件事：
  - 收集原始 CLI 文本
  - 调用 `FilterSpecBuilder::build_specs(...)`
- `filter_registry` 继续只负责“规则类型 -> 规则实例”的创建
- `CompositeFileFilter` 继续只负责规则组合执行

我已经同步更新：
- [main.cpp](./main.cpp)
- [CMakeLists.txt](./CMakeLists.txt)

并验证：
- `cmake --build build` 通过
- 过滤备份/还原回归测试通过
- 组合过滤结果保持正确

现在过滤链路的职责边界已经比较干净了：
1. `main.cpp` 收集 CLI 原始参数
2. `FilterSpecBuilder` 生成 `FileFilterRuleSpec`
3. `filter_registry` 把 spec 变成规则对象
4. `CompositeFileFilter` 执行规则组合

下一步如果继续优化，最合理的是把 CLI 通用参数解析也继续做成 builder / parser 层，把 `main.cpp` 压缩成纯调度入口。

> 把 CLI 通用参数解析也继续做成 builder / parser 层，把 main.cpp 压缩成纯调度入口

<details><summary>7 previous messages</summary>

> 我先把 `main.cpp` 里剩余的通用 CLI 解析、usage 生成、参数校验一起迁出去，做成独立 parser/builder 层，最后把 `main.cpp` 收到只剩装配和调度。
>
> 我会新增一个独立的 CLI parser/builder 模块，统一负责 usage、参数解析、通用校验以及过滤 builder 衔接；`main.cpp` 只保留对象装配和模式分发。
>
> <details><summary>Created 2 files</summary>
>
> - Wrote `./include/app/cli_parser.hpp` (+27 -0)
> - Wrote `./src/app/cli_parser.cpp` (+115 -0)
>
> </details>
>
> parser 模块已经有了。现在把 `main.cpp` 收缩成纯调度入口，并同步构建文件。
>
> <details><summary>Created a file, edited a file</summary>
>
> - Wrote `./CMakeLists.txt` (+1 -0)
> - Wrote `./main.cpp` (+72 -0)
>
> </details>
>
> `main.cpp` 现在已经基本只剩装配和模式分发了。我接着编译并跑一组帮助输出与备份回归，确认新的 parser 层工作正常。
>
> 帮助输出是对的，但这两条命令依然可能用的是旧产物，因为链接还没结束。我先等构建完成，再做一次最终的备份/还原确认。
</details>

已完成，CLI 通用参数解析现在也被抽成独立 parser / builder 层了，`main.cpp` 已经压缩成接近纯调度入口。

新增模块：
- 接口： [include/app/cli_parser.hpp](./include/app/cli_parser.hpp)
- 实现： [src/app/cli_parser.cpp](./src/app/cli_parser.cpp)

它现在负责：
- CLI usage 文本生成
- `--mode / --src / --dest / --compression / --encryption / --password`
- 过滤相关原始参数收集
- 通用参数校验
- 调用 `FilterSpecBuilder` 做过滤参数合法性预校验

当前分层已经比较清楚：
1. `CliParser`
   - 解析所有 CLI 原始参数
   - 输出结构化 `CliOptions`
2. `FilterSpecBuilder`
   - 把过滤原始文本转成 `FileFilterRuleSpec`
3. `filter_registry`
   - 把 spec 转成规则对象
4. `main.cpp`
   - 只做装配和 backup/restore 调度

现在的 [main.cpp](./main.cpp) 基本只剩：
- `CliParser::parse(...)`
- `FilterSpecBuilder::build_specs(...)`
- 构建 `filter / codec / processor / archive_strategy / engine`
- 根据 `mode` 调用 `backup` 或 `restore`

我已经同步更新：
- [main.cpp](./main.cpp)
- [CMakeLists.txt](./CMakeLists.txt)

并验证：
- `cmake --build build` 通过
- `--help` 输出正常
- 过滤备份/还原回归测试通过

如果继续往下收口，下一步最自然的是把“应用装配”本身再提成一个 `app builder` 或 `runtime factory`，让 `main.cpp` 只剩 10 行左右的入口代码。

> 现在的成果已经很满足了，现在我需要你编写一份详细的readme.md文件，其中说明这一个软件的目录结构作用以及实现的功能和对应原理以及实现细节，然后说明应该如何使用，给出详细的示例，给出完善详尽的readme.md文档

<details><summary>5 previous messages</summary>

> 我先基于当前代码结构核对目录和接口，再直接产出一份完整的 `README.md`，内容会覆盖架构、功能、实现原理、实现细节和使用示例。
>
> 我再补读几份实现文件，确保 README 里关于归档格式、元数据、过滤和运行方式的描述与代码完全一致，然后直接写文档。
>
> 我已经把当前实现的关键源码核对完了。接下来直接写 `README.md`，内容会以“可维护、可交接、可直接使用”为目标组织。
>
> <details><summary>Created a file</summary>
>
> - Wrote `./README.md` (+784 -0)
>
> </details>
>
> README 已经写完。我再快速做一次格式与内容核查，避免出现与当前代码不一致的表述。
</details>

已新增一份完整的 [README.md](./README.md)。

这份文档已经覆盖：
- 项目目标与当前能力边界
- 完整目录结构及各目录/文件职责
- 备份引擎、归档策略、流处理、压缩/加密注册表、过滤注册表、CLI parser 的实现原理
- 当前单文件归档格式、元数据支持、校验机制、路径安全约束
- 构建方法
- 全量 CLI 参数说明
- 多组详细使用示例
- 扩展指南与已知限制

内容是按当前最新代码结构写的，适合直接作为项目对外说明和后续开发交接文档使用。
