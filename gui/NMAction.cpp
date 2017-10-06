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
#include "NMIterableComponent.h"
#include "NMModelSerialiser.h"

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

NMModelController *NMAction::getModelController(void)
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
    if (mModelController->isModelRunning())
    {
        NMLogError(<< mModelName.toStdString() << ": The model cannot be removed "
                   << "while being executed!");
        return;
    }
    this->resetUserModel();
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
    QString oldKey = mMapTypeTrigger[type];
    if (type != NMAction::NM_ACTION_TRIGGER_NIL)
    {
        if (!oldKey.isEmpty())
        {
            mMapTriggerType.remove(oldKey);
        }

        mMapTypeTrigger[type] = key;
        mMapTriggerType[key] = type;
    }
    else
    {
        if (!oldKey.isEmpty())
        {
            disconnect(this, SIGNAL(triggered(bool)),
                       NMGlobalHelper::getMainWindow(),
                       SLOT(selectUserTool(bool)));
            if (!key.isEmpty())
            {
                connect(this, SIGNAL(triggered()),
                        NMGlobalHelper::getMainWindow(),
                        SLOT(executeUserModel()));
            }
        }
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
    mMenu->addSeparator();

    QIcon stopIcon(":model-stop-icon.png");
    QAction* stopAct = new QAction(stopIcon, "Stop!", mMenu);
    mMenu->addAction(stopAct);

    QIcon resetIcon(":model-reset-icon.png");
    QAction* resetAct = new QAction(resetIcon, QString("Reset"), mMenu);
    mMenu->addAction(resetAct);
    mMenu->addSeparator();

    QAction* reloadAct = new QAction(QString("Reload"), mMenu);
    mMenu->addAction(reloadAct);

    mMenu->addSeparator();
    QAction* removeAct = new QAction(QString("Remove"), mMenu);
    mMenu->addAction(removeAct);

    this->setMenu(mMenu);

    connect(configAct, SIGNAL(triggered()), this, SLOT(callConfigDlg()));
    connect(stopAct, SIGNAL(triggered()), this, SLOT(requestAbort()));
    connect(resetAct, SIGNAL(triggered()), this, SLOT(resetUserModel()));
    connect(reloadAct, SIGNAL(triggered()), this, SLOT(reloadModel()));
    connect(removeAct, SIGNAL(triggered()), this, SLOT(requestRemoveTool()));
}

void
NMAction::requestAbort(void)
{
    if (mModelController)
    {
        mModelController->abortModel();
    }
}

void
NMAction::resetUserModel()
{
    if (this->mModelController == 0)
    {
        return;
    }

    this->mModelController->resetComponent("root");
}

