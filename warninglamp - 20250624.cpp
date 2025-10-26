#include "warninglamp.h"
#include "ui_warninglamp.h"


WarningLamp::WarningLamp(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::WarningLamp)
    , qmser(new QmSerial())
{
    ui->setupUi(this);
    qmser->setWarning(this);
    mainStatus="OFFLINE";


//    QString button_style="QPushButton{background-color:green;
//    color: white;   border-radius: 10px;  border: 2px groove gray; border-style: outset;}
//    QPushButton:hover{background-color:gray; color: black;}
//    QPushButton:pressed{background-color:rgb(85, 170, 255);border-style: inset; }";
    QString button_style="QPushButton{background-color:gray; \
        color: white;   border-radius: 10px;  border: 2px groove gray; border-style: outset;}";
    ui->pushButton1->setStyleSheet(button_style);

    QString ModuleNo0="淬火";
    QString EffDt0="2025-1-1";

    ReadSetting();
    CheckInit();
    QTimer::singleShot(100, this, &WarningLamp::FlashLamp5s);


//    QTimer *Beeptimer= new QTimer();
//    Beeptimer->setInterval(WarningTerm);
//    Beeptimer->setTimerType(Qt::PreciseTimer);
//    Beeptimer->start();

    QTimer *CheckSerial= new QTimer();
    CheckSerial->setInterval(SAMPLING_INTERVAL);
    CheckSerial->setTimerType(Qt::PreciseTimer);
    CheckSerial->start();

//    connect(Beeptimer,&QTimer::timeout,this,&WarningLamp::Beeptimer_timeout_slot);
    connect(CheckSerial,&QTimer::timeout,this,&WarningLamp::CheckSerial_timeout_slot);

    QTimer *CheckTemp= new QTimer();
    CheckTemp->setInterval(SAMPLING_INTERVAL*2);
    CheckTemp->setTimerType(Qt::PreciseTimer);
    CheckTemp->start();
    connect(CheckTemp,&QTimer::timeout,this,&WarningLamp::CheckWarning);

}

WarningLamp::~WarningLamp()
{
    delete ui;
    delete qmser;
}



void WarningLamp::ReadSetting(void)
{
    //文件路径+文件名
    QString fileName = QCoreApplication::applicationDirPath() + "/config.ini";
    qDebug() << fileName;
    //创建配置目标，输入文件路径，文件格式
    QSettings *setting = new QSettings(fileName , QSettings::IniFormat);
    //设置文件编码，配置文件中使用中文时，这是必须的，否则乱码
    //    setting->setIniCodec(QTextCodec::codecForName("UTF-8"));
    // 判断文件是否存在
    if(QFile::exists(fileName)){// 文件存在，读出配置项
   // 这里的setting->value的第二参数，是配置项缺省值，即当读取的配置项不存在时，读取该值
   // User是配置组，name和age是配置项
   QString databasetype = setting->value( "database/type").toString();
   QString ip = setting->value( "dbconn/ip").toString();
   QString port = setting->value( "dbconn/port").toString();
   QString dbname = setting->value( "dbconn/db").toString();
   QString table = setting->value( "dbconn/table").toString();
   QString user = setting->value( "dbconn/user").toString();
   QString password = setting->value( "dbconn/password").toString();
//   int WarningTerm = setting->value("dbconn/warning_term" ).toInt();

   //        MysqlWendu::connectDB(ip,port,db,user,password);
   connectDB(  ip,  port,  dbname,   user,  password);



    label_com = setting->value( "Line1/COM").toString();
    label_qm = setting->value( "Line1/QM").toString();
    QString lstr="<html><head/><body><p><span style=\" font-weight:700; color:#00557f;\">"+label_qm+" Warning Lamp</span></p></body></html>";
    ui->label->setText(lstr);

    HModuleNo = setting->value( "Line1/HModuleNo").toString();
    LModuleNo = setting->value( "Line1/LModuleNo").toString();

    qmser->SetParameter(label_com );

    qDebug() <<databasetype<< ip <<dbname <<table <<user<<password<<label_com;

    }
    // delete setting;
}


