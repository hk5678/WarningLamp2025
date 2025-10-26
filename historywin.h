#ifndef HISTORYWIN_H
#define HISTORYWIN_H
#include <QMainWindow>
#include <QDate>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMessageBox>
#include <QSqlError>
#include <QTableWidget>
#include <QHeaderView>
#include <QSettings>
#include <QButtonGroup>

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

QT_BEGIN_NAMESPACE
namespace Ui {
class HistoryWin;
}

QT_END_NAMESPACE

class HistoryWin :  public QMainWindow
{
    Q_OBJECT

public:
    HistoryWin(QWidget *parent = nullptr   );
    ~HistoryWin();

    void setDB(QSqlDatabase);
    void getDataByDay(const QDate &date);
    void toModuleNo(QString h,QString m,QString l);

private slots:

    void on_calendarWidget_clicked(const QDate &date);


    void on_chButton_clicked();

    void on_hhButton_clicked();

private:
    Ui::HistoryWin *ui;
    QDate aDay;
    QSqlDatabase db;

    QSettings *setting;
    QString defaultModuleNo,HModuleNo, LModuleNo;

};
#endif // HISTORYWIN_H
