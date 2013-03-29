/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * Razor - a lightweight, Qt based, desktop toolset
 * http://razor-qt.org
 *
 * Copyright: 2013 Razor team
 * Authors:
 *   Kuzma Shapran <kuzma.shapran@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "actions.hpp"

#include "org.razorqt.global_action.daemon.h"

Actions::Actions(QObject *parent)
    : QObject(parent)
    , mServiceWatcher(new QDBusServiceWatcher("org.razorqt.global_action", QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, this))
    , mMultipleActionsBehaviour(MULTIPLE_ACTIONS_BEHAVIOUR_FIRST)
{
    connect(mServiceWatcher, SIGNAL(serviceUnregistered(QString)), this, SLOT(on_daemonDisappeared(QString)));
    connect(mServiceWatcher, SIGNAL(serviceRegistered(QString)), this, SLOT(on_daemonAppeared(QString)));
    mDaemonProxy = new org::razorqt::global_action::daemon("org.razorqt.global_action", "/daemon", QDBusConnection::sessionBus(), this);

    connect(mDaemonProxy, SIGNAL(actionAdded(qulonglong)), this, SLOT(on_actionAdded(qulonglong)));
    connect(mDaemonProxy, SIGNAL(actionEnabled(qulonglong,bool)), this, SLOT(on_actionEnabled(qulonglong,bool)));
    connect(mDaemonProxy, SIGNAL(actionModified(qulonglong)), this, SLOT(on_actionModified(qulonglong)));
    connect(mDaemonProxy, SIGNAL(actionRemoved(qulonglong)), this, SLOT(on_actionRemoved(qulonglong)));
    connect(mDaemonProxy, SIGNAL(actionShortcutChanged(qulonglong)), this, SLOT(on_actionShortcutChanged(qulonglong)));
    connect(mDaemonProxy, SIGNAL(actionsSwapped(qulonglong,qulonglong)), this, SLOT(on_actionsSwapped(qulonglong,qulonglong)));
    connect(mDaemonProxy, SIGNAL(multipleActionsBehaviourChanged(uint)), this, SLOT(on_multipleActionsBehaviourChanged(uint)));

    QTimer::singleShot(0, this, SLOT(delayedInit()));
}

Actions::~Actions()
{
}

void Actions::delayedInit()
{
    if (mDaemonProxy->isValid())
        on_daemonAppeared(QString());
}

void Actions::on_daemonDisappeared(const QString &)
{
    clear();
    emit daemonDisappeared();
}

void Actions::on_daemonAppeared(const QString &)
{
    init();
    emit daemonAppeared();
}

void Actions::init()
{
    clear();

    mGeneralActionInfo = getAllActions();
    GeneralActionInfos::const_iterator M = mGeneralActionInfo.constEnd();
    for (GeneralActionInfos::const_iterator I = mGeneralActionInfo.constBegin(); I != M; ++I)
    {
        if (I.value().type == "dbus")
        {
            QString shortcut;
            QString description;
            bool enabled;
            QString service;
            QDBusObjectPath path;
            if (getDBusActionInfoById(I.key(), shortcut, description, enabled, service, path))
            {
                DBusActionInfo info;
                info.shortcut = shortcut;
                info.description = description;
                info.enabled = enabled;
                info.service = service;
                info.path = path;
                mDBusActionInfo[I.key()] = info;
            }
        }
        else if (I.value().type == "method")
        {
            QString shortcut;
            QString description;
            bool enabled;
            QString service;
            QDBusObjectPath path;
            QString interface;
            QString method;
            if (getMethodActionInfoById(I.key(), shortcut, description, enabled, service, path, interface, method))
            {
                MethodActionInfo info;
                info.shortcut = shortcut;
                info.description = description;
                info.enabled = enabled;
                info.service = service;
                info.path = path;
                info.interface = interface;
                info.method = method;
                mMethodActionInfo[I.key()] = info;
            }
        }
        else if (I.value().type == "command")
        {
            QString shortcut;
            QString description;
            bool enabled;
            QString command;
            QStringList arguments;
            if (getCommandActionInfoById(I.key(), shortcut, description, enabled, command, arguments))
            {
                CommandActionInfo info;
                info.shortcut = shortcut;
                info.description = description;
                info.enabled = enabled;
                info.command = command;
                info.arguments = arguments;
                mCommandActionInfo[I.key()] = info;
            }
        }
    }

    mMultipleActionsBehaviour = static_cast<MultipleActionsBehaviour>(getMultipleActionsBehaviour());
}

void Actions::clear()
{
    mGeneralActionInfo.clear();
    mDBusActionInfo.clear();
    mMethodActionInfo.clear();
    mCommandActionInfo.clear();
    mMultipleActionsBehaviour = MULTIPLE_ACTIONS_BEHAVIOUR_FIRST;
}

QList<qulonglong> Actions::allActionIds() const
{
    return mGeneralActionInfo.keys();
}

