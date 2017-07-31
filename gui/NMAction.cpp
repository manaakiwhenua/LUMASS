/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2017 Landcare Research New Zealand Ltd
 *
 * This file is part of 'LUMASS', which is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif

#include "NMAction.h"

#include <QMenu>
#include <QtTreePropertyBrowser>
#include <QtVariantEditorFactory>
#include <QtVariantPropertyManager>
#include <QtVariantProperty>
#include <QtProperty>

#include "NMGlobalHelper.h"
#include "NMModelController.h"

NMAction::NMAction(QObject* parent)
    : QAction(parent),
      mMenu(0),
      mDialog(0)
{
}

NMAction::NMAction(const QString& text, QObject* parent)
    : QAction(text, parent),
      mMenu(0),
      mDialog(0)
{
}

NMAction::NMAction(const QIcon& icon, const QString& text, QObject* parent)
    : QAction(icon, text, parent),
      mMenu(0),
      mDialog(0)
{
}

NMAction::~NMAction()
{
    if (mMenu)
    {
        delete mMenu;
    }

    if (mDialog)
    {
        delete mDialog;
    }
}

void NMAction::updateActionParameter(const QString& key, QVariant value,
                                     NMAction::NMActionInputType type)
{
    this->setProperty(key.toStdString().c_str(), value);
    mMapInputType[key] = type;
    emit updatedActionParameter(key, value);
}

NMModelController*
NMAction::getModelController(void)
{
    return mModelController;
}

void
NMAction::setModelController(NMModelController *ctrl)
{
    mModelController = ctrl;
    if (ctrl != 0)
    {
        connect(this, SIGNAL(updatedActionParameter(const QString&, QVariant)),
                ctrl, SLOT(updateSettings(const QString&, QVariant)));
    }
}

void
NMAction::requestRemoveTool()
{
    emit signalRemoveUserTool(this);
}

int
NMAction::getTriggerCount(void)
{
    return mMapTypeTrigger.count();
}

void
NMAction::setTrigger(const QString &key, NMAction::NMActionTriggerType type)
{
    if (type != NMAction::NM_ACTION_TRIGGER_NIL)
    {
        QString oldKey = mMapTypeTrigger[type];
        if (!oldKey.isEmpty())
        {
            mMapTriggerType.remove(oldKey);
        }

        mMapTypeTrigger[type] = key;
        mMapTriggerType[key] = type;
    }

    switch(type)
    {
    case NMAction::NM_ACTION_TRIGGER_ID:
    case NMAction::NM_ACTION_TRIGGER_POLY:
    case NMAction::NM_ACTION_TRIGGER_REGION:
    //case NMAction::NM_ACTION_TRIGGER_COLUMN:
        this->setCheckable(true);
        break;
    case NMAction::NM_ACTION_TRIGGER_NIL:
        this->setCheckable(false);
        break;
    default:
        break;
    }

    if (mMenu == 0)
    {
        initMenu();
    }
}

QString
NMAction::getTriggerKey(NMAction::NMActionTriggerType type)
{
    return mMapTypeTrigger[type];
}

NMAction::NMActionTriggerType
NMAction::getTriggerType(const QString &key)
{
    return mMapTriggerType[key];
}

void
NMAction::initMenu()
{
    if (mMenu != 0)
    {
        delete mMenu;
    }
    mMenu = new QMenu();

    QAction* configAct = new QAction("Configure ...", mMenu);
    mMenu->addAction(configAct);

    QAction* removeAct = new QAction(QString("Remove %1").arg(this->objectName()), mMenu);
    mMenu->addSeparator();
    mMenu->addAction(removeAct);

    this->setMenu(mMenu);

    connect(configAct, SIGNAL(triggered()), this, SLOT(callConfigDlg()));
    connect(removeAct, SIGNAL(triggered()), this, SLOT(requestRemoveTool()));
}

//bool
//NMAction::eventFilter(QObject *watched, QEvent *event)
//{
////    if (QString::fromLatin1("QtTreePropertyBrowser").compare(watched->metaObject()->className()) == 0)
////    {
////        if (event->type() == QEvent::KeyPress)
////        {
////            QKeyEvent* ke = static_cast<QKeyEvent*>(event);
////            if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter)
//            {
//                NMLogDebug(<< "sender: " << watched->metaObject()->className()
//                           << "  type: " << event->type());
////                return true;
//            }
////        }
////    }
//    return false;
//}

//bool NMAction::event(QEvent* event)
//{
//    NMLogDebug(<< event->type() << " ")
//}


