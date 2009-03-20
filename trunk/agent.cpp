#include "agent.h"

KaixinFriend::KaixinFriend(QString id)
{
    setData("id",id);
}
void KaixinFriend::setData(QString pKey, QString pValue)
{
    data[pKey]=pValue;
}
QString KaixinFriend::getData(QString pKey)
{
    return data[pKey];
}
KaixinGardenField::KaixinGardenField(KaixinGarden* pParent)
{
    parent=pParent;
}
QString KaixinGardenField::get_verify()
{
    return parent->verify;
}
QString KaixinGardenField::get_farmnum()
{
    return data["farmnum"];
}
QString KaixinGardenField::get_fuid()
{
    return parent->owner->getData("id");
}
bool KaixinGardenField::isForeign()
{
    if(data["shared"]!="0")
        return true;
    else
        return false;
}
QString KaixinGardenField::getData(QString pKey)
{
    return data[pKey];
}
void KaixinGardenField::setData(QString pKey, QString pValue)
{
    data[pKey]=pValue;
}
QList<KaixinGardenField*> KaixinGarden::allFields()
{
    return fields.values();
}
KaixinGarden::KaixinGarden(QString gardenXML, QString verifyCode)
{
    status="Updated";
    verify=verifyCode;
    //parse the xml and assign the properties
    QDomDocument doc;
    doc.setContent(gardenXML);
    //1. parse account information
    QDomElement account=doc.elementsByTagName("account").item(0).toElement();
    for(QDomNode n = account.firstChild(); !n.isNull(); n = n.nextSibling())
    {
        QDomElement t = n.toElement();
        if (!t.isNull())
            settings[t.tagName()]=t.text();
    }
    //2. parse farm fields
    QDomNodeList items=doc.elementsByTagName("item");
    for(int i=0;i<items.count();i++)
    {
        QDomElement t = items.item(i).toElement();
        if (!t.isNull())
        {
            KaixinGardenField* field=new KaixinGardenField(this);
            for(QDomNode m = t.firstChild(); !m.isNull(); m=m.nextSibling())
            {
                QDomElement o=m.toElement();
                if(!o.isNull())
                {
                    field->setData(o.tagName(),o.text());
                }
            }
            fields[field->get_farmnum().toInt()]=field;
        }
    }
    //    qDebug()<<"Garden created with "<<settings.count()<<"account settings and "<<allFields().count()<<" farm fields";
    if(settings.count()==0)
    {
        QTextCodec *codec=QTextCodec::codecForName("UTF-8");
        QString my=codec->toUnicode(QString("没有添加").toAscii());
        if(gardenXML.indexOf(my)!=-1)
            status="Component not installed.";
        else
            status="Error:\n"+gardenXML;
    }
}
void KaixinGarden::setOwner(KaixinFriend* pOwner)
{
    owner=pOwner;
}
QString KaixinGarden::getSetting(QString pKey)
{
    return settings[pKey];
}
KaixinAgent::KaixinAgent(QString pUserName, QString pPassword)
{
    userName=pUserName;
    password=pPassword;
    connect(&http, SIGNAL(finished(QNetworkReply*)), this, SLOT(gotReply(QNetworkReply*)));
    http.setCookieJar(&cookies);
    ageCount=0;
    connect(&clock, SIGNAL(timeout()), this, SLOT(timeout()));
    loading=false;
    codec=QTextCodec::codecForName("UTF-8");
}
void KaixinAgent::startMonitor(QString pField)
{
    QString a=pField;
    return;
}
void KaixinAgent::login()
{
    QString postData="url=/home/&email=%1&password=%2&remember=1";
    postData=postData.arg(userName).arg(password);
    qDebug()<<"login reply: "<<codec->toUnicode(blockPost("http://www.kaixin001.com/login/login.php",postData).toAscii());
}
QString KaixinAgent::getGardenXML(QString uid)
{
    //get verify code
    verifyCode="";
    for(int i=0;i<3&&verifyCode=="";i++)//Kaixin server is not stable, so try three times
    {
        QString gardenHomepage=blockPost(QString("http://www.kaixin001.com/~house/garden/index.php?fuid=%1").arg(uid));
        QRegExp rxVerify("g_verify = \\\"([^\\\"]+)\\\"");
        rxVerify.indexIn(gardenHomepage, 0);
        verifyCode=rxVerify.cap(1);
        //qDebug()<<"verify code captured: "<<verifyCode;
    }
    QString url="http://www.kaixin001.com/house/garden/getconf.php?verify=%1&fuid=%2&r=0";
    return codec->toUnicode(blockPost(url.arg(verifyCode).arg(uid)).toAscii());
}
void KaixinAgent::getFriends()
{
    QString friendList=unUCode(blockPost("http://www.kaixin001.com/interface/suggestfriend.php","parse=&type=all&_="));
    QRegExp rx("\\{\"uid\":(\\d*),\"real_name\":\"(\\w+)\",\"real_name_unsafe\":\"(\\w+)\"");
    for(int pos=0;;pos++)
    {
        pos=rx.indexIn(friendList, pos);
        if(pos==-1)
            break;
        friends[rx.cap(1)]=new KaixinFriend(rx.cap(1));
        friends[rx.cap(1)]->setData("name",rx.cap(2));
        friends[rx.cap(1)]->setData("name_unsafe",rx.cap(3));
    }
    qDebug()<<friends.count()<<" friends populated";
}
void KaixinAgent::gotReply(QNetworkReply*)// reply)
{
    requestFinished=true;
}
QString KaixinAgent::blockPost(QString pUrl, QString pPostData)
{
    loading=true;
    sleep(2);
    requestFinished=false;
    QNetworkReply* reply=http.post(QNetworkRequest(QUrl(pUrl)), pPostData.toAscii().toPercentEncoding("&="));
    QString ret="";
    do
    {
        QCoreApplication::processEvents();
        reply->waitForReadyRead(1000);
        ret+=reply->readAll();
    }
    while(!requestFinished);
    //            foreach(QByteArray header, reply->rawHeaderList())
    //                qDebug()<<reply->rawHeader(header);
    loading=false;
    return ret;
}