QPair<bool, GeneralActionInfo> Actions::actionById(qulonglong id) const
{
    GeneralActionInfos::const_iterator I = mGeneralActionInfo.constFind(id);
    if (I == mGeneralActionInfo.constEnd())
        return qMakePair(false, GeneralActionInfo());
    return qMakePair(true, I.value());
}

QList<qulonglong> Actions::allDBusActionIds() const
{
    return mDBusActionInfo.keys();
}

QPair<bool, DBusActionInfo> Actions::dBusActionInfoById(qulonglong id) const
{
    DBusActionInfos::const_iterator I = mDBusActionInfo.constFind(id);
    if (I == mDBusActionInfo.constEnd())
        return qMakePair(false, DBusActionInfo());
    return qMakePair(true, I.value());
}

QList<qulonglong> Actions::allMethodActionIds() const
{
    return mMethodActionInfo.keys();
}

QPair<bool, MethodActionInfo> Actions::methodActionInfoById(qulonglong id) const
{
    MethodActionInfos::const_iterator I = mMethodActionInfo.constFind(id);
    if (I == mMethodActionInfo.constEnd())
        return qMakePair(false, MethodActionInfo());
    return qMakePair(true, I.value());
}

QList<qulonglong> Actions::allCommandActionIds() const
{
    return mCommandActionInfo.keys();
}

QPair<bool, CommandActionInfo> Actions::commandActionInfoById(qulonglong id) const
{
    CommandActionInfos::const_iterator I = mCommandActionInfo.constFind(id);
    if (I == mCommandActionInfo.constEnd())
        return qMakePair(false, CommandActionInfo());
    return qMakePair(true, I.value());
}

MultipleActionsBehaviour Actions::multipleActionsBehaviour() const
{
    return mMultipleActionsBehaviour;
}

bool Actions::getDBusActionInfoById(qulonglong id, QString &shortcut, QString &description, bool &enabled, QString &service, QDBusObjectPath &path)
{
    return mDaemonProxy->getDBusActionInfoById(id, shortcut, description, enabled, service, path);
}

bool Actions::getMethodActionInfoById(qulonglong id, QString &shortcut, QString &description, bool &enabled, QString &service, QDBusObjectPath &path, QString &interface, QString &method)
{
    return mDaemonProxy->getMethodActionInfoById(id, shortcut, description, enabled, service, path, interface, method);
}

bool Actions::getCommandActionInfoById(qulonglong id, QString &shortcut, QString &description, bool &enabled, QString &command, QStringList &arguments)
{
    return mDaemonProxy->getCommandActionInfoById(id, shortcut, description, enabled, command, arguments);
}

QList<qulonglong> Actions::getAllActionIds()
{
    return mDaemonProxy->getAllActionIds();
}

bool Actions::getActionById(qulonglong id, QString &shortcut, QString &description, bool &enabled, QString &type, QString &info)
{
    return mDaemonProxy->getActionById(id, shortcut, description, enabled, type, info);
}

QMap<qulonglong,GeneralActionInfo> Actions::getAllActions()
{
    return mDaemonProxy->getAllActions();
}

bool Actions::isActionEnabled(qulonglong id)
{
    return mDaemonProxy->isActionEnabled(id);
}

uint Actions::getMultipleActionsBehaviour()
{
    return mDaemonProxy->getMultipleActionsBehaviour();
}

void Actions::do_actionAdded(qulonglong id)
{
    QString shortcut;
    QString description;
    bool enabled;
    QString type;
    QString info;
    if (getActionById(id, shortcut, description, enabled, type, info))
    {
        GeneralActionInfo generalActionInfo;
        generalActionInfo.shortcut = shortcut;
        generalActionInfo.description = description;
        generalActionInfo.enabled = enabled;
        generalActionInfo.type = type;
        generalActionInfo.info = info;
        mGeneralActionInfo[id] = generalActionInfo;
    }

    if (type == "dbus")
    {
        QString service;
        QDBusObjectPath path;
        if (getDBusActionInfoById(id, shortcut, description, enabled, service, path))
        {
            DBusActionInfo dBusActionInfo;
            dBusActionInfo.shortcut = shortcut;
            dBusActionInfo.description = description;
            dBusActionInfo.enabled = enabled;
            dBusActionInfo.service = service;
            dBusActionInfo.path = path;
            mDBusActionInfo[id] = dBusActionInfo;
        }
    }
    else if (type == "method")
    {
        QString service;
        QDBusObjectPath path;
        QString interface;
        QString method;
        if (getMethodActionInfoById(id, shortcut, description, enabled, service, path, interface, method))
        {
            MethodActionInfo methodActionInfo;
            methodActionInfo.shortcut = shortcut;
            methodActionInfo.description = description;
            methodActionInfo.enabled = enabled;
            methodActionInfo.service = service;
            methodActionInfo.path = path;
            methodActionInfo.interface = interface;
            methodActionInfo.method = method;
            mMethodActionInfo[id] = methodActionInfo;
        }
    }
    else if (type == "command")
    {
        QString command;
        QStringList arguments;
        if (getCommandActionInfoById(id, shortcut, description, enabled, command, arguments))
        {
            CommandActionInfo commandActionInfo;
            commandActionInfo.shortcut = shortcut;
            commandActionInfo.description = description;
            commandActionInfo.enabled = enabled;
            commandActionInfo.command = command;
            commandActionInfo.arguments = arguments;
            mCommandActionInfo[id] = commandActionInfo;
        }
    }
}

