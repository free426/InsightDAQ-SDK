// DaqIO.cpp — DAT 文件读取工具（SDK 自包含版本）
// 不依赖主程序任何源码，可直接编译进插件 DLL

#include "DaqIO.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QDataStream>
#include <QByteArray>
#include <cstdint>

// ================================================================
// 大端序读取辅助
// ================================================================
static uint16_t readU16BE(const char* buf, int offset)
{
    auto* p = reinterpret_cast<const uint8_t*>(buf + offset);
    return static_cast<uint16_t>((static_cast<uint16_t>(p[0]) << 8) | p[1]);
}

static uint32_t readU32BE(const char* buf, int offset)
{
    auto* p = reinterpret_cast<const uint8_t*>(buf + offset);
    return (static_cast<uint32_t>(p[0]) << 24) |
           (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) |
            static_cast<uint32_t>(p[3]);
}

static uint64_t readU48BE(const char* buf, int offset)
{
    auto* p = reinterpret_cast<const uint8_t*>(buf + offset);
    return (static_cast<uint64_t>(p[0]) << 40) |
           (static_cast<uint64_t>(p[1]) << 32) |
           (static_cast<uint64_t>(p[2]) << 24) |
           (static_cast<uint64_t>(p[3]) << 16) |
           (static_cast<uint64_t>(p[4]) << 8) |
            static_cast<uint64_t>(p[5]);
}

static uint64_t readU40BE(const char* buf, int offset)
{
    auto* p = reinterpret_cast<const uint8_t*>(buf + offset);
    return (static_cast<uint64_t>(p[0]) << 32) |
           (static_cast<uint64_t>(p[1]) << 24) |
           (static_cast<uint64_t>(p[2]) << 16) |
           (static_cast<uint64_t>(p[3]) << 8) |
            static_cast<uint64_t>(p[4]);
}

// ================================================================
// 从文件名解析通道号
// ================================================================
static int parseChannelFromFileName(const QString& fileName)
{
    if (fileName.startsWith("CH", Qt::CaseInsensitive)) {
        QString numStr;
        for (int i = 2; i < fileName.size(); ++i) {
            if (fileName[i].isDigit()) numStr += fileName[i];
            else break;
        }
        if (!numStr.isEmpty()) return numStr.toInt();
    }
    if (fileName.startsWith("TRIG", Qt::CaseInsensitive)) return 5;
    return -1;
}

// ================================================================
// 0xDA 格式解析（内联 DatPacket::deserialize 逻辑）
// ================================================================
static DatFile parseDaFormat(const QString& filePath, const QByteArray& data)
{
    DatFile result;
    result.fileName = QFileInfo(filePath).fileName();
    result.channel = parseChannelFromFileName(result.fileName);

    const int fileSize = data.size();
    if (fileSize < 12) {
        result.error = "0xDA 文件太小";
        return result;
    }

    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);

    // 包头 0xDA 0x01
    uint8_t magic1, magic2;
    stream >> magic1 >> magic2;
    if (magic1 != 0xDA || magic2 != 0x01) {
        result.error = "0xDA 包头错误";
        return result;
    }

    // 包长度
    uint32_t packetLength;
    stream >> packetLength;

    // 时间戳（6字节 + 5字节）
    result.timestampSec = 0;
    for (int i = 5; i >= 0; --i) {
        uint8_t b;
        stream >> b;
        result.timestampSec |= (static_cast<uint64_t>(b) << (i * 8));
    }
    // 跳过纳秒（5字节）
    for (int i = 0; i < 5; ++i) { uint8_t b; stream >> b; }

    // 数据标识 0xDA 0x02
    uint8_t d1, d2;
    stream >> d1 >> d2;
    if (d1 != 0xDA || d2 != 0x02) {
        result.error = "0xDA 数据标识错误";
        return result;
    }

    // 数据长度
    uint32_t dataLen;
    stream >> dataLen;

    // 数据格式
    uint8_t format;
    stream >> format;

    // 读取采样数据
    int bytesPerSample = 0;
    switch (format) {
    case 0x00: bytesPerSample = 2; break; // int16
    case 0x01: bytesPerSample = 4; break; // int32
    case 0x02: bytesPerSample = 4; break; // float32
    case 0x03: bytesPerSample = 8; break; // float64
    default:
        result.error = QStringLiteral("未知数据格式: 0x%1").arg(format, 2, 16, QChar('0'));
        return result;
    }

    int sampleCount = dataLen / bytesPerSample;
    result.samples.resize(sampleCount);

    for (int i = 0; i < sampleCount; ++i) {
        switch (format) {
        case 0x00: { // int16
            qint16 val; stream >> val;
            result.samples[i] = static_cast<double>(val);
            break;
        }
        case 0x01: { // int32
            qint32 val; stream >> val;
            result.samples[i] = static_cast<double>(val);
            break;
        }
        case 0x02: { // float32
            float val; stream >> val;
            result.samples[i] = static_cast<double>(val);
            break;
        }
        case 0x03: { // float64
            double val; stream >> val;
            result.samples[i] = val;
            break;
        }
        }
    }

    result.valid = true;
    return result;
}

