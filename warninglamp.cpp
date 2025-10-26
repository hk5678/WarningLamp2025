#include "warninglamp.h"
#include "ui_warninglamp.h"

#include "tempsensor.h"
#include "qmserial.h"
#include "historywin.h"

#include <QCoreApplication>
#include <QSettings>
#include <QFile>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

namespace {
const char* kStyleGray  = "QPushButton{background-color:gray;  color: white; border-radius: 10px; border: 2px groove gray; border-style: outset;}";
const char* kStyleGreen = "QPushButton{background-color:green; color: white; border-radius:  10px; border: 2px groove gray; border-style: outset;}";
const char* kStyleRed   = "QPushButton{background-color:red;   color: white; border-radius: 10px; border: 2px groove gray; border-style: outset;}";
}

WarningLamp::WarningLamp(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::WarningLamp),
    m_hSensor(new TempSensor(this)),
    m_mSensor(new TempSensor(this)),
    m_lSensor(new TempSensor(this)),
    m_qmSerial(new QmSerial(this)),
    m_mainStatus("OFFLINE")
{
    ui->setupUi(this);

    // 传按钮到传感器（用于改变颜色）
    m_hSensor->setButton(ui->btnHigh);
    m_lSensor->setButton(ui->btnLow);

    // 串口对象回调指向本窗口（兼容接口）
    m_qmSerial->setWarning(this);

    // 初始按钮风格
    ui->btnStart->setStyleSheet(kStyleGray);
    ui->btnClean->setStyleSheet(kStyleGray);
    ui->btnBrowse->setStyleSheet(kStyleGray);

    // 预设
    m_moduleNo0 = "淬火";
    m_effDt0    = "2025-01-01";

    // 读配置、初始化
    readSetting();
    checkInit();

    QTimer::singleShot(100, this, &::WarningLamp::cleanLampAll);
    QTimer::singleShot(300, this, &WarningLamp::flashLamp5s);

    // 串口定时轮询改为直接触发 ReadStatus（由 QmSerial 状态变化信号驱动 UI）
    m_checkSerial = new QTimer(this);
    m_checkSerial->setInterval(SAMPLING_INTERVAL*2);
    m_checkSerial->setTimerType(Qt::PreciseTimer);
    connect(m_checkSerial, &QTimer::timeout, m_qmSerial, &QmSerial::ReadStatus);
    m_checkSerial->start();

    // 订阅串口解析完成后的状态变化
    connect(m_qmSerial, &QmSerial::statusChanged,
            this, &WarningLamp::onSerialStatusChanged);

    // 定时查询报警
    m_checkTemp = new QTimer(this);
    m_checkTemp->setInterval(SAMPLING_INTERVAL * 2);
    m_checkTemp->setTimerType(Qt::PreciseTimer);
    connect(m_checkTemp, &QTimer::timeout, this, &WarningLamp::checkWarning);
    m_checkTemp->start();

    // 传感器信号
    connect(m_hSensor, &TempSensor::warningDetected, this, &WarningLamp::onWarningDetected);
    connect(m_hSensor, &TempSensor::statusNormal,    this, &WarningLamp::onStatusNormal);
    connect(m_mSensor, &TempSensor::warningDetected, this, &WarningLamp::onWarningDetected);
    connect(m_mSensor, &TempSensor::statusNormal,    this, &WarningLamp::onStatusNormal);
    connect(m_lSensor, &TempSensor::warningDetected, this, &WarningLamp::onWarningDetected);
    connect(m_lSensor, &TempSensor::statusNormal,    this, &WarningLamp::onStatusNormal);
}

WarningLamp::~WarningLamp()
{
    delete ui;
    // 其余子对象 m_qmSerial/m_hSensor/m_mSensor/m_lSensor/m_checkSerial/m_checkTemp
    // 因为设置了 parent，会随窗口销毁自动清理
}