void
NMAction::reloadUserConfig(void)
{
    QString baseName = QString("%1Tool").arg(mModelName);
    QString toolTableName = QString("%1/%2.ldb")
                    .arg(mModelPath).arg(baseName);

    QFileInfo fnInfo(toolTableName);
    if (!fnInfo.exists() || !fnInfo.isReadable())
    {
        NMLogWarn(<< mModelName.toStdString() << "Reloading tool configuration failed!");
        return;
    }

    otb::SQLiteTable::Pointer toolTable = otb::SQLiteTable::New();
    toolTable->SetUseSharedCache(false);
    toolTable->SetDbFileName(toolTableName.toStdString());
    if (!toolTable->openConnection())
    {
        NMLogWarn(<< mModelName.toStdString() << "Reloading tool configuration failed!");
        return;
    }
    toolTable->SetTableName(baseName.toStdString());
    toolTable->PopulateTableAdmin();

    // ==============================
    // clear old settings
    // ===============================
    // note: the triggers are taken care of
    // by ::setTrigger(...)

    // remove old settings
    if (mModelController != 0)
    {
        mModelController->clearModelSettings();
    }
    mOutputs.clear();

    foreach(const QString& ik, mMapInputType.keys())
    {
        this->setProperty(ik.toStdString().c_str(), QVariant());
    }
    mMapInputType.clear();



    // ====================================
    // parse Tool Table
    // ====================================

    NMAction::NMOutputMap outMap;
    bool bKeyFound;
    int nrecs = toolTable->GetNumRows();
    long long minid = toolTable->GetMinPKValue();
    for (long long id=minid; id < minid+nrecs; ++id)
    {
        // ----------------------------------
        // OUTPUTS
        // ----------------------------------

        QString output = toolTable->GetStrValue("Output", id).c_str();
        QString outTypeStr = toolTable->GetStrValue("OutputType", id).c_str();
        if (    (!output.isEmpty() && !outTypeStr.isEmpty())
             && (    output.compare(QString::fromLatin1("NULL")) != 0
                  && outTypeStr.compare(QString::fromLatin1("NULL")) != 0
                )
           )
        {
            const int oatei = NMAction::staticMetaObject.indexOfEnumerator("NMActionOutputType");
            NMAction::NMActionOutputType outActType = static_cast<NMAction::NMActionOutputType>(
                    NMAction::staticMetaObject.enumerator(oatei).keyToValue(
                        outTypeStr.toStdString().c_str(), &bKeyFound));
            if (bKeyFound)
            {
                outMap.insert(output, outActType);
            }
        }

        // ----------------------------------
        // TRIGGERS
        // ----------------------------------

        QString trigger = toolTable->GetStrValue("Trigger", id).c_str();
        QString triggerTypeStr = toolTable->GetStrValue("TriggerType", id).c_str();
        if (    (!trigger.isEmpty() && !triggerTypeStr.isEmpty())
             && (    trigger.compare(QString::fromLatin1("NULL")) != 0
                  && triggerTypeStr.compare(QString::fromLatin1("NULL")) != 0
                )
           )
        {
            const int ttei = NMAction::staticMetaObject.indexOfEnumerator("NMActionTriggerType");
            NMAction::NMActionTriggerType triggerType = static_cast<NMAction::NMActionTriggerType>(
                        NMAction::staticMetaObject.enumerator(ttei).keyToValue(
                            triggerTypeStr.toStdString().c_str(), &bKeyFound));
            if (bKeyFound)
            {
                this->setTrigger(trigger, triggerType);
            }
        }


        // ----------------------------------
        // INPUT PARAMETERS
        // ----------------------------------
        QString actionParam = toolTable->GetStrValue("Input", id).c_str();
        QString actionValStr = toolTable->GetStrValue("InputValue", id).c_str();
        QString actionTypeStr = toolTable->GetStrValue("InputValueType", id).c_str();
        if (!actionParam.isEmpty())
        {
            NMAction::NMActionInputType inputType = NMAction::NM_ACTION_INPUT_UNKNOWN;
            if (!actionTypeStr.isEmpty())
            {
                const int iaei = NMAction::staticMetaObject.indexOfEnumerator("NMActionInputType");
                inputType = static_cast<NMAction::NMActionInputType>(
                            NMAction::staticMetaObject.enumerator(iaei).keyToValue(
                                actionTypeStr.toStdString().c_str(), &bKeyFound));
            }
            this->updateActionParameter(actionParam, QVariant::fromValue(actionValStr),
                                        inputType);
        }
    }
    this->setOutputs(outMap);

    this->populateSettingsBrowser();
}

void
NMAction::reloadModel(void)
{
    // first remove all components we've loaded
    NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(mModelController->getComponent("root"));
    NMModelComponentIterator cit = ic->getComponentIterator();
    QStringList delcomps;
    while (*cit != 0)
    {
        delcomps << (*cit)->objectName();
        ++cit;
    }

    foreach(const QString& mc, delcomps)
    {
        mModelController->removeComponent(mc);
    }

    // now reload the model
    QString fn = QString("%1/%2.lmx")
                    .arg(mModelPath).arg(mModelName);

    QFileInfo fifo(fn);
    if (fifo.exists() && fifo.isReadable())
    {
        NMModelSerialiser reader;
        reader.setModelController(mModelController);
        reader.setLogger(mLogger);
        reader.parseComponent(fn, 0, mModelController);
    }

    reloadUserConfig();

    NMLogInfo(<< mModelName.toStdString() << ": Model reloaded from '"
              << fn.toStdString() <<  "'");
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

    reloadUserConfig();

    mDialog->show();
}
