#ifndef QM_COM_H
#define QM_COM_H

#include <QDialog>

namespace Ui {
class Qm_Com;
}

class Qm_Com : public QDialog
{
    Q_OBJECT

public:
    explicit Qm_Com(QWidget *parent = nullptr);
    ~Qm_Com();


    void setComQM(QString qm_name, int port_name) ;
    int getCom( );
    QString getQM( );




private:
    Ui::Qm_Com *ui;
};

#endif // QM_COM_H
