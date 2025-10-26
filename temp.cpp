#include <QObject>
#include <QSerialPort>
#include <QByteArray>
#include <QString>

class WarningLamp; // 前置声明，避免循环包含

class QmSerial : public QObject
{
    Q_OBJECT
public:
    explicit QmSerial(QObject* parent = nullptr);
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

    // 兼容原接口
    void setWarning(WarningLamp *warning);

signals:
    // 新增：串口状态解析完成时发出（ONLINE/CLEAN/OFFLINE/WARNING）
    void statusChanged(const QString& status);

private slots:
    void onReadyRead();

private:
    // 解析分发
    void tryParseFrames();

    // 各状态处理
    QString handleReadStatusResponse(const QByteArray &frame);
    QString handleCleanLampResponse(const QByteArray &frame);
    QString handleFlashLampResponse(const QByteArray &frame);
    QString handleFlashLamp5sResponse(const QByteArray &frame);

private:
    enum class State {
        Idle = 0,
        WaitingForReadStatusResponse,
        WaitingForCleanLampResponse,
        WaitingForFlashLampResponse,
        WaitingForFlashLamp5sResponse
    };

    QSerialPort* m_serial = nullptr;
    QByteArray   m_rxBuffer;

    QString status = "STOP";
    int interval = 2000;

    State currentState = State::Idle;

    WarningLamp* warninglamp = nullptr;
};
