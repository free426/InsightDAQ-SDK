#ifndef DAQEXPORT_H
#define DAQEXPORT_H

// ================================================================
// DaqExport.h — 插件导出宏（正确写法）
// ================================================================
//
// ⚠️ 常见错误：使用 Q_PLUGIN_METADATA 宏
//
//   ❌ 错误写法（会导致 createPlugin 无法导出）：
//      class MyPlugin : public QObject, public IDaqPlugin {
//          Q_OBJECT
//          Q_PLUGIN_METADATA(IID IDaqPlugin_iid)  // ← 这行导致问题
//          Q_INTERFACES(IDaqPlugin)               // ← 这行也导致问题
//      };
//
//   ✅ 正确写法：
//      class MyPlugin : public QObject, public IDaqPlugin {
//          Q_OBJECT
//      public:
//          // ... 实现接口
//      };
//      DAQ_PLUGIN_EXPORT IDaqPlugin* createPlugin() { return new MyPlugin(); }
//
// 原因：Q_PLUGIN_METADATA 会让 Qt 插件系统接管 DLL 导出，
//       覆盖我们的 createPlugin 工厂函数。
//
// ================================================================

// 插件导出宏：extern "C" 防止名称修饰，visibility("default") 确保符号可见
#ifdef _WIN32
    #define DAQ_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
    #define DAQ_PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif

#endif // DAQEXPORT_H