QString KaixinAgent::unUCode(QString from)
{
    QString sub;
    int i=-1;
    while(true)
    {
        i=from.indexOf("\\u",i+1);
        if(i==-1)
            break;
        unsigned short code=from.mid(i+2,4).toUShort(0,16);
        sub.setUtf16(&code,1);
        from.replace(i,6,sub);
    }
    return from;
}

void KaixinAgent::updateGardens()
{
    foreach(KaixinFriend* f, friends)
    {
        QString id=f->getData("id");
        updateGarden(id);
        doSchedule();
    }
    clock.start(10000);//every 60s check whether need water or sth
}

void KaixinAgent::updateGarden(QString id)
{
    if(loading)
    {
        //    qDebug()<<"http busy, doing later...";
        return;
    }
    //loading=true;
    QString xml=getGardenXML(id);
    gardens.remove(id);
    gardens[id]=new KaixinGarden(xml, verifyCode);
    gardens[id]->setOwner(friends[id]);
    gardens[id]->age=++ageCount;
    qDebug()<<friends[id]->getData("name")<<gardens[id]->status;
    scheduleCare(gardens[id]->allFields());
    //loading=false;
}

bool KaixinGardenField::needWater()
{
    if(getData("status")=="0")
        return false;
    if(getData("water").toInt()<5)
        return true;
    return false;
}

bool KaixinGardenField::needDeweed()
{
    if(getData("grass").toInt()>0)
        return true;
    return false;
}

bool KaixinGardenField::needDeinsect()
{
    if(getData("vermin").toInt()>0)
        return true;
    return false;
}

bool KaixinGardenField::needHavest()
{
    QTextCodec *codec=QTextCodec::codecForName("UTF-8");
    QString sy=codec->toUnicode(QString("剩余：").toAscii());
    QString tg=codec->toUnicode(QString("已偷过").toAscii());
    QString kt=codec->toUnicode(QString("可偷").toAscii());
    QString status=getData("crops");
    //qDebug()<<status;
    if(status.indexOf(sy)!=-1)//which means there are something left, but may be 0 left
    {
        //qDebug()<<"found fruit...";
        if(status.mid(status.indexOf(sy)+3,1)=="0")//0 left
            return false;
        if(status.indexOf(tg)!=-1)//stolen already
            return false;
        if(status.indexOf(kt)!=-1)//protected
            return false;
        if(getData("shared")!="0")//shared field
        {
            //qDebug()<<"shared field, can't steal"<<status<<getData("farm");
            //TODO: here check whether it is my shared field, if so, harvest.
            return false;
        }
        //qDebug()<<status;
        return true;
    }
    return false;
}

QString KaixinGardenField::getOwnerName()
{
    return parent->owner->getData("name");
}

void KaixinAgent::scheduleCare(QList<KaixinGardenField*> fields)
{
    foreach(KaixinGardenField* f, fields)
    {
        if(f->needWater())
        {
            schedule<<new KaixinEvent(QDateTime::currentDateTime(),f->get_fuid(),f->get_farmnum(),"water");
            qDebug()<<"watering scheduled for "<<f->getOwnerName()<<" field number "<<f->get_farmnum();
        }
        if(f->needHavest())
        {
            schedule<<new KaixinEvent(QDateTime::currentDateTime(),f->get_fuid(),f->get_farmnum(),"havest");
            qDebug()<<"havest scheduled for "<<f->getOwnerName()<<" field number "<<f->get_farmnum();
        }
        if(f->needDeinsect())
        {
            schedule<<new KaixinEvent(QDateTime::currentDateTime(),f->get_fuid(),f->get_farmnum(),"antivermin");
            qDebug()<<"deinsect scheduled for "<<f->getOwnerName()<<" field number "<<f->get_farmnum();
        }
        if(f->needDeweed())
        {
            schedule<<new KaixinEvent(QDateTime::currentDateTime(),f->get_fuid(),f->get_farmnum(),"antigrass");
            qDebug()<<"deweed scheduled for "<<f->getOwnerName()<<" field number "<<f->get_farmnum();
        }
        //TODO: here analyze the time left to harvest, and schedule a time to steal the crop.
    }
}

