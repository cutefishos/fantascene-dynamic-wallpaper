#include "wallpaper.h"

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <malloc.h>

#include <QFileDialog>
#include <QStandardPaths>
#include <QApplication>
#include <QtX11Extras/QX11Info>
#include <QScreen>
#include <QApplication>
#include <QPainter>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QDesktopWidget>
#include <QDebug>
#include <QLabel>
#include <QDBusConnection>
#include <QWebEngineView>
#include <QDesktopWidget>
#include <QGuiApplication>

#include "application.h"

Wallpaper::Wallpaper(QString path, int currentScreen, QWidget *parent)
    : QWidget(parent)
    , m_currentPath(path)
    , m_currentScreen(currentScreen)
{

    setAttribute(Qt::WA_TranslucentBackground);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setSpacing(0);
    layout->setMargin(0);
    setLayout(layout);

    registerDesktop();

    connect(dApp, &Application::refreshPix, this, &Wallpaper::slotrefreshPix);
    connect(dApp, &Application::setScreenMode, this, &Wallpaper::slotsetScreenMode);
    connect(qApp->desktop(), &QDesktopWidget::resized, this, &Wallpaper::updateGeometry);
    connect(dApp, &Application::setPlayPath, this, &Wallpaper::setFile);
    connect(dApp, &Application::setMpvPlay, this, &Wallpaper::play);
    connect(dApp, &Application::setMpvpause, this, &Wallpaper::pause);
    connect(dApp, &Application::setMpvstop, this, &Wallpaper::stop);
    connect(dApp, &Application::setMpvVolume, this, &Wallpaper::setVolume);
    connect(dApp, &Application::setScreen, this, &Wallpaper::setScreen);
    connect(dApp, &Application::sigupdateGeometry, this, &Wallpaper::updateGeometry);
    connect(dApp, &Application::setMpvValue, this, &Wallpaper::slotSetMpvValue);

    QDesktopWidget *desktopwidget = QApplication::desktop();
    connect(desktopwidget, &QDesktopWidget::resized, this, [ = ] {
        QTimer::singleShot(1000, [ = ]{
            updateGeometry();
        });
        updateGeometry();
    });
//    connect(desktopwidget, &QDesktopWidget::screenCountChanged, this, [ = ] {
//        if (qApp->desktop()->screenCount() > 1 && IdCopyScreen == dApp->m_cuurentMode && !m_mpv2 && m_mpv)
//        {
//            if (!m_mpv2) {
//                m_mpv2 = new MpvWidget();
//            }
//            layout->addWidget(m_label2);
//        } else
//        {
//            if (m_mpv2) {
//                layout->removeWidget(m_mpv2);
//                m_mpv2->deleteLater();
//                m_mpv2 = nullptr;
//            }
//        }
//        QTimer::singleShot(1000, [ = ] {
//            updateGeometry();
//        });
//        updateGeometry();
//    });

    QDBusConnection::sessionBus().connect("com.deepin.SessionManager", "/com/deepin/SessionManager",
                                          "org.freedesktop.DBus.Properties", "PropertiesChanged", this,
                                          SLOT(onSysLockState(QString, QVariantMap, QStringList)));

    QTimer::singleShot(1, this, &Wallpaper::updateGeometry);
    QTimer::singleShot(1000, this, [ = ] {
        int index = 0;
        int index1 = 0;
        qDebug() << "ssss" << index1;
        for (const QString &arg : qApp->arguments())
        {
            if (index != 0) {
                if (QFileInfo(arg).isFile()) {
                    setFile(arg);
                    play();
                    index1++;
                    qDebug() << "ssss1" << index1;
                }
            }
            index++;
        }
        qDebug() << "ssss2" << index1;
        if (index1 == 0)
        {
            QString playPath = "/opt/durapps/yoyo-fantacy/09.mp4";

            m_currentPath = m_currentPath.replace("file://", "");
            if (!m_currentPath.isEmpty()) {
                if (QFileInfo(m_currentPath).isFile()) {
                    playPath = m_currentPath;
                } else if (path.contains("www") || path.contains("http//") || path.contains("https//")) {
                    playPath = m_currentPath;
                }
            }
            setFile(playPath);
            play();
            m_currentPath = QFileInfo(m_currentPath).filePath();
            emit dApp->pathChanged(m_currentPath);
        }

    });

    setVolume(0);

    m_mouseWebEventTimer = new QTimer(this);
    connect(m_mouseWebEventTimer, SIGNAL(timeout()), this, SLOT(slotMouseEvent()));
    m_mouseWebEventTimer->start(30);

}