void WarningLamp::readSetting()
{
    const QString fileName = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(fileName, QSettings::IniFormat);

    // settings.setIniCodec(QTextCodec::codecForName("UTF-8"));

    if (!QFile::exists(fileName)) {
        QMessageBox::warning(this, "配置文件", "未找到配置文件 config.ini，使用默认配置。");
        return;
    }

    // DB
    const QString ip       = settings.value("dbconn/ip").toString();
    const QString port     = settings.value("dbconn/port").toString();
    const QString dbname   = settings.value("dbconn/db").toString();
    const QString user     = settings.value("dbconn/user").toString();
    const QString password = settings.value("dbconn/password").toString();
    connectDB(ip, port, dbname, user, password);

    // UI 标签
    m_labelCom = settings.value("Line1/COM").toString();
    m_labelQm  = settings.value("Line1/QM").toString();

    const QString labelText =
        "<html><head/><body><p><span style=\" font-weight:700; color:#00557f;\">"
        + m_labelQm + " Warning Lamp</span></p></body></html>";
    ui->label->setText(labelText);

    // 传感器配置
    m_hSensor->setModuleNo(settings.value("Line1/HModuleNo").toString());
    m_hSensor->setModuleName("淬火");
    m_hSensor->setNum("DO2");

    m_lSensor->setModuleNo(settings.value("Line1/LModuleNo").toString());
    m_lSensor->setModuleName("回火");
    m_lSensor->setNum("DO3");

    // 如需中温，解开以下
    // m_mSensor->setModuleNo(settings.value("Line1/MModuleNo").toString());
    // m_mSensor->setModuleName("中温");
    // m_mSensor->setNum("DO3");

    // 串口
    if (!m_labelCom.isEmpty()) {
        m_qmSerial->SetParameter(m_labelCom);
    } else {
        qWarning() << "COM 配置为空，未初始化串口";
    }
}

void WarningLamp::connectDB(const QString &host, const QString &port,
                            const QString &data, const QString &user, const QString &pwd)
{
    // 指定连接名避免覆盖默认连接
    if (!m_db.isValid()) {
        m_db = QSqlDatabase::addDatabase("QODBC", "main-odbc");
    }
    const QString conString = QString("DRIVER={SQL SERVER};SERVER=%1,%2;DATABASE=%3;UID=%4;PWD=%5;")
                                  .arg(host, port, data, user, pwd);
    m_db.setDatabaseName(conString);

        qDebug()<< host<<data<<user<<pwd;
    if(!m_db.open()) {
        QMessageBox::warning(this, "数据库连接", "不能连接到ODBC服务器！");
        qDebug() << "连接失败" << m_db.lastError().text();
        close();
    } else {
        qDebug() << "Good! Connected!";
    }
}

void WarningLamp::checkWarning()
{
    qDebug() << "checkWarning............";
    m_hSensor->checkWarning(m_db, m_mainStatus);
    // m_mSensor->checkWarning(m_db, m_mainStatus);
    m_lSensor->checkWarning(m_db, m_mainStatus);
}

void WarningLamp::checkInit()
{
    const QString EffDt = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    m_hSensor->setEffDt(EffDt);
    // m_mSensor->setEffDt(EffDt);
    m_lSensor->setEffDt(EffDt);
}

void WarningLamp::onSerialStatusChanged(const QString& listStr)
{
    qDebug() << "Serial status changed:" << listStr << " main=" << m_mainStatus;

    if (m_mainStatus == "ONLINE" && listStr == "OFFLINE") {
        qDebug("从OFFLINE到ONLINE");
        m_mainStatus = "OFFLINE";
        m_hSensor->setStatus("OFFLINE");
        // m_mSensor->setStatus("OFFLINE");
        m_lSensor->setStatus("OFFLINE");
        addListStr(listStr);
        offline();
        return;
    }

    if (m_mainStatus == "WARNING") {
        if (listStr == "CLEAN") {
              qDebug("清除警告！");
            QTimer::singleShot(100, this, &WarningLamp::cleanLampAll);
            if (m_hSensor->status() == "WARNING") {
                m_hSensor->resetWarning(listStr, m_db);
                m_hSensor->setStatus("ONLINE");
            }
            if (m_lSensor->status() == "WARNING") {
                m_lSensor->resetWarning(listStr, m_db);
                m_lSensor->setStatus("ONLINE");
            }
            addListStr(listStr);
            return;
        } else if (listStr == "OFFLINE") {
            qDebug("直接下线！");
            QTimer::singleShot(100, this, &WarningLamp::cleanLampAll);
            m_mainStatus = "OFFLINE";
            resetWarning("OFFLINE", m_hSensor->moduleNo());
            // resetWarning("OFFLINE", m_mSensor->moduleNo());
            resetWarning("OFFLINE", m_lSensor->moduleNo());
            addListStr(listStr);
            offline();
            return;
        }
    }

    if (m_mainStatus == "OFFLINE" && listStr == "ONLINE") {
        m_mainStatus = "ONLINE";
        m_hSensor->setStatus("ONLINE");
        // m_mSensor->setStatus("ONLINE");
        m_lSensor->setStatus("ONLINE");
        checkInit();
        online();
        flashLamp5s();
        addListStr(listStr);
        return;
    }

    // 其他组合按需处理
}

