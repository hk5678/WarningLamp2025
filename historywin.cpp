#include "historywin.h"
#include "ui_historywin.h"

HistoryWin::HistoryWin(QWidget *parent )
    : QMainWindow(parent),
      ui(new Ui::HistoryWin)
{
    ui->setupUi(this);
    aDay=QDate::currentDate();

    QButtonGroup* btnGroup1 = new QButtonGroup(this);
    btnGroup1->setExclusive(true);
    btnGroup1->addButton(ui->chButton, 0);
    btnGroup1->addButton(ui->hhButton, 1);
}

HistoryWin::~HistoryWin(  )
{
    delete ui;

}

void HistoryWin::setDB(QSqlDatabase db0  )
{

        db = db0; // 复制连接句柄
        if (!db.isOpen() && !db.open()) {
            qWarning() << "Open DB failed:" << db.lastError().text();
            ui->InfoBar->setPlainText("数据库打开失败: " + db.lastError().text());
            return;
        }
        getDataByDay(aDay);




}
void HistoryWin::toModuleNo(QString h,QString m, QString l){

    defaultModuleNo=h ;
    HModuleNo=h;
    LModuleNo=l;

};

void HistoryWin::on_calendarWidget_clicked(const QDate &date)
{
    aDay=date;
    getDataByDay(aDay);
    // 假设 tableWidget 是你的 QTableWidget 实例
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    return;


}




void HistoryWin::getDataByDay(const QDate &date)
{
    if (!db.isValid() || !db.isOpen()) {
        ui->InfoBar->setPlainText("数据库未连接或未打开");
        qWarning() << "DB invalid or not open";
        return;
    }

    // 单日范围：[start, end)
    const QDateTime start(date, QTime(0,0,0));
    const QDateTime end = start.addDays(2);

    QSqlQuery query(db);
    query.prepare(
        "SELECT ModuleNo, EffDt, WarnText, Info, id, ResetMode, "
        "       CASE WHEN ResetTime IS NULL OR ResetTime < '2000-01-01' "
        "            THEN NULL ELSE ResetTime END AS ResetTime, "
        "       CASE WHEN ResetTime IS NULL OR ResetTime < '2000-01-01' "
        "            THEN NULL ELSE DATEDIFF(MINUTE, EffDt, ResetTime) END AS WaitTime "
        "FROM AlarmBeep "
        "WHERE ModuleNo = ? AND EffDt >= ? AND EffDt < ? "
        "ORDER BY EffDt DESC"
        );

    // ModuleNo 既可能是数字也可能是字符，直接绑定 QString 由驱动转换
    query.addBindValue(defaultModuleNo);
    query.addBindValue(start); // 让驱动传递为 DATETIME
    query.addBindValue(end);

    if (!query.exec()) {
        const QString err = query.lastError().text();
        ui->InfoBar->setPlainText("查询失败: " + err);
        qWarning() << "Query failed:" << err;
        return;
    }

    // UI 填充前关闭刷新/排序，避免闪烁和卡顿
    ui->tableWidget->setUpdatesEnabled(false);
    ui->tableWidget->setSortingEnabled(false);

    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);
    ui->tableWidget->setColumnCount(8);

    QStringList headers = {"序号ID", "线号", "越线", "报警", "参考值", "恢复", "持续(m)", "方式"};
    ui->tableWidget->setHorizontalHeaderLabels(headers);

    int row = 0;
    while (query.next()) {
        ui->tableWidget->insertRow(row);

        const QVariant effVar = query.value("EffDt");
        const QDateTime effDt = effVar.toDateTime();
        const QString effStr = effDt.isValid()
                                   ? effDt.toString("yyyy-MM-dd HH:mm:ss")
                                   : effVar.toString().left(19); // 兜底格式化

        const QVariant rVar = query.value("ResetTime");
        const QString resetStr = rVar.isNull()
                                     ? QString()
                                     : rVar.toDateTime().toString("yyyy-MM-dd HH:mm:ss");

        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(query.value("id").toString()));
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(query.value("ModuleNo").toString()));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(effStr));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(query.value("WarnText").toString()));
        ui->tableWidget->setItem(row, 4, new QTableWidgetItem(query.value("Info").toString()));
        ui->tableWidget->setItem(row, 5, new QTableWidgetItem(resetStr));
        ui->tableWidget->setItem(row, 6, new QTableWidgetItem(query.value("WaitTime").toString()));
        ui->tableWidget->setItem(row, 7, new QTableWidgetItem(query.value("ResetMode").toString()));
        ++row;
    }

    // 友好信息展示
    ui->InfoBar->setPlainText(
        QString("模块: %1   日期: %2   记录数: %3")
            .arg(defaultModuleNo)
            .arg(date.toString("yyyy-MM-dd"))
            .arg(row)
        );

    // 表格属性和自适应
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    ui->tableWidget->setSortingEnabled(true);
    ui->tableWidget->setUpdatesEnabled(true);
    ui->tableWidget->setVisible(true);
}




void HistoryWin::on_chButton_clicked()
{
    defaultModuleNo=HModuleNo;

    getDataByDay(aDay);
}


void HistoryWin::on_hhButton_clicked()
{
    defaultModuleNo=LModuleNo;

        getDataByDay(aDay);
}


