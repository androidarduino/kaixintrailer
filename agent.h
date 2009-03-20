#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkCookieJar>
#include <QStringList>
#include <QDebug>
#include <QApplication>
#include <QTextCodec>
#include <QDomDocument>
#include <QDomElement>
#include <QTimer>
#include <QDateTime>

class KaixinGarden;

class KaixinFriend:public QObject
{
    Q_OBJECT
    public:
        KaixinFriend(QString id);
        void setData(QString pKey, QString pValue);
        QString getData(QString pKey);
        QMap<QString, QString> data;
};

class KaixinGardenField:public QObject
{
    Q_OBJECT
    public:
        KaixinGardenField(KaixinGarden* pParent);
        QString get_verify();
        QString get_farmnum();
        QString get_fuid();
        bool isForeign();
        QString getData(QString pKey);
        void setData(QString pKey, QString pValue);
        QString toString();
        QString getOwnerName();
    private:
        QMap<QString, QString> data;
        KaixinGarden* parent;
    public:
        bool needWater();
        bool needPlough();
        bool needDeinsect();
        bool needHavest();
        bool needDeweed();
};

class KaixinGarden:public QObject
{
    Q_OBJECT
    private:
        QMap<int, KaixinGardenField*> fields;
        QMap<QString, QString> settings;
    public:
        uint age;
        KaixinFriend* owner;
        QString verify;
        static QNetworkAccessManager* http;
        QList<KaixinGardenField*> allFields();
        KaixinGarden(QString gardenXML, QString verifyCode);
        void setOwner(KaixinFriend* pOwner);
        QString getSetting(QString pKey);
        QString toString();
        QString status;
};

class KaixinEvent
{
    public:
        KaixinEvent(QDateTime pDue, QString pUid, QString pFieldNumber, QString pAction);
        bool due();
        void reSchedule(int minutes);
        int retries;
        QString uid, fieldNumber, action;
    private:
        QDateTime dueTime;
};

class KaixinAgent:public QObject
{
    Q_OBJECT
    public:
        KaixinAgent(QString pUserName, QString pPassword);
        void startMonitor(QString pField);
        void login();
        void getFriends();
        void updateGardens();
        void updateGarden(QString id);
        bool careField(QString uid, QString farmnum, QString operation);
        bool careField(KaixinGardenField* field, QString operation);
        QList<KaixinGardenField*> allFields();
    protected:
        QString getGardenXML(QString uid="0");
        void doSchedule();
    protected slots:
        void gotReply(QNetworkReply*);
        void timeout();
    private:
        QString blockPost(QString pUrl, QString pPostData="");
        QString unUCode(QString from);
        void scheduleCare(QList<KaixinGardenField*> field);
        QString userName,password,verifyCode;
        QNetworkAccessManager http;
        QNetworkCookieJar cookies;
        QMap<QString, KaixinFriend*> friends;
        QMap<QString, KaixinGarden*> gardens;
        bool requestFinished;
        uint ageCount, timerCount;
        QSet<KaixinEvent*> schedule;
        QTimer clock;
        bool loading;
        QTextCodec * codec;
        bool inCriticalTimeZone();
};