void WarningLamp::checkSerialTimeout()
{
    // 兼容旧逻辑：仅触发 ReadStatus
    if (m_qmSerial) m_qmSerial->ReadStatus();
}

void WarningLamp::doStatus()
{
    // 兼容旧逻辑：取一次状态并复用新入口
    if (!m_qmSerial) return;
    const QString s = m_qmSerial->getStatus();
    onSerialStatusChanged(s);
}

void WarningLamp::addListStr(const QString &listStr)
{
    ui->listWidget->addItem(QDateTime::currentDateTime().toString("hh:mm:ss") + " " + listStr + "   " + m_mainStatus);

    const int maxItemCount = 50;
    if (ui->listWidget->count() > maxItemCount) {
        const int removeCount = ui->listWidget->count() - maxItemCount;
        for (int i = 0; i < removeCount; ++i) {
            QListWidgetItem *item = ui->listWidget->takeItem(0);
            delete item;
        }
    }
    ui->listWidget->scrollToBottom();
}

void WarningLamp::flashLamp(QString num)
{
    if (m_qmSerial) {
        QString lampOut="DO1";
        // m_qmSerial->FlashLamp(lampOut);
        m_qmSerial->FlashLamp(num);
    }
}

void WarningLamp::flashLamp5s()
{
    // QString lampOut="DO1";
    if (m_qmSerial){

        m_qmSerial->FlashLamp5s();

    }

}

void WarningLamp::cleanLamp(QString num)
{
    if (m_qmSerial) {
        m_qmSerial->CleanLamp(num);

    }
}

void WarningLamp::cleanLampAll()
{
    if (m_qmSerial) m_qmSerial->CleanLampAll();
}

void WarningLamp::resetWarning(const QString &mode, const QString &moduleNo)
{
    int alarmID = 0;
    if (moduleNo == m_hSensor->moduleNo()) {
        alarmID = m_hSensor->alarmID();
    } else if (moduleNo == m_lSensor->moduleNo()) {
        alarmID = m_lSensor->alarmID();
    } else {
        // alarmID = m_mSensor->alarmID();
    }

    QSqlQuery q(m_db);
    q.prepare("update AlarmBeep set ResetTime=getdate(), ResetMode=? where id=?");
    q.addBindValue(mode);
    q.addBindValue(alarmID);
    if (!q.exec()) {
        qWarning() << "resetWarning failed:" << q.lastError().text()
        << " mode=" << mode << " id=" << alarmID;
    }
}

void WarningLamp::setStatus(const QString &status)
{
    m_mainStatus = status;
}

void WarningLamp::on_btnStart_clicked()
{
    qDebug() << "CLICK00----" << m_mainStatus;

    if (m_mainStatus == "ONLINE") {
        qDebug() << "CLICK05----" << m_mainStatus;
        m_mainStatus = "OFFLINE";
        m_hSensor->offline();
        // m_mSensor->offline();
        m_lSensor->offline();
        offline();
        addListStr("OFFLINE");
    } else if (m_mainStatus == "OFFLINE") {
        qDebug() << "CLICK08----" << m_mainStatus;
        m_mainStatus = "ONLINE";
        checkInit();
        flashLamp5s();
        online();
        m_hSensor->online();
        // m_mSensor->online();
        m_lSensor->online();
        addListStr("ONLINE");
    }
    qDebug() << "CLICK10----" << m_mainStatus;
}

void WarningLamp::on_btnHigh_clicked()
{
    if (m_mainStatus == "OFFLINE") return;


    QTimer::singleShot(100, this, &WarningLamp::cleanLampAll);
    m_hSensor->resetWarning("CLICK", m_db);
    m_hSensor->setStatus("ONLINE");
    addListStr("CLEAN");
}

