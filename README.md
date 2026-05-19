# Smart Library Management System in C++20

一个基于 C++20 的智能图书管理系统，面向课程设计、数据结构实践和面向对象编程学习场景。项目采用分层解耦与服务化设计，覆盖图书、读者、借阅、预约、推荐、信用、报表、多终端同步和安全审计等模块。

> English keywords: C++20, library management system, object-oriented programming, console application, recommendation system, data persistence.

## 项目亮点

- **现代 C++ 实现**：使用 C++20、STL 容器、移动语义、`std::optional`、`std::filesystem` 等特性。
- **分层架构清晰**：按 Core / Service / Advanced / Utility 拆分职责，便于维护和扩展。
- **完整业务闭环**：支持图书管理、读者管理、借阅、归还、续借、逾期罚金和预约排队。
- **智能化功能**：提供个性化推荐、热门图书统计、信用积分体系、通知中心和数据分析。
- **数据持久化**：支持本地文件保存、自动加载、CSV/JSON 导入导出和自动备份。
- **管理能力增强**：包含社区互动、多终端同步、安全审计、风险评估和多格式报表生成。

## 功能模块

| 模块 | 功能 |
| --- | --- |
| 图书管理 | 新增、删除、修改、库存调整、分类统计、多条件查询 |
| 读者管理 | 注册、修改信息、注销、活跃读者统计、借阅排行榜 |
| 借阅服务 | 借书、还书、续借、历史记录、逾期记录、罚金计算 |
| 搜索系统 | 关键词搜索、标题/作者/分类搜索、高级组合查询 |
| 预约系统 | 图书预约、预约队列、优先级调整、可借通知 |
| 推荐系统 | 个性化推荐、历史记录推荐、热门图书推荐 |
| 通知中心 | 到期提醒、自定义通知、未读通知统计 |
| 信用体系 | 信用分计算、积分加扣、暂停/恢复借阅权限 |
| 数据分析 | 日报、月报、借阅趋势、分类热度、热门预测 |
| 数据存储 | 文件持久化、CSV/JSON 导出、自动备份 |
| 安全审计 | 权限策略、会话记录、审计日志、风险告警 |
| 多终端同步 | 设备注册、同步任务规划、同步状态记录 |

## 技术栈

- Language: C++20
- IDE: Visual Studio 2022
- Build: MSVC / Visual Studio Solution
- Data storage: local `.dat`, `.csv`, `.json`, `.txt`, `.md`, `.html`
- Core containers: `std::unordered_map`, `std::vector`, `std::map`, `std::set`, `std::queue`

## 目录结构

```text
.
├── *.h / *.cpp              # 核心源码
├── Project2.sln             # Visual Studio 解决方案
├── Project2.vcxproj         # Visual Studio C++ 工程配置
├── demo_data/               # 演示数据与导出结果
├── README.md
└── .gitignore
```

## 快速运行

### 方式一：Visual Studio

1. 使用 Visual Studio 2022 打开 `Project2.sln`。
2. 选择 `x64` 和 `Debug` 或 `Release` 配置。
3. 构建并运行项目。

### 方式二：命令行（MSVC 环境）

请先打开 `x64 Native Tools Command Prompt for VS 2022`，进入仓库目录后执行：

```bat
msbuild Project2.sln /p:Configuration=Debug /p:Platform=x64
```

运行生成的可执行文件：

```bat
x64\Debug\Project2.exe
```

## 使用说明

程序包含演示流程和交互式管理员功能。交互模式下的管理员密码为：

```text
1024
```

演示数据位于 `demo_data/` 目录，程序可自动加载图书、读者和借阅记录等本地数据。

## 核心设计

项目采用四层组织方式：

- **Core**：定义 `Book`、`Reader`、`BorrowRecord` 等核心实体。
- **Service**：封装图书、读者、借阅、搜索等业务逻辑。
- **Advanced**：实现推荐、预约、通知、信用、社区、分析、安全审计等高级功能。
- **Utility**：提供日期处理、文件管理、日志、加密和字符串工具。

这种结构将“数据对象”“业务动作”“高级能力”和“基础工具”分离，降低模块耦合，方便后续继续扩展 Web 前端、数据库存储或真实多终端同步。

## 后续计划

- 增加 CMake 构建支持，提升跨平台可用性。
- 补充单元测试，覆盖图书、读者、借阅、预约等核心服务。
- 增加运行截图或 GIF，展示交互式菜单和主要功能流程。
- 将演示数据和运行时数据进一步分离，避免用户数据污染仓库。
- 引入 Qt 或 Web 前端，提供更友好的图形化界面。
- 优化推荐系统，引入更真实的协同过滤或机器学习策略。

## License

建议使用 MIT License。若仓库中尚未添加 `LICENSE` 文件，可以后续补充。