QString KaixinGarden::toString()
{
    QString ret="\n\nGarden \n\n";
    QMapIterator<QString, QString> i(settings);
    while (i.hasNext()) {
        i.next();
        ret+="\t"+i.key()+": "+i.value()+"\n";
    }
    return ret;
}

QString KaixinGardenField::toString()
{
    QString ret="\n\nGardenField \n\n";
    QMapIterator<QString, QString> i(data);
    while (i.hasNext()) {
        i.next();
        ret+="\t"+i.key()+": "+i.value()+"\n";
    }
    return ret;
}

QList<KaixinGardenField*> KaixinAgent::allFields()
{
    QList<KaixinGardenField*> ret;
    foreach(KaixinGarden* f, gardens)
        ret<<f->allFields();
    return ret;
}

bool KaixinAgent::careField(QString uid, QString farmnum, QString operation)
{
    //available operations: water, plough, havest. pest and grass operation not found yet.
    QString verify=gardens[uid]->verify;
    QString url="http://www.kaixin001.com/house/garden/%1.php?farmnum=%2&verify=%3&seedid=%4&r=0&fuid=%5";
    url=url.arg(operation).arg(farmnum).arg(verify).arg(0).arg(uid);
    QString ret=blockPost(url);
    //TODO: here check whether it is my native field or my foreign field. if so, plough and seed again. if required, purchase seeds. I don't know whether this should be implemented at all, making this machine totally automatic will probably rob fun away from the player.
    if(ret.indexOf("succ")!=-1)
        return true;
    else
    {
        qDebug()<<codec->toUnicode(ret.toAscii());
        return false;
    }
}

bool KaixinAgent::careField(KaixinGardenField* field, QString operation)
{
    return careField(field->get_fuid(),field->get_farmnum(),operation);
}

bool KaixinAgent::inCriticalTimeZone()
{
    //Kaixin garden gets updated every 0:00, 4:00, 8:00, 12:00, 16:00, 20:00 Beijing time. The first half an hour is the most critical time to water, depest and deweed. Therefore within this timezone, the program should run six time faster to update a garden per 10 seconds. In the rest time, it just relax and update each minute.
    int tm=QDateTime::currentDateTime().toString("hhmm").toInt();
    for(int i=0;i<2400;i+=400)
    {
        int t=i+200;//time zone difference between australia and china
        int span=30;
        if(tm>t&&tm<t+span)
            return true;
    }
    return false;
}

void KaixinAgent::timeout()
{
    if(gardens.count()==0)
        return;
    //In the critical time zone, update a garden each 10s, else, update a garden per minute.
    timerCount++;
    if(!inCriticalTimeZone()&&timerCount%6!=0)
        return;
    //update the oldest garden
    uint minAge=99999999;
    KaixinGarden* oldest;
    foreach(KaixinGarden* f, gardens)
    {
        if(f->age<minAge)
        {
            minAge=f->age;
            oldest=f;
        }
    }
    QString id=oldest->owner->getData("id");
    updateGarden(id);
    QString c=QDateTime::currentDateTime().toString("hh:mm");
    qDebug()<<c<<": "<<schedule.count()<<" events pending";
    //do schedule
    doSchedule();
}

void KaixinAgent::doSchedule()
{
    if(loading)
    {
        //qDebug()<<"still loading gardens.... do later";
        return;
    }
    foreach(KaixinEvent* e, schedule)
    {
        if(e->due())
        {
            if(!careField(e->uid, e->fieldNumber, e->action))
            {
                e->reSchedule(3);
                qDebug()<<"failed, retry in 3 minutes "<<friends[e->uid]->getData("name")<<e->fieldNumber<<e->action;
            }
            else
            {
                schedule.remove(e);
                qDebug()<<"done: "<<friends[e->uid]->getData("name")<<e->fieldNumber<<e->action;
                continue;
            }
        }
        if(e->retries>3)
        {
            schedule.remove(e);
            qDebug()<<"tried 3 times without any luck, given up: "<<friends[e->uid]->getData("name")<<e->fieldNumber<<e->action;
            //delete e;//is this necessary?
        }
    }
}

KaixinEvent::KaixinEvent(QDateTime pDue, QString pUid, QString pFieldNumber, QString pAction)
{
    uid=pUid;
    fieldNumber=pFieldNumber;
    action=pAction;
    dueTime=pDue;
}

bool KaixinEvent::due()
{
    if(QDateTime::currentDateTime()>dueTime)
        return true;
    else
        return false;
}

void KaixinEvent::reSchedule(int minutes)
{
    dueTime=dueTime.addSecs(minutes*60);
    retries++;
}