void  WarningLamp::connectDB(QString host,QString port,QString data, QString user,QString pwd)
{
    /*
    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC");  //连接的MYSQL的数据库驱动
    db.setHostName(host);         //主机名
    db.setPort(port);                    //端口
    db.setDatabaseName(data);          //数据库名
    db.setUserName(user);              //用户名
    db.setPassword(pwd);            //密码
  */
    QSqlDatabase db=QSqlDatabase::addDatabase("QODBC");
    QString conString=QString("DRIVER={SQL SERVER};"
                                "SERVER=%1,%2;"  //服务器名称
                                "DATABASE=%3;"  //数据库名称
                                "UID=%4;"    //账户名
                                "PWD=%5;")  //密码
                            .arg(host)
                            .arg(port)
                            .arg(data)
                            .arg(user)
                            .arg(pwd);
    db.setDatabaseName(conString);


//    db.setPort(port);
    //    db.open();
    //测试连接
    if(!db.open())
    {
   QMessageBox::warning(this,"数据库连接","不能连接到ODBC服务器！");
   qDebug()<< "连接失败"<<db.lastError().text();

                                 //        delete(ui);
                                 close();
    }
    else
    {


   qDebug() << "Good! 连接成功";
   //        QMessageBox::warning(this,"数据库连接","成功连接到ODBC服务器！");
    }
    //    db.close();


    return ;
}

void WarningLamp::CheckWarning(){
    CheckHWarning();
    CheckLWarning();
    CheckMWarning();
}

void  WarningLamp::CheckHWarning()
{

    if (mainStatus=="OFFLINE") return;
    QString qCheck="select top 1  ModuleNo,EffDt,WarnText,Info,%%PHYSLOC%% as Physloc from Alarm where EffDt>='"+HEffDt0+"' and %%PHYSLOC%%>'"+HPhysloc + "'  and  ModuleNo ="+ HModuleNo+"   order by  EffDt desc  ";

    QSqlQuery q(db);
    q.exec(qCheck);
    if (!q.first()) return;

   QString ModuleNo=q.value(0).toString();
    QString EffDt=q.value(1).toString().right(23);
   QString WarnText=q.value(2).toString().left(12);
    QString Info=q.value(3).toString().left(10);
    QString Physloc=q.value(4).toString();
   // AlarmID=q.value(4).toInt();
    qDebug()<<"1-Check--"<<qCheck<<WarnText<<"---"<<mainStatus<<"  temp="<<Info<<"  PhysLoc=" <<Physloc;
   //复制数据到AlarmBeep
    qCheck="insert into AlarmBeep (ModuleNo,EffDt,WarnText,Info) values( '"+ModuleNo+"', '"+EffDt +"','"+WarnText+"','"+Info+"'   ) ";
   q.exec(qCheck);
    AlarmID=q.lastInsertId().toInt();

  qDebug()<<"-----CheckWarning------";
    qDebug()<<"2-Check--"<<qCheck<<WarnText<<"---"<<mainStatus<<"  temp="<<Info;


  // ui->textEdit->append(moment.toMSecsSinceEpoch(), temp) ;
   if (ModuleNo==HModuleNo){
        ModuleNo="淬火";
   }
   if (ModuleNo==LModuleNo){
       ModuleNo="回火";
   }

   QString str=WarnText.replace('\"', " ");
   QString WarnText0 = str.trimmed();
   if ( WarnText0=="温度越下限" or WarnText0=="温度越下下限" or WarnText0=="温度越上限" or WarnText0=="温度越上上限"  ){
          qDebug()<<"3-Check--"<<WarnText ;
       if ((ModuleNo0!=ModuleNo) or (EffDt0!=EffDt)){
           //发送BeepWarning信号
           EffDt0=EffDt;
           ModuleNo0=ModuleNo;
//           CheckSerial->stop();
                  qDebug()<<"4-Check--FLASHLAMP"<<WarnText ;

               QTimer::singleShot(300, this, &WarningLamp::FlashLamp);
//               FlashLamp();

//           CheckSerial->start();

           mainStatus="WARNING";
           HStatus="WARNING";
           Warning();
       }else{
           qDebug()<<"4-Check--FLASHLAMP"<<WarnText ;

           QTimer::singleShot(300, this, &WarningLamp::FlashLamp);
            mainStatus="WARNING";
           HStatus="WARNING";
           Warning();
       }
   }else if (WarnText0=="回归正常"){
       QTimer::singleShot(100, this, &WarningLamp::CleanLamp);
       ResetWarning("REGRESS",ModuleNo);
       mainStatus="ONLINE";
       HStatus="ONLINE";
       Online();
   }

   QString DispStr=QString::number(AlarmID)+" "+ EffDt.right(12)+"  " + ModuleNo +"  " + mainStatus +"  " + Info;
   addListStr(DispStr);
   HEffDt0=EffDt;
   HAlarmID=AlarmID;

}


