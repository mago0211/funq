#include "attacher.h"
#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <QSocketNotifier>

#ifdef Q_WS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/socket.h>
#include <unistd.h>
#endif

int Attacher::m_sigtermFd[2] = {0,0};

Attacher::Attacher(const QString & exe, const QStringList & args, int port, bool pickMode, QObject *parent) :
    QObject(parent), m_exe(exe), m_args(args), m_wellStarted(false)
{
    m_process.setProcessChannelMode(QProcess::ForwardedChannels);

    QProcessEnvironment env(QProcessEnvironment::systemEnvironment());
#ifndef Q_WS_WIN
    QString ld_preload = env.value("LD_PRELOAD");
    if (! ld_preload.isEmpty()) {
        ld_preload += " ";
    }
    QDir appPath(QCoreApplication::applicationDirPath());
    ld_preload += appPath.absoluteFilePath("libFunq.so");
    env.insert("LD_PRELOAD", ld_preload);

    socketpair(AF_UNIX, SOCK_STREAM, 0, m_sigtermFd);
    m_snTerm = new QSocketNotifier(m_sigtermFd[1], QSocketNotifier::Read, this);
    connect(m_snTerm, SIGNAL(activated(int)), this, SLOT(handleSigTerm()));
#endif
    if (port >=0) {
        env.insert("FUNQ_PORT", QString::number(port));
    }
    if (pickMode) {
        env.insert("FUNQ_MODE_PICK", "1");
    }
    m_process.setProcessEnvironment(env);
}

void Attacher::start() {
    m_process.start(m_exe, m_args);
    m_wellStarted = m_process.waitForStarted();
#ifdef Q_WS_WIN
    if (m_wellStarted) {
        QDir appPath(QCoreApplication::applicationDirPath());
        QString libraryPath = appPath.absoluteFilePath("funq.dll");
        wchar_t path[_MAX_PATH];
        const int pathLength = libraryPath.toWCharArray(path);
        path[pathLength] = 0;
        HMODULE library = ::LoadLibraryW(path);
        if (!library) {
            qDebug() << QString("Impossible de charger la librairie %1").arg(libraryPath);
            m_process.kill();
            return;
        }
        FARPROC hook = ::GetProcAddress(library, "_Z11installHooqP11HINSTANCE__m");
        if (!hook) {
            qDebug() << "Impossible de trouver la fonction installHooq";
            m_process.kill();
            return;
        }
        typedef void(*Installer)(HINSTANCE, DWORD);
        Installer installer = reinterpret_cast<Installer>(hook);
        (*installer)(library, m_process.pid()->dwThreadId);
    }
#endif
    if (!m_wellStarted) {
        qDebug() << QString("Impossible de lancer %1").arg(m_exe);
        qApp->quit();
    }
}

int Attacher::returnCode() {
    if (! m_wellStarted) {
        return 127;
    }
    return m_process.exitCode();
}

#ifndef Q_WS_WIN

void Attacher::termSignalHandler(int) {
    char a = 1;
    write(m_sigtermFd[0], &a, sizeof(a));
}

void Attacher::handleSigTerm()
 {
     m_snTerm->setEnabled(false);
     char tmp;
     read(m_sigtermFd[1], &tmp, sizeof(tmp));

     // do Qt stuff
    m_process.close();
    if (! m_process.waitForFinished(1000)) {
        m_process.terminate();
        m_process.waitForFinished(1000);
    }

     m_snTerm->setEnabled(true);
 }

#endif