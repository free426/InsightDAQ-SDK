#ifndef DAQIO_H
#define DAQIO_H

#include <QString>
#include <QVector>
#include <cstdint>

// ================================================================
// DaqIO — DAT 文件读取工具
// ================================================================
// 提供读取 .dat 文件的便捷接口，支持两种格式：
//   - 0xDA 格式（新版，float64，DatPacket 规范）
//   - 0xAB 格式（旧版，int16，RecordingExporter 格式）
//
// 插件开发者通过 DaqIO::readDatDirectory() 一行代码读取所有数据。
// ================================================================

struct DatFile
{
    QString fileName;         // 文件名，如 "CH1_000001.dat"
    QVector<double> samples;  // 采样数据（已转为主机字节序，单位 mV）
    int channel = -1;         // 通道号（从元数据或文件名解析）
    uint64_t timestampSec = 0; // 时间戳（Unix epoch 秒）
    uint32_t frameIndex = 0;  // 帧序号
    bool valid = false;       // 解析是否成功
    QString error;            // 错误信息（valid=false 时有值）
};

namespace DaqIO {

// 读取单个 .dat 文件（自动判断 0xDA / 0xAB 格式）
DatFile readDatFile(const QString& filePath);

// 读取目录下所有 .dat 文件（按文件名排序，跳过无效文件）
QVector<DatFile> readDatDirectory(const QString& dirPath);

} // namespace DaqIO

#endif // DAQIO_H
