#include "NMAbstractAction.h"

#include <QMenu>

#include <QtTreePropertyBrowser>
#include <QtVariantEditorFactory>
#include <QtVariantPropertyManager>
#include <QtVariantProperty>
#include <QtProperty>

#include "NMGlobalHelper.h"
#include "NMLogger.h"

NMAbstractAction::NMAbstractAction(QObject* parent)
    : QAction(parent),
      mMenu(0),
      mDialog(0)
{
}

NMAbstractAction::NMAbstractAction(const QString& text, QObject* parent)
    : QAction(text, parent),
      mMenu(0),
      mDialog(0)
{
}

NMAbstractAction::NMAbstractAction(const QIcon& icon, const QString& text, QObject* parent)
    : QAction(icon, text, parent),
      mMenu(0),
      mDialog(0)
{
}

NMAbstractAction::~NMAbstractAction()
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

QString
NMAbstractAction::getTriggerKey(NMAbstractAction::NMActionTriggerType type)
{
    return mMapTypeTrigger[type];
}

//QString
//NMAbstractAction::getInputKey(NMAbstractAction::NMActionInputType type)
//{

//}

NMAbstractAction::NMActionTriggerType
NMAbstractAction::getTriggerType(const QString &key)
{
    return mMapTriggerType[key];
}

NMAbstractAction::NMActionInputType
NMAbstractAction::getInputType(const QString& key)
{
    return mMapInputType[key];
}


int
NMAbstractAction::getTriggerCount(void)
{
    return mMapTypeTrigger.count();
}

void
NMAbstractAction::setTrigger(const QString &key, NMAbstractAction::NMActionTriggerType type)
{
    QString oldKey = mMapTypeTrigger[type];
    if (type != NMAbstractAction::NM_ACTION_TRIGGER_NIL)
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
    case NMAbstractAction::NM_ACTION_TRIGGER_ID:
    case NMAbstractAction::NM_ACTION_TRIGGER_POLY:
    case NMAbstractAction::NM_ACTION_TRIGGER_REGION:
    //case NMAbstractAction::NM_ACTION_TRIGGER_COLUMN:
        this->setCheckable(true);
        break;
    case NMAbstractAction::NM_ACTION_TRIGGER_NIL:
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

void NMAbstractAction::updateActionParameter(const QString& key, QVariant value,
                                     NMAbstractAction::NMActionInputType type)
{
    this->setProperty(key.toStdString().c_str(), value);
    mMapInputType[key] = type;
    emit updatedActionParameter(key, value);
}

void
NMAbstractAction::callConfigDlg()
{
    if (mDialog == 0)
    {
        createConfigDialog();
    }

    //reloadUserConfig();

    mDialog->show();
}

void
NMAbstractAction::settingsFeeder(QtProperty *prop, const QStringList &valList)
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
        QVariant wsVar = NMGlobalHelper::getUserSetting("Workspace");
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
            updatePropSettings(prop, QVariant::fromValue(newValue));
        }
    }
}

void
NMAbstractAction::updatePropSettings(QtProperty *prop, QVariant valueVar)
{
    NMActionInputType type = NM_ACTION_INPUT_UNKNOWN;
    if (mMapInputType.contains(prop->propertyName()))
    {
        type = mMapInputType[prop->propertyName()];
    }

    // need to convert the int ENUM type into a string
    // if we've got an ENUM-configured property!
    if (    type == NMAbstractAction::NM_ACTION_INPUT_ENUM
        ||  type == NMAbstractAction::NM_ACTION_INPUT_BOOLEAN
       )
    {
        QStringList evals;
        int curEnumVal = fetchEnumValues(prop->propertyName(), evals);
        bool bOK;
        int newEnumVal = valueVar.toInt(&bOK);

        if (bOK && newEnumVal >=0 && newEnumVal < evals.size())
        {
            valueVar = QVariant::fromValue(evals.at(newEnumVal));
        }
        else
        {
            QString defaultValue = evals.size() ? evals.at(0) : "null";
            NMLogWarn(<< mModelName.toStdString() << ": Update action parameter: "
                      << "No valid value set for '" << prop->propertyName().toStdString() << "'! "
                      << "Forced assignment: " << prop->propertyName().toStdString()
                      << " = " << defaultValue.toStdString());
            valueVar = QVariant::fromValue(defaultValue);
        }
    }

    updateActionParameter(prop->propertyName(), valueVar, type);
    populateSettingsBrowser();
}

void
NMAbstractAction::reloadUserConfig(void)
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

                if (inputType == NMAbstractAction::NM_ACTION_INPUT_LAYER_SELECTION)
                {
                    actionValStr = QString("%1").arg(-1);
                }
            }
            this->updateActionParameter(actionParam, QVariant::fromValue(actionValStr),
                                        inputType);
        }
    }
    this->setOutputs(outMap);

    this->populateSettingsBrowser();
}

