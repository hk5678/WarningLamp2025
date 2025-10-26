#include <QObject>
#include <QSerialPort>
#include <QByteArray>
#include <QString>
// [差异] 相比第二个文件：本文件没有头文件保护宏；包含的头更少（未包含 <QDateTime>, <QDebug> 等）

class WarningLamp; // 前置声明，避免循环包含
// [差异] 第二个文件除了前置声明，还 #include "WarningLamp.h"

class QmSerial : public QObject
{
    Q_OBJECT
public:
    explicit QmSerial(QObject* parent = nullptr); // [差异] 构造函数带 QObject* parent 且为 explicit；第二个文件构造函数无参数
    ~QmSerial();

    void SetParameter(QString port);

    // 业务动作
    void ReadStatus();
    void CleanLampAll();
    void CleanLamp(QString &num);
    void FlashLamp(QString &num);
    void FlashLamp5s();

    // 状态获取/设置（兼容原接口）
    QString getStatus();
    void setStatus(QString strStatus);

    // 串口开关
    bool open_port();
    bool close_port();

    // 工具
    static QString QByteArray_to_QString(QByteArray valu);
    // [差异] 这里是 static；第二个文件中对应方法为非 static
    // [差异] 第二个文件还多了 QByteArray_to_Int()

    // 兼容原接口
    void setWarning(WarningLamp *warning);

signals:
    // 新增：串口状态解析完成时发出（ONLINE/CLEAN/OFFLINE/WARNING）
    void statusChanged(const QString& status);
    // [差异] 本文件定义了 signal；第二个文件没有任何 signals

private slots:
    void onReadyRead();
    // [差异] 本文件使用 Qt slots 并命名为 onReadyRead；第二个文件是 public: void ReadyRead();

private:
    // 解析分发
    void tryParseFrames();
    // [差异] 仅本文件有，第二个文件没有该私有解析分发函数

    // 各状态处理
    QString handleReadStatusResponse(const QByteArray &frame);
    QString handleCleanLampResponse(const QByteArray &frame);
    QString handleFlashLampResponse(const QByteArray &frame);
    QString handleFlashLamp5sResponse(const QByteArray &frame);
    // [差异] 这些处理函数在本文件是 private，且参数为 const QByteArray&；
    //       第二个文件把它们放在 public，且参数为按值传递 QByteArray

private:
    enum class State {
        Idle = 0, // [差异] 明确赋 0；第二个文件未显式赋值
        WaitingForReadStatusResponse,
        WaitingForCleanLampResponse,
        WaitingForFlashLampResponse,    // [差异] 本文件顺序：Flash 在前，Flash5s 在后；第二个文件顺序相反
        WaitingForFlashLamp5sResponse
    };

    QSerialPort* m_serial = nullptr; // [差异] 带默认初始化 nullptr；第二个文件未给默认值
    QByteArray   m_rxBuffer;         // [差异] 缓冲区命名为 m_rxBuffer；第二个文件命名为 buffer

    QString status = "STOP";         // [差异] 默认值为 "STOP"；第二个文件未给默认值
    int interval = 2000;             // [差异] 默认 2000ms；第二个文件未给默认值

    State currentState = State::Idle; // [差异] 默认状态 Idle；第二个文件未给默认值

    WarningLamp* warninglamp = nullptr; // [差异] 默认 nullptr；第二个文件未给默认值
    // [差异] 第二个文件还有 QTimer* readyTimer；本文件没有
    // [差异] 第二个文件还有 Start()/receiveData()/xorChecksum()/ReadData()/ReadyRead() 等额外接口；本文件没有
};
