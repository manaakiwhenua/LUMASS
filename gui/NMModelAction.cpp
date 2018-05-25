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

#include "NMModelAction.h"

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

NMModelAction::NMModelAction(QObject* parent)
    : NMAbstractAction(parent)
{
}

NMModelAction::NMModelAction(const QString& text, QObject* parent)
    : NMAbstractAction(text, parent)
{
}

NMModelAction::NMModelAction(const QIcon& icon, const QString& text, QObject* parent)
    : NMAbstractAction(icon, text, parent)
{
}


NMModelController *NMModelAction::getModelController(void)
{
    return mModelController;
}

void
NMModelAction::setModelController(NMModelController *ctrl)
{
    mModelController = ctrl;
    if (ctrl != 0)
    {
        connect(this, SIGNAL(updatedActionParameter(const QString&, QVariant)),
                ctrl, SLOT(updateSettings(const QString&, QVariant)));
    }
}

void
NMModelAction::requestAbort(void)
{
    if (    mModelController
        &&  mModelController->isModelRunning()
       )
    {
        mModelController->abortModel();
        NMLogInfo(<< mModelName.toStdString() << ": Model abortion requested by user!");
    }
}

void
NMModelAction::resetUserModel()
{
    if (this->mModelController == 0)
    {
        return;
    }

    this->mModelController->resetComponent("root");
}



void
NMModelAction::reloadModel(void)
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


void
NMModelAction::settingsFeeder(QtProperty *prop, const QStringList &valList)
{
    if (!valList.isEmpty())
    {
        // watch out for Workspace & UserModels

        QString propName = prop->propertyName();
        NMActionInputType type = NMAbstractAction::NM_ACTION_INPUT_UNKNOWN;
        if (mMapInputType.contains(propName))
        {
            type = mMapInputType[propName];
        }
        else if (   prop->propertyName().compare(QString::fromLatin1("Workspace")) == 0
                 || prop->propertyName().compare(QString::fromLatin1("UserModels")) == 0)
        {
            type = NMAbstractAction::NM_ACTION_INPUT_EXISTING_DIRECTORY;
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
NMModelAction::reloadUserConfig(void)
{
    otb::SQLiteTable::Pointer toolTable = getToolTable();
    if (toolTable.IsNull())
    {
        NMLogWarn(<< mModelName.toStdString() << ": Reloading tool configuration failed!");
        return;
    }

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

    NMAbstractAction::NMOutputMap outMap;
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
            const int oatei = NMAbstractAction::staticMetaObject.indexOfEnumerator("NMActionOutputType");
            NMAbstractAction::NMActionOutputType outActType = static_cast<NMAbstractAction::NMActionOutputType>(
                    NMAbstractAction::staticMetaObject.enumerator(oatei).keyToValue(
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
            const int ttei = NMAbstractAction::staticMetaObject.indexOfEnumerator("NMActionTriggerType");
            NMAbstractAction::NMActionTriggerType triggerType = static_cast<NMAbstractAction::NMActionTriggerType>(
                        NMAbstractAction::staticMetaObject.enumerator(ttei).keyToValue(
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
            NMAbstractAction::NMActionInputType inputType = NMAbstractAction::NM_ACTION_INPUT_UNKNOWN;
            if (!actionTypeStr.isEmpty())
            {
                const int iaei = NMAbstractAction::staticMetaObject.indexOfEnumerator("NMActionInputType");
                inputType = static_cast<NMAbstractAction::NMActionInputType>(
                            NMAbstractAction::staticMetaObject.enumerator(iaei).keyToValue(
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
NMModelAction::requestRemoveTool()
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


