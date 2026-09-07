# InsightDAQ 插件 SDK v1.0

## 快速开始

### 1. 创建插件项目

```
my_plugin/
├── src/
│   └── MyPlugin.cpp        ← 复制 templates/MyPlugin.cpp，修改算法
├── node.json               ← 复制 templates/node.json，填写信息
└── CMakeLists.txt          ← 复制 templates/CMakeLists.txt
```

### 2. 修改 MyPlugin.cpp

```cpp
#include <DaqPluginSDK.h>

class MyPlugin : public QObject, public IDaqPlugin
{
    Q_OBJECT
public:
    QString pluginId() const override { return "my_plugin"; }
    QString pluginName() const override { return "我的插件"; }
    QString category() const override { return "信号分析"; }
    QString description() const override { return "功能描述"; }

    QJsonObject process(const QString& datDir, const QJsonObject& params) override
    {
        // 一行读取所有 .dat 文件
        QVector<DatFile> files = DaqIO::readDatDirectory(datDir + "/dat");

        // 写你的算法...
        return QJsonObject{{"text", "处理完成"}};
    }
};

DAQ_PLUGIN_EXPORT IDaqPlugin* createPlugin() { return new MyPlugin(); }
#include "MyPlugin.moc"
```

### 3. 编写 node.json

```json
{
    "id": "my_plugin",
    "name": "我的插件",
    "category": "信号分析",
    "description": "功能描述",
    "type": "cpp",
    "inputs": [],
    "outputs": [],
    "displayType": "chart",
    "parameters": [
        {"name": "threshold", "type": "float", "default": 0.8, "description": "阈值 (mV)"}
    ]
}
```

### 4. 编译

```bash
cmake -B build -DDAQ_SDK_DIR=path/to/sdk
cmake --build build --config Release
```

### 5. 部署

将编译产物放到主程序的 `custom_nodes/插件名/` 目录：

```
custom_nodes/my_plugin/
├── node.json
└── libmy_plugin.dll       ← 或 libmy_plugin.so (Linux)
```

启动主程序，插件自动识别。

## ⚠️ 常见错误

### 不要使用 Q_PLUGIN_METADATA

```cpp
// ❌ 错误：会导致 createPlugin 无法导出
class MyPlugin : public QObject, public IDaqPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IDaqPlugin_iid)  // ← 删除这行
    Q_INTERFACES(IDaqPlugin)               // ← 删除这行
};

// ✅ 正确：
class MyPlugin : public QObject, public IDaqPlugin {
    Q_OBJECT
};
DAQ_PLUGIN_EXPORT IDaqPlugin* createPlugin() { return new MyPlugin(); }
```

## API 参考

### IDaqPlugin 接口

| 方法 | 说明 |
|------|------|
| `pluginId()` | 唯一标识（与 node.json 的 id 一致） |
| `pluginName()` | 显示名称 |
| `category()` | 分类名称 |
| `description()` | 功能描述 |
| `process(datDir, params)` | 核心处理函数 |
| `parameterSchema()` | 参数定义（可选） |

### process() 返回值

| 格式 | 说明 |
|------|------|
| `{"chartFile": "xxx.png"}` | 显示图片 |
| `{"data": [...]}` | 显示数据表格 |
| `{"text": "..."}` | 显示文本 |
| `{"error": "..."}` | 显示错误 |

### DaqIO 工具

```cpp
// 读取单个文件
DatFile file = DaqIO::readDatFile("path/to/file.dat");

// 读取整个目录
QVector<DatFile> files = DaqIO::readDatDirectory(datDir + "/dat");
```

### DatFile 结构体

| 字段 | 类型 | 说明 |
|------|------|------|
| `fileName` | QString | 文件名 |
| `samples` | QVector\<double\> | 采样数据 |
| `channel` | int | 通道号 |
| `timestampSec` | uint64_t | 时间戳 |
| `valid` | bool | 解析是否成功 |
| `error` | QString | 错误信息 |