void Wallpaper::changeScreenMode(ScreenMode mode)
{
    switch (mode) {
    case IdCopyScreen: {
        if (qApp->desktop()->screenCount() > 1) {

            QString path = dApp->m_currentPath;
            if (nullptr == m_mpv2) {
                m_mpv2 = new MpvWidget(this);
                m_mpv2->setVisible(true);
                m_mpv2->setProperty("loop", true);
                m_mpv2->setProperty("panscan", 1);
                m_mpv2->command(QStringList() << "loadfile" << path);
                m_mpv2->setProperty("pause", true);
                m_mpv2->setProperty("volume", 0);
                layout()->addWidget(m_mpv2);

            }

        }
        break;
    }
    case IdlayoutScreen: {
        if (nullptr != m_mpv2) {
            layout()->removeWidget(m_mpv2);
            delete m_mpv2 ;
            m_mpv2 = nullptr;
        }
        break;
    }
    case IdManualSet: {
        if (nullptr != m_mpv2) {
            layout()->removeWidget(m_mpv2);
            delete m_mpv2 ;
            m_mpv2 = nullptr;
        }
        break;
    }
    default:
        break;
    }
    //暂时调用两次,为保证切换顺利
    QTimer::singleShot(10, [ = ] {
        updateGeometry();
    });
    updateGeometry();
}

void Wallpaper::setScreen(const int &index)
{
    m_currentScreen = index;
}

void Wallpaper::setFile(const QString &path)
{
    malloc_trim(0);
    if (path.contains("html") || path.contains("www") || path.contains("http//") || path.contains("https//")) {
        if (m_mpv2) {
            layout()->removeWidget(m_mpv2);
            delete m_mpv2;
            m_mpv2 = nullptr;
        }
        if (m_mpv) {
            layout()->removeWidget(m_mpv);
            m_mpv->deleteLater();
            m_mpv = nullptr;
        }

        pause();

    }  else {

        if (!m_mpv) {
            m_mpv = new MpvWidget(this);
            m_mpv->setGeometry(geometry());

            layout()->addWidget(m_mpv);
            m_mpv->setProperty("loop", true);
            m_mpv->setProperty("panscan", 1);
            m_mpv->setGeometry(geometry());
            m_mpv->show();


            if (qApp->screens().count() > 1 && IdCopyScreen == dApp->m_cuurentMode) {
                if (!m_mpv2) {
                    m_mpv2 = new MpvWidget(this);
                }
                m_mpv2->setProperty("loop", true);
                m_mpv2->setProperty("panscan", 1);
                m_mpv2->setGeometry(geometry());
                m_mpv2->setProperty("volume", 0);
                m_mpv2->show();
                layout()->addWidget(m_mpv2);
            }

        }

        m_mpv->command(QStringList() << "loadfile" << path);
        m_mpv->setProperty("pause", true);
        if (m_mpv2) {
            m_mpv2->command(QStringList() << "loadfile" << path);
            m_mpv2->setProperty("pause", true);
        }
    }
    //发送读取配置文件
    emit dApp->sigReadPlayerConfig();
    //暂时调用两次,为保证切换顺利
    QTimer::singleShot(10, [ = ] {
        updateGeometry();
    });
    updateGeometry();
}

void Wallpaper::setVolume(const qint32 volume)
{
    if (m_mpv) {
        m_mpv->setProperty("volume", volume);
    }
    if (m_mpv2) {
        m_mpv2->setProperty("volume", 0);
    }

}

void Wallpaper::clear()
{
    stop();
    hide();
}

void Wallpaper::play()
{
    if (m_mpv) {
        m_mpv->show();
        m_mpv->setProperty("pause", false);
        dApp->m_currentIsPlay = true;
    }
    if (m_mpv2) {
        m_mpv2->show();
        m_mpv2->setProperty("pause", false);
    }
}

void Wallpaper::pause()
{
    dApp->m_currentIsPlay = false;
    if (m_mpv) {
        m_mpv->setProperty("pause", true);
    }
    if (m_mpv2) {
        m_mpv2->setProperty("pause", true);
    }
}

void Wallpaper::stop()
{
    if (m_mpv) {
        m_mpv->setProperty("stop", true);
    }
    if (m_mpv2) {
        m_mpv2->setProperty("stop", true);
    }
}

void Wallpaper::slotrefreshPix(const QPixmap &pix)
{
    if (m_label2) {
        m_label2->setPixmap(pix);
    }
}

void Wallpaper::slotsetScreenMode(const QString &mode)
{
    if (mode == tr("Copy")) {
        if (dApp->m_cuurentMode != IdCopyScreen) {
            dApp->m_cuurentMode = IdCopyScreen;
            changeScreenMode(dApp->m_cuurentMode);
        }
    } else if (mode == tr("Extend")) {
        if (dApp->m_cuurentMode != IdlayoutScreen) {
            dApp->m_cuurentMode = IdlayoutScreen;
            changeScreenMode(dApp->m_cuurentMode);
        }
    } else if (mode == tr("Manual")) {
        if (dApp->m_cuurentMode != IdManualSet) {
            dApp->m_cuurentMode = IdManualSet;
            changeScreenMode(dApp->m_cuurentMode);
        }
    }


}

