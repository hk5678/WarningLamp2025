
#include "qmserial.h"
#include <QDateTime>
#include <QDebug>

// =========== CRC16 & 工具 ==============
namespace {

// 标准 Modbus RTU CRC16（多项式 0xA001，低位在前）
static quint16 modbusCRC16(const QByteArray &data) {
    quint16 crc = 0xFFFF;
    for (unsigned char b : data) {
        crc ^= b;
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001;
            else              crc >>= 1;
        }
    }
    return crc;
}

static void appendCRC(QByteArray &frame) {
    quint16 crc = modbusCRC16(frame);
    quint8 lo = crc & 0xFF;
    quint8 hi = (crc >> 8) & 0xFF;
    frame.append(static_cast<char>(lo));
    frame.append(static_cast<char>(hi));
}

static quint16 coilAddrFromNum(const QString &num) {
    if (num == "DO1") return 0x0000;
    if (num == "DO2") return 0x0001;
    if (num == "DO3") return 0x0002;
    if (num == "DO4") return 0x0003;
    return 0x0000; // 默认 DO1, 不要写死 DO3
}

static bool verifyCRC(const QByteArray &frame) {
    if (frame.size() < 4) return false;
    const int n = frame.size();
    quint16 got = static_cast<quint8>(frame[n-2]) | (static_cast<quint8>(frame[n-1]) << 8);
    quint16 expect = modbusCRC16(frame.left(n - 2));
    return got == expect;
}



} // namespace
// =====================================

QmSerial::QmSerial(QObject* parent)
    : QObject(parent)
    , m_serial(new QSerialPort(this))
{
    connect(m_serial, &QSerialPort::readyRead, this, &QmSerial::onReadyRead);
}

QmSerial::~QmSerial() {
    if (m_serial->isOpen()) m_serial->close();
}

void QmSerial::setWarning(WarningLamp *warning) {
    warninglamp = warning; // 兼容原接口（当前未实际使用）
}

void QmSerial::SetParameter(QString port) {
    // 支持传 "16" 或 "COM16"
    QString name = port.trimmed();
    if (!name.startsWith("COM", Qt::CaseInsensitive)) name = "COM" + name;

    m_serial->setPortName(name);
    m_serial->setBaudRate(QSerialPort::Baud9600);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);
    open_port();
}

bool QmSerial::open_port() {
    if (!m_serial->isOpen()) {
        if (!m_serial->open(QIODevice::ReadWrite)) {
            qDebug() << "Serial port open error!" << m_serial->errorString();
            return false;
        }
    }
    qDebug() << "Serial port open success!" << m_serial->portName();
    return true;
}

bool QmSerial::close_port() {
    if (m_serial->isOpen()) m_serial->close();
    return true;
}

// ============ 发送指令 =============

void QmSerial::ReadStatus() {
    QByteArray f;
    f.append('\x01'); // addr
    f.append('\x02'); // Read Discrete Inputs
    f.append('\x00'); f.append('\x00'); // start addr 0x0000
    f.append('\x00'); f.append('\x04'); // quantity 4
    appendCRC(f);
    m_serial->write(f);
    currentState = State::WaitingForReadStatusResponse;
}

void QmSerial::CleanLampAll() {
    // 0x0F 写多线圈，起始0x0000，数量3，1字节数据，全关 0x00
    QByteArray f;
    f.append('\x01'); f.append('\x0F');
    f.append('\x00'); f.append('\x00'); // start addr
    f.append('\x00'); f.append('\x03'); // quantity
    f.append('\x01');                   // byte count
    f.append('\x00');                   // coils: b000
    appendCRC(f);
    m_serial->write(f);
    currentState = State::WaitingForCleanLampResponse;
}

void QmSerial::CleanLamp(QString &num) {
    // 0x05 写单线圈：写0x0000表示关闭
    quint16 addr = coilAddrFromNum(num);
    QByteArray f;
    f.append('\x01'); f.append('\x05');
    f.append(static_cast<char>((addr >> 8) & 0xFF));
    f.append(static_cast<char>(addr & 0xFF));
    f.append('\x00'); f.append('\x00'); // OFF
    appendCRC(f);
    m_serial->write(f);
    currentState = State::WaitingForCleanLampResponse;
    qDebug()<<"CleanLamp  "<<f.toHex();
    // m_serial->flush();
    // m_serial->waitForBytesWritten(100);
}

void QmSerial::FlashLamp(QString &num) {
    // 0x05 写单线圈：写0xFF00表示打开
     qDebug("QmSerial::FlashLamp 01 05 00 00 FF 00 8C 3A ");

    quint16 addr = coilAddrFromNum(num);
    QByteArray f;
    f.append('\x01'); f.append('\x05');
    f.append(static_cast<char>((addr >> 8) & 0xFF));
    f.append(static_cast<char>(addr & 0xFF));
    f.append('\xFF'); f.append('\x00'); // ON
    appendCRC(f);
    m_serial->write(f);
    currentState = State::WaitingForFlashLampResponse;
    m_serial->flush();
    m_serial->waitForBytesWritten(100);
    // 如果要“闪烁”，请在上层加一个延时后 CleanLamp(num)

    qDebug() <<"QmSerial::FlashLamp"<<num<<"==>"<< f.toHex();
}