void  WarningLamp::CheckMWarning()
{

    if (mainStatus=="OFFLINE") return;
    QString qCheck="select top 1  ModuleNo,EffDt,WarnText,Info,%%PHYSLOC%% as Physloc from Alarm where EffDt>='"+MEffDt0+"' and %%PHYSLOC%%>'"+MPhysloc + "'  and  ModuleNo ="+ HModuleNo+"   order by  EffDt desc  ";

    QSqlQuery q(db);
    q.exec(qCheck);
    if (!q.first()) return;

    QString ModuleNo=q.value(0).toString();
    QString EffDt=q.value(1).toString().right(23);
    QString WarnText=q.value(2).toString().left(12);
    QString Info=q.value(3).toString().left(10);
    QString Physloc=q.value(4).toString();
    // AlarmID=q.value(4).toInt();
    qDebug()<<"1-Check--"<<qCheck<<WarnText<<"---"<<mainStatus<<"  temp="<<Info<<"  PhysLoc=" <<Physloc;
    //复制数据到AlarmBeep
    qCheck="insert into AlarmBeep (ModuleNo,EffDt,WarnText,Info) values( '"+ModuleNo+"', '"+EffDt +"','"+WarnText+"','"+Info+"'   ) ";


    // ui->textEdit->append(moment.toMSecsSinceEpoch(), temp) ;
    if (ModuleNo==HModuleNo){
        ModuleNo="淬火";
    }
    if (ModuleNo==LModuleNo){
        ModuleNo="回火";
    }

    QString str=WarnText.replace('\"', " ");
    QString WarnText0 = str.trimmed();
    if ( WarnText0=="温度越下限" or WarnText0=="温度越下下限" or WarnText0=="温度越上限" or WarnText0=="温度越上上限"  ){
        qDebug()<<"3-Check--"<<WarnText ;
        if ((ModuleNo0!=ModuleNo) or (EffDt0!=EffDt)){
            //发送BeepWarning信号
            EffDt0=EffDt;
            ModuleNo0=ModuleNo;
            //           CheckSerial->stop();
            qDebug()<<"4-Check--FLASHLAMP"<<WarnText ;

            QTimer::singleShot(300, this, &WarningLamp::FlashLamp);
            //               FlashLamp();

            //           CheckSerial->start();

            mainStatus="WARNING";
            MStatus="WARNING";
            Warning();
        }else{
            qDebug()<<"4-Check--FLASHLAMP"<<WarnText ;

            QTimer::singleShot(300, this, &WarningLamp::FlashLamp);
            mainStatus="WARNING";
            MStatus="WARNING";
            Warning();
        }
    }else if (WarnText0=="回归正常"){
        QTimer::singleShot(100, this, &WarningLamp::CleanLamp);
        ResetWarning("REGRESS",ModuleNo);
        mainStatus="ONLINE";
        MStatus="ONLINE";
        Online();
    }

    QString DispStr=QString::number(AlarmID)+" "+ EffDt.right(12)+"  " + ModuleNo +"  " + mainStatus +"  " + Info;
    addListStr(DispStr);
    MEffDt0=EffDt;
    MAlarmID=AlarmID;

}