void Wallpaper::registerDesktop()
{
    xcb_ewmh_connection_t m_ewmh_connection;
    xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(QX11Info::connection(), &m_ewmh_connection);
    xcb_ewmh_init_atoms_replies(&m_ewmh_connection, cookie, NULL);

    xcb_atom_t atoms[1];
    atoms[0] = m_ewmh_connection._NET_WM_WINDOW_TYPE_DESKTOP;
    xcb_ewmh_set_wm_window_type(&m_ewmh_connection, winId(), 1, atoms);

    QTimer::singleShot(1, this, [ = ] {
        show();
        lower();
    });
    if (!dApp->m_screenWid.contains(winId())) {
        dApp->m_screenWid.push_back(winId());
    }
}
#include <QWindow>
bool Wallpaper::event(QEvent *event)
{
    //https://github.com/dependon/fantascene-dynamic-wallpaper/issues/4，临时解决WIN+D的问题
    if (event->type() == QEvent::WindowActivate) {
        qDebug() << "Video WindowActivate";
        for (auto wid : dApp->m_screenWid) {
            QWindow *window = QWindow::fromWinId(wid);
            if (window) {
                window->raise();
            }
        }
    }
    return  QWidget::event(event);
}

void Wallpaper::onSysLockState(QString, QVariantMap key2value, QStringList)
{
    if (key2value.value("Locked").value<bool>()) {
        dApp->m_isNoMpvPause = false;
        pause();
    } else {
        play();
        dApp->m_isNoMpvPause = true;
    }
}

void Wallpaper::slotSetMpvValue(const QString &key, const QString &value)
{
    if (m_mpv) {
        m_mpv->setProperty(key, value);
    }
    if (m_mpv2) {
        m_mpv2->setProperty(key, value);
    }
}

void Wallpaper::updateGeometry()
{
    QTimer::singleShot(100, [ = ] {
        dApp->m_currentScreenNum = dApp->desktop()->screenCount();
        QRect rec;
        QSize size1(0, 0);
        rec = qApp->desktop()->screenGeometry(qApp->desktop()->primaryScreen());
        QRect rec2 = qApp->desktop()->screenGeometry();
        QRect deskRect = qApp->desktop()->availableGeometry();
        rec = rec2;
        if (dApp->m_cuurentMode == IdCopyScreen)
        {
            rec = QRect(0, 0, rec.width(), rec.height());
            size1.setWidth(rec.width());
            size1.setHeight(rec.height());
            int twidth = 0;
            int theight = 0;
            for (auto screen : qApp->screens()) {
                twidth += screen->geometry().width();
                if (screen->geometry().height() > theight) {
                    theight = screen->geometry().height();
                }
            }

            this->setGeometry(QRect(0, 0, twidth, theight));

            int i = 1;
            for (auto screen : qApp->screens()) {
                dApp->m_currentScreenNum = dApp->desktop()->screenCount();
                if (i == 1 && m_mpv) {
                    qDebug() << screen->geometry();
                    m_mpv->setGeometry(screen->geometry());
                    i++;
                    continue;
                }
                if (i == 2 && m_mpv2) {
                    qDebug() << screen->geometry();
                    m_mpv2->setGeometry(screen->geometry());
                    i++;
                    continue;
                }

            }

        } else if (dApp->m_cuurentMode == IdlayoutScreen)
        {
            rec = QRect(0, 0, rec.width() * dApp->desktop()->screenCount(), rec.height());
            size1.setWidth(rec.width());
            size1.setHeight(rec.height());
            this->setGeometry(rec);

            if (m_mpv) {
                m_mpv->setGeometry(rec);
            }
            if (m_mpv2) {
                this->layout()->removeWidget(m_mpv2);
                m_mpv2->deleteLater();
                m_mpv2 = nullptr;
            }
        } else  if (dApp->m_cuurentMode == IdManualSet)
        {
            rec = dApp->m_manual;
            size1.setWidth(dApp->m_manual.width());
            size1.setHeight(dApp->m_manual.height());
            this->setGeometry(rec);
            if (m_mpv) {
                m_mpv->setGeometry(rec);
            }
            if (m_mpv2) {
                this->layout()->removeWidget(m_mpv2);
                m_mpv2->deleteLater();
                m_mpv2 = nullptr;
            }
        }
        lower();
        for (auto wid : dApp->m_screenWid)
        {
            QWindow *window = QWindow::fromWinId(wid);
            if (window) {
                window->raise();
            }
        }
    });

}
#include <QEvent>
#include <QMouseEvent>
void Wallpaper::slotMouseEvent()
{

}
void Wallpaper::LeftMouseClick(QWidget *eventsReciverWidget, QPoint clickPos)
{
    QMouseEvent *press = new QMouseEvent(QEvent::MouseMove,
                                         clickPos,
                                         Qt::LeftButton,
                                         Qt::MouseButton::NoButton,
                                         Qt::NoModifier);
    QCoreApplication::postEvent(eventsReciverWidget, press);
    // Some delay
//    QTimer::singleShot(300, [clickPos, eventsReciverWidget]() {
//        QMouseEvent *release = new QMouseEvent(QEvent::MouseMove,
//                                               clickPos,
//                                               Qt::LeftButton,
//                                               Qt::MouseButton::NoButton,
//                                               Qt::NoModifier);
//        QCoreApplication::postEvent(eventsReciverWidget, release);
//    });
}
