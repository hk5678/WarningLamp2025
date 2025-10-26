#include "qm_com.h"
#include "ui_qm_com.h"

Qm_Com::Qm_Com(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Qm_Com)
{
    ui->setupUi(this);
}

Qm_Com::~Qm_Com()
{
    delete ui;
}

void Qm_Com::setComQM(QString qm_name, int com_name)
{ //初始化数据显示
    ui->qmEdit->setText(qm_name);
    ui->comEdit->setValue(com_name);


}

int Qm_Com::getCom( )
{ //初始化数据显示
    return  ui->comEdit->value( );


}
QString Qm_Com::getQM( )
{ //初始化数据显示
    return ui->qmEdit->text();


}