void  WarningLamp::CheckLWarning()
{

    if (mainStatus=="OFFLINE") return;
    QString qCheck="select top 1  ModuleNo,EffDt,WarnText,Info,%%PHYSLOC%% as Physloc from Alarm where EffDt>='"+HEffDt0+"' and %%PHYSLOC%%>'"+LPhysloc + "'  and  ModuleNo ="+ LModuleNo+"   order by  EffDt desc  ";

    QSqlQuery q(db);
    q.exec(qCheck);
    if (!q.first()) return;

    QString ModuleNo=q.value(0).toString();
    QString EffDt=q.value(1).toString().right(23);
    QString WarnText=q.value(2).toString().left(12);
    QString Info=q.value(3).toString().left(10);
    QString Physloc=q.value(4).toString();
    // AlarmID=q.value(4).toInt();
    qDebug()<<"1-Check--"<<qCheck<<WarnText<<"---"<<mainStatus<<"  temp="<<Info<<"  PhysLoc=" <<Physloc;
    //复制数据到AlarmBeep
    qCheck="insert into AlarmBeep (ModuleNo,EffDt,WarnText,Info) values( '"+ModuleNo+"', '"+EffDt +"','"+WarnText+"','"+Info+"'   ) ";
    q.exec(qCheck);
    AlarmID=q.lastInsertId().toInt();

    qDebug()<<"-----CheckWarning------";
    qDebug()<<"2-Check--"<<qCheck<<WarnText<<"---"<<mainStatus<<"  temp="<<Info;


    // ui->textEdit->append(moment.toMSecsSinceEpoch(), temp) ;
    if (ModuleNo==HModuleNo){
        ModuleNo="淬火";
    }
    if (ModuleNo==LModuleNo){
        ModuleNo="回火";
    }

    QString str=WarnText.replace('\"', " ");
    QString WarnText0 = str.trimmed();
    if ( WarnText0=="温度越下限" or WarnText0=="温度越下下限" or WarnText0=="温度越上限" or WarnText0=="温度越上上限"  ){
        qDebug()<<"3-Check--"<<WarnText ;
        if ((ModuleNo0!=ModuleNo) or (EffDt0!=EffDt)){
            //发送BeepWarning信号
            EffDt0=EffDt;
            ModuleNo0=ModuleNo;
            //           CheckSerial->stop();
            qDebug()<<"4-Check--FLASHLAMP"<<WarnText ;

            QTimer::singleShot(300, this, &WarningLamp::FlashLamp);
            //               FlashLamp();

            //           CheckSerial->start();

            mainStatus="WARNING";
            LStatus="WARNING";
            Warning();
        }else{
            qDebug()<<"4-Check--FLASHLAMP"<<WarnText ;

            QTimer::singleShot(300, this, &WarningLamp::FlashLamp);
            mainStatus="WARNING";
            LStatus="WARNING";
            Warning();
        }
    }else if (WarnText0=="回归正常"){
        QTimer::singleShot(100, this, &WarningLamp::CleanLamp);
        ResetWarning("REGRESS",ModuleNo);
        mainStatus="ONLINE";
        LStatus="ONLINE";
        Online();
    }

    QString DispStr=QString::number(AlarmID)+" "+ EffDt.right(12)+"  " + ModuleNo +"  " + mainStatus +"  " + Info;
    addListStr(DispStr);
    LEffDt0=EffDt;
    LAlarmID=AlarmID;

}


void  WarningLamp::CheckInit()
{
   QString qCheck="select top 1 ModuleNo,EffDt,WarnText,Info ,%%PHYSLOC%% as physloc from Alarm    (ModuleNo ="+ HModuleNo+" or ModuleNo=" + LModuleNo + ")  order by  EffDt desc  ";

   QSqlQuery q(db);
   q.exec(qCheck);
   q.first();

   QString ModuleNo=q.value(0).toString();
   QString EffDt=q.value(1).toString().right(23);
   QString WarnText=q.value(2).toString().left(12);
   QString Info=q.value(3).toString().left(10);
   QString Physloc=q.value(4).toString();

   qDebug()<<"1-init"<<qCheck<<"  "<<WarnText<<q.value(1)<<"  temp="<<Info <<"  Physloc=" <<Physloc;
    EffDt0=EffDt;
   HPhysloc=Physloc;
   MPhysloc=Physloc;
   LPhysloc=Physloc;

   // ui->textEdit->append(moment.toMSecsSinceEpoch(), temp) ;
   if (ModuleNo==HModuleNo){
       ModuleNo="淬火";
   }
   if (ModuleNo==LModuleNo){
       ModuleNo="回火";
   }

   if ((ModuleNo0!=ModuleNo) or (EffDt0!=EffDt)){
       //发送BeepWarning信号

       ModuleNo0=ModuleNo;

   }
    //开机响5秒


   //记录开机时间
   QString DispStr=QString::number(AlarmID)+" "+EffDt.right(12) +"  " + ModuleNo +"  " + Info+"  "+ mainStatus;
   ui->listWidget->addItem(DispStr);


}