void WarningLamp::on_btnLow_clicked()
{
    if (m_mainStatus == "OFFLINE") return;

    QTimer::singleShot(100, this, &WarningLamp::cleanLampAll);
    m_lSensor->resetWarning("CLICK", m_db);
    m_lSensor->setStatus("ONLINE");
    addListStr("CLEAN");
}


void WarningLamp::onWarningDetected()
{
    if (auto* senderSensor = qobject_cast<TempSensor*>(sender())) {
        senderSensor->warning();

        // 根据传感器绑定的通道决定输出哪一路
        QString num = senderSensor->num(); //""DO1"(仪表灯) "DO2"（高温）或 "DO3"（低温）

        // 立即打开对应 DO
        QTimer::singleShot(200, this, [this, num](){
       this->flashLamp(num);
        });

        QString lampout="DO1";
        QTimer::singleShot(400, this, [this, lampout](){
            this->flashLamp(lampout);
        });

        // m_qmSerial->FlashLamp(lampout);

        // m_qmSerial->FlashLamp(num);

        // 如果希望保持5秒后自动关闭该 DO，保留以下延时清除；
        // 如果希望人工确认后再清除，删掉下面这两行即可。
        // QTimer::singleShot(5000, this, [this, num](){
        //     this->cleanLamp(num); // 关闭刚才打开的那一路 DO
        // });
    }

    m_mainStatus = "WARNING";
    warning(); // 改变启动按钮样式为红色
}

void WarningLamp::onStatusNormal(){

    if (auto* senderSensor = qobject_cast<TempSensor*>(sender())) {
        senderSensor->online();
        // 如需只清除本路 DO，使用如下两行；否则保留 cleanLampAll()
        QString num = senderSensor->num();
        QTimer::singleShot(200, this, [this, num]{ this->cleanLamp(num); });
           qDebug()<<"On Status Normal "<< num;
     }


    // QTimer::singleShot(100, this, &WarningLamp::cleanLampAll); // 保持原有行为
    if (m_hSensor->status()=="ONLINE" && m_lSensor->status()=="ONLINE") {


        m_mainStatus="ONLINE";
         QTimer::singleShot(400, this, &WarningLamp::cleanLampAll);
        // QString lampout="DO1";
        // QTimer::singleShot(400, this, [this, lampout](){
        //     this->cleanLamp(lampout);
        // });
        online();
    }

}


void WarningLamp::warning()
{
    if (ui->btnStart) {
        ui->btnStart->setStyleSheet(kStyleRed);
    }
}

void WarningLamp::online()
{
    if (ui->btnStart) {
        qDebug() << "0.5st: ONLINE";
        // ui->btnStart->setStyleSheet(kStyleGreen);
    }
    ui->btnStart->setStyleSheet(kStyleGreen);
    m_hSensor->online();
    m_lSensor->online();
    qDebug() << "1st: ONLINE";
}

void WarningLamp::offline()
{
    if (ui->btnStart) {
        ui->btnStart->setStyleSheet(kStyleGray);
    }
    ui->btnStart->setStyleSheet(kStyleGray);
    m_hSensor->offline();
    m_lSensor->offline();
    qDebug() << "2nd: OFFLINE";
}

void WarningLamp::on_btnClean_clicked()
{
    if (m_mainStatus == "WARNING") {
        m_mainStatus = "ONLINE";
        online();
    }
    if (m_mainStatus == "OFFLINE") return;

    QTimer::singleShot(100, this, &WarningLamp::cleanLampAll);

    if (m_hSensor->status() == "WARNING") {
        m_hSensor->resetWarning("CLEANALL", m_db);
        m_hSensor->setStatus("ONLINE");
    }
    if (m_lSensor->status() == "WARNING") {
        m_lSensor->resetWarning("CLEANALL", m_db);
        m_lSensor->setStatus("ONLINE");
    }
    // if (m_mSensor->status() == "WARNING") {
    //     m_mSensor->resetWarning("CLEANALL", m_db);
    //     m_mSensor->setStatus("ONLINE");
    // }

    addListStr("CLEANALL");
}

void WarningLamp::on_btnBrowse_clicked()
{
    auto* h1 = new HistoryWin();
    h1->setDB(m_db);
    qDebug() << "Q&L=" << m_hSensor->moduleNo() << "   " << m_lSensor->moduleNo();
    h1->toModuleNo(m_hSensor->moduleNo(), m_mSensor->moduleNo(), m_lSensor->moduleNo());
    h1->show();
}
