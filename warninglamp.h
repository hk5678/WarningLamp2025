#ifndef WARNINGLAMP_H
#define WARNINGLAMP_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QString>
#include <QTimer>

class TempSensor;
class QmSerial;

namespace Ui {
class WarningLamp;
}

// 默认采样间隔（毫秒），如你已有全局定义可移除本宏
#ifndef SAMPLING_INTERVAL
#define SAMPLING_INTERVAL 1000
#endif

class WarningLamp : public QMainWindow
{
    Q_OBJECT
public:
    explicit WarningLamp(QWidget *parent = nullptr);
    ~WarningLamp() override;

    void setStatus(const QString &status);

private:
    // 初始化与配置
    void readSetting();
    void connectDB(const QString &host, const QString &port,
                   const QString &data, const QString &user, const QString &pwd);
    void checkInit();

    // 串口联动与显示
    void addListStr(const QString &listStr);
    void flashLamp(QString num);
    void flashLamp5s();
    void cleanLamp(QString num);
    void cleanLampAll();

    // DB 操作
    void resetWarning(const QString &mode, const QString &moduleNo);

    // UI 状态
    void warning();
    void online();
    void offline();

    // 报警检查
    void checkWarning();

private slots:
    // 新增：串口状态变更的统一入口（由 QmSerial 发出）
    void onSerialStatusChanged(const QString& status);

    // 兼容：旧的定时/延时方式，保留但不再强依赖
    void checkSerialTimeout();
    void doStatus();

    // 按钮槽
    void on_btnStart_clicked();
    void on_btnHigh_clicked();
    void on_btnLow_clicked();
    void on_btnClean_clicked();
    void on_btnBrowse_clicked();

    // 传感器信号槽
    void onWarningDetected();
    void onStatusNormal();

private:
    Ui::WarningLamp* ui = nullptr;

    int OverTime;

    QSqlDatabase m_db;
    TempSensor*  m_hSensor = nullptr;
    TempSensor*  m_mSensor = nullptr;
    TempSensor*  m_lSensor = nullptr;
    QmSerial*    m_qmSerial = nullptr;

    QString m_mainStatus = "OFFLINE";
    QString m_labelCom;
    QString m_labelQm;

    // 预留的初始信息（如需展示/校验，可继续使用）
    QString m_moduleNo0;
    QString m_effDt0;

    // 定时器
    QTimer* m_checkSerial = nullptr;
    QTimer* m_checkTemp   = nullptr;
};

#endif // WARNINGLAMP_H