void WarningLamp::CheckSerial_timeout_slot(){


   //查询串口的状态
   /* 打开端口，设置速率为19200，8，0，
    * 读取Input1端口，如果端口1压着，则设至状态为ONLINE，绿灯亮着；如果不压，则状态为OFFLINE，绿灯不亮；
    * 循坏等待报警数据库，每5秒查询一次
    * 如果查到有报警，则输出WARNING状态，Output1闭合，报警器发声音；
    * 读取Input2端口，如果端口2压着，则当前状态为WARNING，则返回状态为ONLINE； ，Output1释放，报警器关闭。
    *
    *
    */
      qDebug()<<"---------";
      qDebug()<<"Checktimer_timeout_slot---";

   qmser->ReadStatus();
        QTimer::singleShot(100, this, &WarningLamp::DoStatus);

}
void WarningLamp::DoStatus(){

   QString     listStr=qmser->getStatus();
        qDebug()<<"Now mainStatus="<< mainStatus ;
   if (mainStatus=="ONLINE"){

       if (listStr=="OFFLINE"){
           mainStatus="OFFLINE";
           HStatus="OFFLINE";
           MStatus="OFFLINE";
           LStatus="OFFLINE";
           Offline();
           addListStr(listStr);
       }
   }else   if (mainStatus=="WARNING"){
       if (listStr=="CLEAN"){
           QTimer::singleShot(100, this, &WarningLamp::CleanLamp);
           if (HStatus=="WARNING"){
               ResetWarning("CLEAN",HModuleNo);
               HStatus="ONLINE";
           }
           if (LStatus=="WARNING"){
               ResetWarning("CLEAN",LModuleNo);
               MStatus="ONLINE";
           }
           if (MStatus=="WARNING"){
               ResetWarning("CLEAN",MModuleNo);
               LStatus="ONLINE";
           }
           if ((HStatus=="ONLINE") and (MStatus=="ONLINE") and (LStatus=="ONLINE" )){
               Online();
               mainStatus="ONLINE";
           }

           addListStr(listStr);
       }
       if (listStr=="OFFLINE"){
           QTimer::singleShot(100, this, &WarningLamp::CleanLamp);
           mainStatus="OFFLINE";
           Offline();
           ResetWarning("OFFLINE",HModuleNo);
            ResetWarning("OFFLINE",MModuleNo);
           ResetWarning("OFFLINE",LModuleNo);
           addListStr(listStr);
       }
   }else    if (mainStatus=="OFFLINE"){
         if (listStr=="ONLINE"){
           mainStatus="ONLINE";
             HStatus="ONLINE";
             MStatus="ONLINE";
             LStatus="ONLINE";
           CheckInit();
           FlashLamp5s();
           Online();
           addListStr(listStr);
         }
   }



}

void WarningLamp::addListStr(QString listStr){

    // ui->listWidget->addItem(QDateTime::currentDateTime().toString("hh:mm:ss")+" "+mainStatus+"   "+listStr);

    ui->listWidget->addItem(listStr+"   "+mainStatus);

    const int maxItemCount = 50;
    int itemCount = ui->listWidget->count();

    if (itemCount > maxItemCount) {
       int itemsToRemove = itemCount - maxItemCount;
       for (int i = 0; i < itemsToRemove; ++i) {
             QListWidgetItem *item = ui->listWidget->takeItem(0); // 删除第一个条目
             delete item; // 释放内存
       }
    }
    ui->listWidget->scrollToBottom();
}


void WarningLamp::setStatus(QString strStatus)
{

   mainStatus=strStatus;
}
void WarningLamp::FlashLamp()
{
     qmser->FlashLamp();
}


void WarningLamp::FlashLamp5s()
{

   qmser->FlashLamp5s();
}

void WarningLamp::CleanLamp()
{
     qmser->CleanLamp();

//   CheckSerial->start();
}



