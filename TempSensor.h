#ifndef TEMPSENSOR_H
#define TEMPSENSOR_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDebug>
#include <QSqlError>
#include <QPushButton>  // Add this include

class TempSensor : public QObject
{
    Q_OBJECT
public:
    explicit TempSensor(QObject *parent = nullptr);
    TempSensor(const QString &moduleNo, const QString &moduleName, QObject *parent = nullptr);

    // Getters
    QString num() ;
    QString status() const;
    QString effDt() const;
    QString moduleNo() const;
    QString moduleName() const;
    // QString physloc() const;
    int alarmID() const;

    // Setters
    void setStatus(const QString &status);
    void setNum(const QString &num);
    void setEffDt(const QString &effDt);
    void setModuleNo(const QString &moduleNo);
    void setModuleName(const QString &moduleName);
    // void setPhysloc(const QString &physloc);
    void setAlarmID(int alarmID);
    void setButton(QPushButton* button);  // Add this method
    void resetWarning(const QString &mode,QSqlDatabase &db);
    // Check warning method
    bool checkWarning(QSqlDatabase &db, QString &mainStatus);



    void online();
    void offline();
    void warning();

signals:
    void warningDetected();  // 检测到警告信号
    void statusNormal();     // 状态恢复正常信号
    void statusChanged(const QString &status); // 状态变化信号

private:
    QString m_num;
    QString m_status;
    QString m_effDt;
    QString m_moduleNo;
    QString m_moduleName;
    // QString m_physloc;
    int m_alarmID;
    QPushButton* m_button;  // Add this member

};

#endif // TEMPSENSOR_H
