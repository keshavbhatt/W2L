#include "dbmanager.h"
#include <QtSql>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QCoreApplication>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>
#include <QFile>
#include <QRegularExpression>
#include <QString>
#include <QTextStream>
#include <QRegularExpression>
#include "downloader.h"
#include <QStringList>

#include <QFileInfo>


int current= 0;
QStringList down_links;
QString imgpath;


dbmanager::dbmanager(QObject *parent) : QObject(parent)
{

}

bool add_in_db(int pageid , int revid)
{
    QDir databasePath;
    QString path = databasePath.currentPath()+"WTL.db";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");//not dbConnection
    db.setDatabaseName(path);
    if(!db.open())
    {
        qDebug() <<"error in opening DB";
    }
    else
    {
        qDebug() <<"connected to DB" ;
    }

    QSqlQuery query;

    query.prepare("INSERT INTO pages (page_ID,page_revision) "
                     "VALUES (? , ?)");
       query.bindValue(0,pageid);
       query.bindValue(1, revid);

       if(query.exec())
       {
           qDebug() << "done";
           return(true);
       }
       else
       {
           qDebug() << query.lastError();

       }
       return (false);
}

bool save_images(QString filename)
{
    QString content , newpath , style;
    qDebug() << filename +"html filename ";
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
         qDebug() <<"unable to open file";
         return false;
    }
    else{
         content = file.readAll();
         //  download images here

        QRegularExpression link_regex("src=(?<path>.*?)>");
        QRegularExpressionMatchIterator links = link_regex.globalMatch(content);

        while (links.hasNext()) {
            QRegularExpressionMatch match = links.next();
            QString down_link = match.captured(1).remove(QString("&mode=mathml\"")); ;
//            qDebug()<<down_link.remove(QString("&mode=mathml\""));
            down_links << down_link;  //prepare list of downloads
            //start downloading images
            QString d = content.replace("&mode=mathml\"",".svg"); //clean img src in local html file
            newpath = d.replace("http://en.wikitolearn.org/index.php?title=Special:MathShowImage&hash=",imgpath+"/"); // clean img src in local html file and prepare the local path those are to be saved in html file

        }



        qDebug() << down_links; //got the list of downloads
        file.close();

    }

    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() <<"unable to write to file";
        return false;
    }

    else
    {
        qDebug() <<"write to file here";
        QTextStream out(&file);
        out << newpath;
       // qDebug()<<newpath;

        file.close();

    }

    dbmanager *d = new dbmanager(0) ;
    d->doDownload(down_links);

return true ;

}


//start downloading images




void dbmanager::doDownload(const QVariant& v)
{
    if (v.type() == QVariant::StringList) {


        QNetworkAccessManager *manager= new QNetworkAccessManager(this);

             QUrl url = v.toStringList().at(current);

             filename = url.toString().remove("http://en.wikitolearn.org/index.php?title=Special:MathShowImage&hash=");
             m_network_reply = manager->get(QNetworkRequest(QUrl(url)));

             connect(m_network_reply, SIGNAL(downloadProgress (qint64, qint64)),this, SLOT(updateDownloadProgress(qint64, qint64)));
             connect(m_network_reply,SIGNAL(finished()),this,SLOT(downloadFinished()));


    }
}



void dbmanager::downloadFinished(){
    qDebug()<<filename;
        if(m_network_reply->error() == QNetworkReply::NoError){


             m_file =  new QFile(imgpath+"/"+filename+".svg");
             qDebug()<<imgpath+"/"+filename;
             if(!m_file->open(QIODevice::ReadWrite | QIODevice::Truncate)){
                 qDebug() << m_file->errorString();
              }
             m_file->write(m_network_reply->readAll());

             QByteArray bytes = m_network_reply->readAll();
             QString str = QString::fromUtf8(bytes.data(), bytes.size());
             int statusCode = m_network_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
             qDebug() << QVariant(statusCode).toString();
         }
          m_file->flush();
          m_file->close();
            int total = down_links.count();
            if(current<total-1){
             current++;
             qDebug()<<"current = "<<current<<"total = "<<total;
             doDownload(down_links);}
            else if(current==total-1)
            {
                qDebug()<<"download complete";
            }

}


void dbmanager::updateDownloadProgress(qint64 bytesRead, qint64 totalBytes)
{
//    ui->progressBar->setMaximum(totalBytes);
//    ui->progressBar->setValue(bytesRead);
    qDebug()<<bytesRead<<totalBytes;
}


void dbmanager::add()
{

    QString text ;
    int pageid , revid;


    // create custom temporary event loop on stack
       QEventLoop eventLoop;

       // "quit()" the event-loop, when the network request "finished()"
       QNetworkAccessManager mgr;
       QObject::connect(&mgr, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));

       // the HTTP request
       QNetworkRequest req( QUrl( QString("http://en.wikitolearn.org/api.php?action=parse&page=Linear%20Algebra/Sets&format=json") ) );
       QNetworkReply *reply = mgr.get(req);
       eventLoop.exec();

       if (reply->error() == QNetworkReply::NoError) {
           //success
           //qDebug() << "Success" <<reply->readAll();
          QString   html = (QString)reply->readAll();
          QJsonDocument jsonResponse = QJsonDocument::fromJson(html.toUtf8());

          QJsonObject jsonObj = jsonResponse.object();


          text = jsonObj["parse"].toObject()["text"].toObject()["*"].toString();
          pageid = jsonObj["parse"].toObject()["pageid"].toInt();
          revid = jsonObj["parse"].toObject()["revid"].toInt();
         text = text.replace("\n","");
         text = text.replace("&#39;/index.php", "http://en.wikitolearn.org/index.php");
         text = text.replace("&amp;","&");
         text = text.replace("MathShowImage&amp;", "MathShowImage&");
         text = text.replace("mode=mathml&#39;", "mode=mathml""");
         text = text.replace("<meta class=\"mwe-math-fallback-image-inline\" aria-hidden=\"true\" style=\"background-image: url(" ,"<img style=\"background-repeat: no-repeat; background-size: 100% 100%; vertical-align: -0.838ex;height: 2.843ex;\""   "src=");
         text = text.replace("<meta class=\"mwe-math-fallback-image-display\" aria-hidden=\"true\" style=\"background-image: url(" ,"<img style=\"background-repeat: no-repeat; background-size: 100% 100%; vertical-align: -0.838ex;height: 2.843ex;\""  "src=");
         text = text.replace("&mode=mathml);" , "&mode=mathml\">");
       //  qDebug() << text;
         qDebug() <<pageid;

         delete reply;
       }

       else {
           //failure
           qDebug() << "Failure" <<reply->errorString();
           delete reply;
       }
       QDir dir;
       imageDownloadPath = QString::number(pageid);
       imgpath =imageDownloadPath;

       if(QDir(imageDownloadPath).exists())
       {
        qDebug() << " already exist ";
       }
       else{
           dir.mkdir(imageDownloadPath);

           QString filename = imageDownloadPath+".html";
           QFile file(filename);
             file.open(QIODevice::WriteOnly | QIODevice::Text);
             QTextStream out(&file);
             out << text;

             // optional, as QFile destructor will already do it:
             file.close();
             bool success = add_in_db(pageid,revid);
             if(success == true)
             {
                 qDebug() <<"entry added to DB successfully ";
             }
             else
             {
                 qDebug() <<" failed to add in DB ";
             }

              success = save_images(filename);
       }


}

void dbmanager::del()
{
    qDebug() <<"DELETION CODE GOES HERE";
}