void WarningLamp::Warning(){
     QString button_style_green="QPushButton{background-color:green; \
         color: white;   border-radius: 10px;  border: 2px groove gray; border-style: outset;}";
   QString button_style_red="QPushButton{background-color:red; \
       color: white;   border-radius: 10px;  border: 2px groove gray; border-style: outset;}";
    ui->pushButton1->setStyleSheet(button_style_green);
    ui->pushButton2->setStyleSheet(button_style_red);
}

void WarningLamp::Online(){
    QString button_style_green="QPushButton{background-color:green; \
        color: white;   border-radius: 10px;  border: 2px groove gray; border-style: outset;}";
    QString button_style_gray="QPushButton{background-color:gray; \
        color: white;   border-radius: 10px;  border: 2px groove gray; border-style: outset;}";

    ui->pushButton1->setStyleSheet(button_style_green);

    ui->pushButton2->setStyleSheet(button_style_gray);

}

void WarningLamp::Offline(){
    QString button_style_gray="QPushButton{background-color:gray; \
        color: white;   border-radius: 10px;  border: 2px groove gray; border-style: outset;}";
    ui->pushButton1->setStyleSheet(button_style_gray);
        ui->pushButton2->setStyleSheet(button_style_gray);

}

void WarningLamp::ResetWarning(QString mode,QString M){
    if (M ==HModuleNo){
        AlarmID=HAlarmID;
    }else  if (M==LModuleNo){
          AlarmID=LAlarmID;
    }else{
        AlarmID=MAlarmID;
    }
    QString qCheck="update AlarmBeep set ResetTime=getdate(),ResetMode='"+mode+"' where id="+QString::number(AlarmID);
        //   QString const str=QString("select top 600 samptime,tempavg,linenum,qm from ")+tb_name+QString(" where samptime >='%1'  and samptime<'%2' ").arg(momentInTime.toString("yyyy-MM-dd hh:mm:ss" )).arg(lastTime.toString("yyyy-MM-dd hh:mm:ss" ))+QString("  and  convert(varchar,samptime,120)  like '%00:00' order by samptime asc ");
        //   qDebug()<<"mon="<<momentInTime<<"  start2="<<start<<str;
        QSqlQuery q(db);
        q.exec(qCheck);
}


void WarningLamp::on_pushButton1_clicked()
{
     qDebug()<<"CLICK00----"<<mainStatus;

    if (mainStatus=="ONLINE"){
         qDebug()<<"CLICK05----"<<mainStatus;
             mainStatus="OFFLINE";
             Offline();
             addListStr("OFFLINE");
     }else if (mainStatus=="OFFLINE"){
  qDebug()<<"CLICK08----"<<mainStatus;
             mainStatus="ONLINE";
             CheckInit();
             FlashLamp5s();
             Online();
             addListStr("ONLINE");

    }
    qDebug()<<"CLICK10----"<<mainStatus;

}


void WarningLamp::on_pushButton2_clicked()
{

             QTimer::singleShot(100, this, &WarningLamp::CleanLamp);
             ResetWarning("CLEAN",HModuleNo);
             HStatus="ONLINE";
             Online();
             addListStr("CLEAN");

}


void WarningLamp::on_btnBrowse_clicked()
{
             HistoryWin    *h1=new HistoryWin(); //创建对话框
             //   dlgTableSize->setAttribute(Qt::WA_DeleteOnClose);
             //对话框关闭时自动删除对话框对象,用于不需要读取返回值的对话框
             //如果需要获取对话框的返回值，不能设置该属性，可以在调用完对话框后删除对话框
             //    Qt::WindowFlags    flags=HisWindow->windowFlags();
             //    HisWindow->setWindowFlags(flags | Qt::MSWindowsFixedSizeDialogHint); //设置对话框固定大小

             //    HisWindow->setComQM(ui->qmEdit_1->text(),ui->comEdit_1->text().toInt()); //对话框数据初始化
             h1->setDB(db);
             qDebug()<<"Q&L="<< HModuleNo<<"   "<<LModuleNo;
             h1->toModuleNo(HModuleNo,LModuleNo);
              h1->show();// 以模态方式显示对话框，用户关闭对话框时返回 DialogCode值
}