// ================================================================
// 0xAB 格式解析（旧版 int16 格式）
// ================================================================
static DatFile parseAbFormat(const QString& filePath, const QByteArray& data)
{
    DatFile result;
    result.fileName = QFileInfo(filePath).fileName();

    const int fileSize = data.size();
    const char* buf = data.constData();
    const int MIN_SIZE = 37;

    if (fileSize < MIN_SIZE) {
        result.error = QStringLiteral("文件太小: %1 < %2").arg(fileSize).arg(MIN_SIZE);
        return result;
    }

    if (static_cast<uint8_t>(buf[0]) != 0xAB || static_cast<uint8_t>(buf[1]) != 0x01) {
        result.error = "0xAB 包头错误";
        return result;
    }

    result.channel = readU16BE(buf, 6);
    result.timestampSec = readU48BE(buf, 10);

    int offset = 21;
    if (offset + 2 > fileSize || static_cast<uint8_t>(buf[offset]) != 0xAB ||
        static_cast<uint8_t>(buf[offset + 1]) != 0x02) {
        result.error = "0xAB 电压段标识错误";
        return result;
    }
    offset += 2;

    if (offset + 4 > fileSize) { result.error = "voltageDataLength 越界"; return result; }
    uint32_t voltageDataLength = readU32BE(buf, offset);
    offset += 4;

    int sampleCount = static_cast<int>(voltageDataLength / 2);
    if (offset + static_cast<int>(voltageDataLength) > fileSize) {
        result.error = "电压数据越界";
        return result;
    }

    result.samples.resize(sampleCount);
    for (int i = 0; i < sampleCount; ++i) {
        int16_t raw = static_cast<int16_t>(readU16BE(buf, offset + i * 2));
        result.samples[i] = static_cast<double>(raw);
    }

    result.valid = true;
    return result;
}

// ================================================================
// readDatFile
// ================================================================
DatFile DaqIO::readDatFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        DatFile result;
        result.fileName = QFileInfo(filePath).fileName();
        result.error = QStringLiteral("无法打开: %1").arg(file.errorString());
        return result;
    }

    QByteArray data = file.readAll();
    file.close();

    if (data.size() < 2) {
        DatFile result;
        result.fileName = QFileInfo(filePath).fileName();
        result.error = "文件太小";
        return result;
    }

    uint8_t magic0 = static_cast<uint8_t>(data[0]);
    if (magic0 == 0xDA) return parseDaFormat(filePath, data);
    if (magic0 == 0xAB) return parseAbFormat(filePath, data);

    DatFile result;
    result.fileName = QFileInfo(filePath).fileName();
    result.error = QStringLiteral("未知格式: 0x%1").arg(magic0, 2, 16, QChar('0'));
    return result;
}

// ================================================================
// readDatDirectory
// ================================================================
QVector<DatFile> DaqIO::readDatDirectory(const QString& dirPath)
{
    QVector<DatFile> results;

    QDir dir(dirPath);
    if (!dir.exists()) {
        qWarning() << "[DaqIO] 目录不存在:" << dirPath;
        return results;
    }

    QStringList filters;
    filters << "*.dat";
    QStringList datFiles = dir.entryList(filters, QDir::Files, QDir::Name);

    if (datFiles.isEmpty()) {
        qDebug() << "[DaqIO] 目录中无 .dat 文件:" << dirPath;
        return results;
    }

    results.reserve(datFiles.size());

    for (const QString& fileName : datFiles) {
        QString filePath = dir.absoluteFilePath(fileName);
        DatFile file = readDatFile(filePath);
        if (file.valid) {
            results.append(file);
        } else {
            qWarning() << "[DaqIO] 跳过无效文件:" << fileName << file.error;
        }
    }

    qDebug() << "[DaqIO] 读取完成:" << dirPath
             << "有效:" << results.size() << "/" << datFiles.size();

    return results;
}
