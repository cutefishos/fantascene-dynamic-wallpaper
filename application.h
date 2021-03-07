#ifndef APPLICATION_H
#define APPLICATION_H
#include <DApplication>
#include <QCoreApplication>
#include <QString>
#include <QObject>
#include <QThread>
#include <QProcess>
#include <QRect>
DWIDGET_USE_NAMESPACE

#define dApp (static_cast<Application*>(QCoreApplication::instance()))
class Application : public DApplication
{
    Q_OBJECT
public:
    Application(int &argc, char **argv);
    ~Application();

Q_SIGNALS:
    void setPlayPath(const QString &PATH);
    void setMpvPlay();
    void setMpvpause();
    void setMpvstop();
    void setMpvVolume(const qint32 volume);
    void setScreen(const int &index);
    void pathChanged(const QString &);
    void refreshPix(const QPixmap &);
    void setScreenMode(const QString &);
    void sigupdateGeometry();

public:
    QThread *m_startDesktop{nullptr};
    QProcess *m_startDesktopProcess{nullptr};
    qint64 m_processId{0};
    QRect m_manual{0,0,0,0};

};

#endif // APPLICATION_H