void
NMAbstractAction::requestRemoveTool()
{
     emit signalRemoveUserTool(this);
}

void
NMAbstractAction::populateSettingsBrowser(void)
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


    QList<QByteArray> dynPropNames = this->dynamicPropertyNames();  //mobj->propertyCount();
    foreach (const QByteArray& propName, dynPropNames)
    {
        const char* name = propName.data();
        int propType = this->property(name).type();

        QMap<QString, NMActionInputType>::const_iterator tit = mMapInputType.find(name);
        if (tit != mMapInputType.cend())
        {
            if (    tit.value() == NMAbstractAction::NM_ACTION_INPUT_LAYER_SELECTION
                 || tit.value() == NMAbstractAction::NM_ACTION_INPUT_LAYER_SELBBOX
               )
            {
                // no config dialog input!
                continue;
            }
        }

        QStringList enumVals;
        int curEnumVal = fetchEnumValues(name, enumVals);

        // add configurable properties
        QtVariantEditorFactory* ed = new QtVariantEditorFactory(bro);
        QtVariantPropertyManager* man = new QtVariantPropertyManager(bro);
        QtVariantProperty* vprop = 0;

        if (enumVals.empty())
        {
            vprop = man->addProperty(this->property(name).type(), name);
            if (vprop)
            {
                vprop->setValue(this->property(name));
            }
        }
        else
        {
            propType = QtVariantPropertyManager::enumTypeId();
            vprop = man->addProperty(propType, name);
            if (vprop)
            {
                vprop->setAttribute("enumNames", enumVals);
                vprop->setValue(QVariant::fromValue(curEnumVal));
            }
        }

        if (vprop != 0)
        {
            bro->setFactoryForManager(man, ed);
            bro->addProperty(vprop);

            connect(man, SIGNAL(signalCallAuxEditor(QtProperty*,const QStringList &)),
                    this, SLOT(settingsFeeder(QtProperty*,const QStringList &)));
            connect(man, SIGNAL(valueChanged(QtProperty*,QVariant)),
                    this, SLOT(updatePropSettings(QtProperty*,QVariant)));
        }
        else
        {
            NMLogWarn(<< "Failed processing tool property '"
                      << name << "' (" << (this->property(name).typeName()) << ")!");
        }
    }
}

void
NMAbstractAction::initMenu()
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
NMAbstractAction::createConfigDialog()
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

otb::SQLiteTable::Pointer
NMAbstractAction::getToolTable(void)
{
    otb::SQLiteTable::Pointer tab;

    QString baseName = QString("%1Tool").arg(mModelName);
    QString toolTableName = QString("%1/%2.ldb")
                    .arg(mModelPath).arg(baseName);

    QFileInfo fnInfo(toolTableName);
    if (!fnInfo.exists() || !fnInfo.isReadable())
    {
        return tab;
    }

    otb::SQLiteTable::Pointer toolTable = otb::SQLiteTable::New();
    toolTable->SetUseSharedCache(false);
    toolTable->SetDbFileName(toolTableName.toStdString());
    toolTable->SetOpenReadOnly(true);

    if (!toolTable->openConnection())
    {
        return tab;
    }

    toolTable->SetTableName(baseName.toStdString());
    if (toolTable->PopulateTableAdmin())
    {
        return toolTable;
    }
    return tab;
}

int
NMAbstractAction::fetchEnumValues(const QString &param, QStringList &emptyList)
{
    int curEnumVal = -1;

    // just in case ...
    emptyList.clear();

    otb::SQLiteTable::Pointer tab = getToolTable();
    if (tab.IsNull())
    {
        return curEnumVal;
    }

    // gather enum values, if applicable
    QString enumCol = QString("%1Enum").arg(param);
    QString curEnumStr = this->property(param.toStdString().c_str()).toString();
    if (tab->ColumnExists(enumCol.toStdString()) >= 0)
    {
        if (!tab->PrepareColumnIterator(enumCol.toStdString()))
        {
            return curEnumVal;
        }

        bool bOK = false;
        int counter=0;

        QString ev = tab->NextTextValue(bOK);
        while (bOK)
        {
            if (!ev.isEmpty() && ev.compare("NULL", Qt::CaseInsensitive) != 0)
            {
                if (ev.compare(curEnumStr, Qt::CaseSensitive) == 0)
                {
                    curEnumVal = counter;
                }
                emptyList << ev;
                ++counter;
            }

            ev = tab->NextTextValue(bOK);
        }
        emptyList.removeDuplicates();
    }
    else if (mMapInputType[param] == NMAbstractAction::NM_ACTION_INPUT_BOOLEAN)
    {
        emptyList << "yes" << "no";
        if (curEnumStr.compare("yes", Qt::CaseInsensitive) == 0)
        {
            curEnumVal = 0;
        }
        else
        {
            curEnumVal = 1;
        }
    }

    return curEnumVal;
}