void Actions::on_actionAdded(qulonglong id)
{
    do_actionAdded(id);
    emit actionAdded(id);
}

void Actions::on_actionEnabled(qulonglong id, bool enabled)
{
    GeneralActionInfos::iterator GI = mGeneralActionInfo.find(id);
    if (GI != mGeneralActionInfo.end())
    {
        GI.value().enabled = enabled;

        if (GI.value().type == "dbus")
        {
            DBusActionInfos::iterator DI = mDBusActionInfo.find(id);
            if (DI != mDBusActionInfo.end())
                DI.value().enabled = enabled;
        }
        else if (GI.value().type == "method")
        {
            MethodActionInfos::iterator MI = mMethodActionInfo.find(id);
            if (MI != mMethodActionInfo.end())
                MI.value().enabled = enabled;
        }
        else if (GI.value().type == "command")
        {
            CommandActionInfos::iterator CI = mCommandActionInfo.find(id);
            if (CI != mCommandActionInfo.end())
                CI.value().enabled = enabled;
        }
    }
    emit actionEnabled(id, enabled);
}

void Actions::on_actionModified(qulonglong id)
{
    do_actionAdded(id);
    emit actionModified(id);
}

void Actions::on_actionShortcutChanged(qulonglong id)
{
    do_actionAdded(id);
    emit actionModified(id);
}

void Actions::on_actionsSwapped(qulonglong id1, qulonglong id2)
{
    GeneralActionInfos::iterator GI1 = mGeneralActionInfo.find(id1);
    GeneralActionInfos::iterator GI2 = mGeneralActionInfo.find(id2);
    if ((GI1 != mGeneralActionInfo.end()) && (GI2 != mGeneralActionInfo.end()))
    {
        bool swapped = false;

        if (GI1.value().type == GI2.value().type)
        {
            if (GI1.value().type == "dbus")
            {
                DBusActionInfos::iterator DI1 = mDBusActionInfo.find(id1);
                DBusActionInfos::iterator DI2 = mDBusActionInfo.find(id2);
                if ((DI1 != mDBusActionInfo.end()) && (DI2 != mDBusActionInfo.end()))
                {
                    DBusActionInfo dBusActionInfo = DI1.value();
                    DI1.value() = DI2.value();
                    DI2.value() = dBusActionInfo;
                    swapped = true;
                }
            }
            else if (GI1.value().type == "method")
            {
                MethodActionInfos::iterator MI1 = mMethodActionInfo.find(id1);
                MethodActionInfos::iterator MI2 = mMethodActionInfo.find(id2);
                if ((MI1 != mMethodActionInfo.end()) && (MI2 != mMethodActionInfo.end()))
                {
                    MethodActionInfo methodActionInfo = MI1.value();
                    MI1.value() = MI2.value();
                    MI2.value() = methodActionInfo;
                    swapped = true;
                }
            }
            else if (GI1.value().type == "command")
            {
                CommandActionInfos::iterator CI1 = mCommandActionInfo.find(id1);
                CommandActionInfos::iterator CI2 = mCommandActionInfo.find(id2);
                if ((CI1 != mCommandActionInfo.end()) && (CI2 != mCommandActionInfo.end()))
                {
                    CommandActionInfo commandActionInfo = CI1.value();
                    CI1.value() = CI2.value();
                    CI2.value() = commandActionInfo;
                    swapped = true;
                }
            }
        }

        if (swapped)
        {
            GeneralActionInfo generalActionInfo = GI1.value();
            GI1.value() = GI2.value();
            GI2.value() = generalActionInfo;
        }
        else
        {
            do_actionRemoved(id1);
            do_actionRemoved(id2);
            do_actionAdded(id1);
            do_actionAdded(id2);
        }
    }
    emit actionsSwapped(id1, id2);
}

void Actions::do_actionRemoved(qulonglong id)
{
    mGeneralActionInfo.remove(id);
    mDBusActionInfo.remove(id);
    mMethodActionInfo.remove(id);
    mCommandActionInfo.remove(id);
}

void Actions::on_actionRemoved(qulonglong id)
{
    do_actionRemoved(id);
    emit actionRemoved(id);
}

void Actions::on_multipleActionsBehaviourChanged(uint behaviour)
{
    mMultipleActionsBehaviour = static_cast<MultipleActionsBehaviour>(behaviour);
}
