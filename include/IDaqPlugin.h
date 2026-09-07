#ifndef IDAQPLUGIN_H
#define IDAQPLUGIN_H

#include <QString>
#include <QJsonObject>
#include <QVariantList>

// ================================================================
// IDaqPlugin — C++ 插件接口
// ================================================================
// 插件开发者实现此接口，从 DAT写盘输出目录读取 .dat 文件，
// 执行自定义算法，返回图表或数据结果。
//
// ⚠️ 重要：不要使用 Q_PLUGIN_METADATA 和 Q_INTERFACES 宏！
//    使用 DaqExport.h 中的 DAQ_PLUGIN_EXPORT 宏导出 createPlugin()。
//
// 正确写法：
//   class MyPlugin : public QObject, public IDaqPlugin {
//       Q_OBJECT
//   public:
//       // ... 实现所有纯虚函数
//   };
//   DAQ_PLUGIN_EXPORT IDaqPlugin* createPlugin() { return new MyPlugin(); }
// ================================================================

class IDaqPlugin
{
public:
    virtual ~IDaqPlugin() = default;

    // ── 基本信息 ──
    virtual QString pluginId() const = 0;
    virtual QString pluginName() const = 0;
    virtual QString category() const = 0;
    virtual QString description() const = 0;

    // ── 核心处理 ──
    // datDir: DAT写盘输出目录（含 dat/ 子目录）
    // params: 用户在配置面板设置的参数
    //
    // 返回值约定：
    //   {"chartFile": "xxx.png"}           → 显示图片
    //   {"data": [...]}                    → 返回数据
    //   {"text": "..."}                    → 显示文本
    //   {"error": "错误信息"}              → 显示错误
    virtual QJsonObject process(const QString& datDir,
                                const QJsonObject& params) = 0;

    // ── 可选：参数定义（自动生成配置面板）──
    virtual QJsonObject parameterSchema() const { return {}; }
};

// 接口 IID（全局唯一标识符）
#define IDaqPlugin_iid "com.insightdaq.IDaqPlugin/1.0"
Q_DECLARE_INTERFACE(IDaqPlugin, IDaqPlugin_iid)

#endif // IDAQPLUGIN_H
