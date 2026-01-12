# 📈 高频交易撮合引擎

一个基于C++17的高性能订单撮合引擎，支持价格时间优先的订单匹配、实时成交回报和网络通信。

## ✨ 核心特性

- **高性能架构**：基于epoll的非阻塞I/O，单线程事件驱动
- **完整订单处理**：支持新单、撤单、部分成交、完全成交
- **价格时间优先**：标准交易所撮合逻辑
- **网络通信**：自定义二进制协议，低延迟
- **实时监控**：完整日志系统和成交回报
- **现代C++**：使用C++17标准，RAII，智能指针，STL容器

## 🏗️ 系统架构

```
客户端 → 网络层(TcpServer/Connection) 
      → 协议层(MessageCodec) 
      → 核心引擎(MatchingEngine) 
      → 订单簿(OrderBook) 
      → 成交回报
```

## 📁 项目结构

```
MatchingEngine/
├── CMakeLists.txt          # 构建配置
├── main.cpp               # 程序入口
├── core/                  # 核心引擎
│   ├── Order.h/cpp        # 订单数据结构
│   ├── OrderBook.h/cpp    # 订单簿实现
│   ├── MatchingEngine.h/cpp # 撮合引擎主逻辑
│   └── ExecutionReport.h  # 成交回报
├── network/               # 网络层
│   ├── TcpServer.h/cpp    # TCP服务器
│   └── Connection.h/cpp   # 客户端连接
├── protocol/              # 协议层
│   ├── MessageType.h      # 消息类型定义
│   └── MessageCodec.h/cpp # 消息编解码
├── utils/                 # 工具类
│   └── Logger.h           # 日志系统
└── logs/                  # 日志目录（运行时生成）
```

## 🔧 构建与运行

### 环境要求
- CMake 3.14+
- GCC 9+ 或 Clang 10+ (支持C++17)
- Linux (使用epoll API)

### 编译步骤
```bash
# 克隆项目
git clone https://github.com/syanken/MatchingEngine.git
cd MatchingEngine

# 创建构建目录
mkdir build && cd build

# 配置和编译
cmake ..
make -j$(nproc)

# 运行引擎
./bin/MatchingEngine
```

### 调试版本
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make clean && make
```

## 📡 通信协议

### 消息格式
```
+----------------+----------------+----------------+----------------+
|     Magic      |    Length      |     Type       |    Payload     |
|   (4 bytes)    |   (2 bytes)    |   (1 byte)     |   (变长)       |
|   0xABCDEF00   |  小端字节序     |  消息类型       |  数据体        |
+----------------+----------------+----------------+----------------+
```

### 消息类型
| 类型 | 值 | 方向 | 描述 |
|------|----|------|------|
| NEW_ORDER | 1 | 客户端→服务器 | 新订单请求 |
| CANCEL_ORDER | 2 | 客户端→服务器 | 撤单请求 |
| HEARTBEAT | 3 | 双向 | 心跳检测 |
| EXECUTION_REPORT | 4 | 服务器→客户端 | 成交回报 |

### 订单数据结构
```cpp
struct Order {
    std::string user_id;        // 用户ID (16字节)
    std::string order_id;       // 订单ID (32字节)
    OrderSide side;             // 买卖方向 (1:买, 0:卖)
    double price;               // 价格 (8字节)
    int32_t quantity;           // 数量 (4字节)
    int32_t remaining_quantity; // 剩余数量 (4字节)
    uint64_t timestamp;         // 时间戳 (8字节)
}; // 总计73字节
```

## 📊 撮合算法

### 价格时间优先
1. **价格优先**：买价高的优先，卖价低的优先
2. **时间优先**：同价格下先到的订单优先
3. **撮合逻辑**：
   - 新买单：与卖单簿最优价（最低）比较，价格>=卖价则成交
   - 新卖单：与买单簿最优价（最高）比较，价格<=买价则成交
   - 部分成交：订单拆分为多个成交

### 订单簿结构
```cpp
// 买单簿：价格降序排列
std::map<double, std::list<Order>, std::greater<double>> buyBook;

// 卖单簿：价格升序排列  
std::map<double, std::list<Order>, std::less<double>> sellBook;
```

## 🚀 性能特点

- **非阻塞I/O**：单线程处理数千连接
- **零拷贝设计**：减少内存复制
- **高效订单匹配**：O(log n)价格查找 + O(1)时间排序
- **智能缓冲**：动态调整接收缓冲区

## 🧪 测试方法

### 1. 手动测试（使用netcat）
```bash
# 连接到引擎
nc localhost 9999

# 发送新订单（需要按协议格式发送二进制数据）
# 具体测试工具见 test_client.cpp（待实现）
```

### 2. 单元测试（建议添加）
```bash
# 计划支持的测试
make test_order_book    # 订单簿测试
make test_matching      # 撮合逻辑测试  
make test_performance   # 性能测试
```

## 📈 监控与日志

### 日志级别
- INFO：连接建立/断开、订单接收、成交回报
- WARN：未知消息类型、格式错误
- ERROR：协议错误、订单解析失败
- DEBUG：心跳包、详细处理流程（需编译Debug版本）

### 日志文件
```
logs/engine.log        # 主日志文件
logs/engine_YYYYMMDD.log # 按日归档（待实现）
```

## 🔄 待实现功能

### 高优先级
- [ ] 并发安全机制（多线程支持）
- [ ] 持久化存储（订单状态恢复）
- [ ] 网络字节序处理
- [ ] 完整的测试套件

### 中级优先级  
- [ ] 管理接口（REST API）
- [ ] 性能监控系统
- [ ] 配置管理系统
- [ ] 风险控制模块

### 高级优先级
- [ ] FIX协议支持
- [ ] 多资产支持
- [ ] 回测系统
- [ ] Web管理界面

## 🛡️ 风险管理

### 当前支持
- 订单价格/数量验证
- 协议格式校验
- 连接超时处理

### 计划支持
- 用户持仓限额
- 单笔订单限额
- 频率限制
- 自成交防护

## 📚 相关技术

### 核心技术栈
- **网络编程**：epoll、非阻塞socket、ET模式
- **数据结构**：红黑树(std::map)、哈希表(std::unordered_map)
- **并发模型**：事件驱动、回调机制
- **序列化**：二进制协议、内存布局优化

### 第三方库
- [spdlog](https://github.com/gabime/spdlog)：高性能日志库
- CMake FetchContent：依赖管理

## 🎯 适用场景

- 量化交易研究
- 交易所原型开发
- 金融系统教学
- 交易算法测试

## ⚠️ 注意事项

1. **生产环境限制**：当前版本缺少持久化，不适合生产环境
2. **并发限制**：单线程设计，CPU密集场景可能成为瓶颈
3. **安全考虑**：无加密通信，适合内网环境
4. **数据完整性**：无崩溃恢复机制

## 🤝 贡献指南

1. Fork本仓库
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启Pull Request

## 📄 许可证

本项目采用MIT许可证 - 详见LICENSE文件

## 📞 联系方式

如有问题或建议，请通过以下方式联系：
- 提交Issue
- 发送邮件至：3055045895@qq.com
- 项目主页

## 🙏 致谢

感谢以下开源项目：
- [spdlog](https://github.com/gabime/spdlog) - 快速C++日志库
- [CMake](https://cmake.org/) - 跨平台构建系统

---

**免责声明**：本项目为教育研究用途，不构成金融投资建议。在实际交易中使用前，请进行充分测试和风险评估。