void
NMAction::createConfigDialog()
{
    if (mDialog == 0)
    {
        mDialog = new QDialog();
        mDialog->setModal(false);
    }

    QVBoxLayout* vlayout = new QVBoxLayout(mDialog);
    QtTreePropertyBrowser* bro = new QtTreePropertyBrowser(mDialog);

    bro->setResizeMode(QtTreePropertyBrowser::Interactive);
    vlayout->addWidget(bro);

    QPushButton* btnClose = new QPushButton("Close", mDialog);
    mDialog->connect(btnClose, SIGNAL(pressed()), mDialog, SLOT(accept()));
    vlayout->addWidget(btnClose);

    this->installEventFilter(btnClose);

    populateSettingsBrowser();
}

void
NMAction::populateSettingsBrowser(void)
{
    if (mDialog == 0)
    {
        return;
    }

    QtTreePropertyBrowser* bro = mDialog->findChild<QtTreePropertyBrowser*>();
    if (bro == 0)
    {
        return;
    }
    bro->clear();

    //    QStringList noprops;
    //    noprops << "objectName" << "autoRepeat" << "checkable"
    //            << "checked" << "enabled" << "font" << "icon"
    //            << "iconText" << "iconVisibleInMenu" << "menuRole"
    //            << "priority" << "shortcut" << "shortcutContext"
    //            << "statusTip" << "text" << "toolTip" << "visible"
    //            << "whatsThis"
    //            << "modelName";

    QList<QByteArray> dynPropNames = this->dynamicPropertyNames();  //mobj->propertyCount();
    foreach (const QByteArray& propName, dynPropNames)
    {
        const char* name = propName.data();
        // don't show all your hand ...
        //        if (noprops.contains(name))
        //        {
        //            continue;
        //        }

        // add configurable properties
        QtVariantEditorFactory* ed = new QtVariantEditorFactory(bro);
        QtVariantPropertyManager* man = new QtVariantPropertyManager(bro);
        QtVariantProperty* vprop = man->addProperty(this->property(name).type(), name);
        if (vprop != 0)
        {
            vprop->setValue(this->property(name));
            bro->setFactoryForManager(man, ed);
            bro->addProperty(vprop);

            connect(man, SIGNAL(signalCallAuxEditor(QtProperty*,const QStringList &)),
                    this, SLOT(settingsFeeder(QtProperty*,const QStringList &)));
            connect(man, SIGNAL(valueChanged(QtProperty*,QVariant)),
                    this, SLOT(updateSettings(QtProperty*,QVariant)));
        }
        else
        {
            NMLogWarn(<< "Failed processing tool property '"
                      << name << "' (" << (this->property(name).typeName()) << ")!");
        }
    }
}

void
NMAction::settingsFeeder(QtProperty *prop, const QStringList &valList)
{
    if (!valList.isEmpty())
    {
        // watch out for Workspace & UserModels

        QString propName = prop->propertyName();
        NMActionInputType type = NMAction::NM_ACTION_INPUT_UNKNOWN;
        if (mMapInputType.contains(propName))
        {
            type = mMapInputType[propName];
        }
        else if (   prop->propertyName().compare(QString::fromLatin1("Workspace")) == 0
                 || prop->propertyName().compare(QString::fromLatin1("UserModels")) == 0)
        {
            type = NMAction::NM_ACTION_INPUT_EXISTING_DIRECTORY;
        }

        QString newValue;
        QVariant wsVar = this->getModelController()->getSetting("Workspace");
        QString ws;
        if (wsVar.isValid())
        {
            ws = wsVar.toString();
        }

        switch(type)
        {
        case NM_ACTION_INPUT_OPEN_FILENAME:
             newValue = QFileDialog::getOpenFileName(mDialog,
                                           tr("Select Input File ..."),
                                           ws);
             break;

        case NM_ACTION_INPUT_SAVE_FILENAME:
             newValue = QFileDialog::getSaveFileName(mDialog,
                                           tr("Select Output File ..."),
                                           ws);
             break;


        case NM_ACTION_INPUT_EXISTING_DIRECTORY:
             newValue = QFileDialog::getExistingDirectory(mDialog,
                                           tr("Select Directory ..."),
                                           ws);
             break;
        default:
            break;
        }

        if (!newValue.isEmpty())
        {
            updateSettings(prop, QVariant::fromValue(newValue));
        }
    }
}

void
NMAction::updateSettings(QtProperty *prop, QVariant valueVar)
{
    NMActionInputType type = NM_ACTION_INPUT_UNKNOWN;
    if (mMapInputType.contains(prop->propertyName()))
    {
        type = mMapInputType[prop->propertyName()];
    }
    updateActionParameter(prop->propertyName(), valueVar, type);
    populateSettingsBrowser();
}

void
NMAction::callConfigDlg()
{
    if (mDialog == 0)
    {
        createConfigDialog();
    }

    mDialog->show();
}
