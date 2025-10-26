#include "tempsensor.h"

TempSensor::TempSensor(QObject *parent) : QObject(parent),
    m_status("OFFLINE"),
    m_effDt(""),
    m_moduleNo(""),
    m_moduleName(""),
    // m_physloc(""),
    m_alarmID(0)
{
}

TempSensor::TempSensor(const QString &moduleNo, const QString &moduleName, QObject *parent) :
    QObject(parent),
    m_status("OFFLINE"),
    m_effDt(""),
    m_moduleNo(moduleNo),
    m_moduleName(moduleName),
    // m_physloc(""),
    m_alarmID(0)
{
}


// Getter 实现
QString TempSensor::num()  { return m_num; }
QString TempSensor::status() const { return m_status; }
QString TempSensor::effDt() const { return m_effDt; }
QString TempSensor::moduleNo() const { return m_moduleNo; }
QString TempSensor::moduleName() const { return m_moduleName; }
// QString TempSensor::physloc() const { return m_physloc; }
int TempSensor::alarmID() const { return m_alarmID; }

// Setter 实现
void TempSensor::setStatus(const QString &status)
{
    if(m_status != status) {
        m_status = status;
        emit statusChanged(m_status);
    }
}

void TempSensor::setNum(const QString &num) { m_num = num; }
void TempSensor::setEffDt(const QString &effDt) { m_effDt = effDt; }
void TempSensor::setModuleNo(const QString &moduleNo) { m_moduleNo = moduleNo; }
void TempSensor::setModuleName(const QString &moduleName) { m_moduleName = moduleName; }
// void TempSensor::setPhysloc(const QString &physloc) { m_physloc = physloc; }
void TempSensor::setAlarmID(int alarmID) { m_alarmID = alarmID; }


// TempSensor.cpp - add these implementations
void TempSensor::setButton(QPushButton* button)
{
    m_button = button;
}

void TempSensor::resetWarning(const QString &mode, QSqlDatabase &db){
    QSqlQuery q(db);
    q.prepare("update AlarmBeep set ResetTime=getdate(), ResetMode=? where id=?");
    q.addBindValue(mode);
    q.addBindValue(alarmID());
    q.exec();
    online();
}

bool TempSensor::checkWarning(QSqlDatabase &db, QString &mainStatus)
{
    if (mainStatus == "OFFLINE") return false;

    QSqlQuery q(db);
    q.prepare("select top 2 ModuleNo, EffDt, WarnText, Info "
              "from Alarm where EffDt >= ? and ModuleNo = ? order by EffDt desc");
    q.addBindValue(m_effDt);
    q.addBindValue(m_moduleNo);

    if(!q.exec() || !q.first()) {
        return false;
    }

    QString ModuleNo = q.value(0).toString();
    QString EffDt    = q.value(1).toString().right(23);
    QString WarnText = q.value(2).toString().left(12);
    QString Info     = q.value(3).toString().left(10);

    QString str = WarnText.replace('\"', " ").trimmed();

    if (str == "回归正常") {
        if (status()== "WARNING"){
            QSqlQuery qu(db);
            qu.prepare("update AlarmBeep set ResetTime=getdate(), ResetMode='REGRESS' where id=?");
            qu.addBindValue(m_alarmID);
            qu.exec();

            setStatus("ONLINE");
            emit statusNormal();
        }
    } else {
        if (EffDt != m_effDt &&
            (str == "温度越下限" || str == "温度越下下限" ||
             str == "温度越上限" || str == "温度越上上限")) {

            QSqlQuery qi(db);
            qi.prepare("insert into AlarmBeep (ModuleNo, EffDt, WarnText, Info) values(?,?,?,?)");
            qi.addBindValue(ModuleNo);
            qi.addBindValue(EffDt);
            qi.addBindValue(WarnText);
            qi.addBindValue(Info);

            if(!qi.exec()) {
                qDebug() << "Failed to insert into AlarmBeep:" << qi.lastError().text();
                return false;
            } else {
                m_effDt = EffDt;
                m_alarmID = qi.lastInsertId().toInt();
            }

            setStatus("WARNING");
            emit warningDetected();
        }
    }
    return true;
}


void TempSensor::warning()
{
    // if (m_button) {
    //     QString button_style_red = "QPushButton{background-color:red; "
    //                                "color: white; border-radius: 10px; border: 2px groove gray; border-style: outset;}";
    //     m_button->setStyleSheet(button_style_red);
    // }
}

void TempSensor::online()
{
    // if (m_button) {
    //     QString button_style_green = "QPushButton{background-color:green; "
    //                                  "color: white; border-radius: 10px; border: 2px groove gray; border-style: outset;}";
    //     m_button->setStyleSheet(button_style_green);
    // }
}

void TempSensor::offline()
{
    // if (m_button) {
    //     QString button_style_gray = "QPushButton{background-color:gray; "
    //                                 "color: white; border-radius: 10px; border: 2px groove gray; border-style: outset;}";
    //     m_button->setStyleSheet(button_style_gray);
    // }
}