void QmSerial::FlashLamp5s() {
    // 保留原寄存器方案，只把CRC改为现算
    //01 10 00 03 00 02 04 00 04 00 64 F3 90

    qDebug("QmSerial::Flash5S");
    QByteArray f;
    f.append('\x01'); f.append('\x10');
    f.append('\x00'); f.append('\x03'); // start addr
    f.append('\x00'); f.append('\x02'); // qty 2
    f.append('\x04');                   // bytes 4
    f.append('\x00'); f.append('\x04');
    f.append('\x00'); f.append('\x32');
    appendCRC(f);
    m_serial->write(f);
    currentState = State::WaitingForFlashLamp5sResponse;
    qDebug() <<"QmSerial::FlashLamp5s  ==>"<< f.toHex();
    m_serial->flush();
    m_serial->waitForBytesWritten(100);
}

// ============ 接收解析 =============

void QmSerial::onReadyRead() {
    m_rxBuffer.append(m_serial->readAll());
    tryParseFrames();
}

void QmSerial::tryParseFrames() {
    while (true) {
        if (m_rxBuffer.size() < 4) return; // 最小帧

        quint8 func = static_cast<quint8>(m_rxBuffer[1]);
        int need = 0;

        if (func == 0x01 || func == 0x02 || func == 0x03 || func == 0x04) {
            if (m_rxBuffer.size() < 5) return;
            quint8 byteCount = static_cast<quint8>(m_rxBuffer[2]);
            need = 3 + byteCount + 2;
        } else if (func == 0x05 || func == 0x06 || func == 0x0F || func == 0x10) {
            need = 8;
        } else {
            // 未知/错误，跳过一个字节
            m_rxBuffer.remove(0, 1);
            continue;
        }

        if (m_rxBuffer.size() < need) return;

        QByteArray frame = m_rxBuffer.left(need);
        if (!verifyCRC(frame)) {
            // CRC错误，跳过一个字节继续
            m_rxBuffer.remove(0, 1);
            continue;
        }

        const QString old = status;

        switch (currentState) {
        case State::WaitingForReadStatusResponse:
            status = handleReadStatusResponse(frame);
            break;
        case State::WaitingForCleanLampResponse:
            status = handleCleanLampResponse(frame);
            break;
        case State::WaitingForFlashLampResponse:
            status = handleFlashLampResponse(frame);
            break;
        case State::WaitingForFlashLamp5sResponse:
            status = handleFlashLamp5sResponse(frame);
            break;
        default:
            // 接收到响应但没有等待任何状态
            break;
        }

        if (status != old) emit statusChanged(status);

        // 移除已处理帧
        m_rxBuffer.remove(0, need);
        // 处理完一次后回到 Idle（可按需保持）
        currentState = State::Idle;
    }
}

// ============ 各响应处理 =============

QString QmSerial::handleReadStatusResponse(const QByteArray &frame) {
    if (frame.size() < 5 || static_cast<quint8>(frame[1]) != 0x02) {
        qDebug() << "Unexpected ReadStatus resp" << frame.toHex(' ');
        return status;
    }
    quint8 byteCount = static_cast<quint8>(frame[2]);
    if (byteCount < 1) return status;

    quint8 inputs = static_cast<quint8>(frame[3]); // 低位对应低通道
    bool di1 = inputs & 0x01; // 按钮1
    bool di2 = inputs & 0x02; // 按钮2

    if (di1 && !di2) {
        status = "ONLINE";
    } else if (di1 && di2) {
        status = "CLEAN";
    } else {
        if (status == "WARNING") {
            QString n = QString("DO1"); CleanLamp(n);
            n = "DO2"; CleanLamp(n);
            n = "DO3"; CleanLamp(n);
        }
        status = "OFFLINE";
    }
    qDebug() << "ReadStatus parsed:" << status << " inputs=" << QString::number(inputs, 16);
    return status;
}

QString QmSerial::handleCleanLampResponse(const QByteArray &frame) {
    if (!verifyCRC(frame)) qDebug() << "Clean resp CRC bad";
    return status;
}

QString QmSerial::handleFlashLampResponse(const QByteArray &frame) {
    if (!verifyCRC(frame)) qDebug() << "Flash resp CRC bad";
    return status;
}

QString QmSerial::handleFlashLamp5sResponse(const QByteArray &frame) {
    if (!verifyCRC(frame)) qDebug() << "Flash5s resp CRC bad";
    return status;
}

// ============ 其他工具/接口 =============

QString QmSerial::getStatus() {
    if (status.isNull() || status.isEmpty()) status = "ONLINE";
    return status;
}

void QmSerial::setStatus(QString strStatus) {
    status = strStatus;
}

QString QmSerial::QByteArray_to_QString(QByteArray valu)
{
    QString value;
    for(int i=0;i<valu.size();i++) {
        unsigned char V=valu[i];
        value+=QString("%1").arg(V, 2, 16, QChar('0'));
    }
    return value;
}
