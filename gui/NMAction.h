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

#ifndef NMACTION_H
#define NMACTION_H

#include "NMMacros.h"

#include <QAction>
#include <QString>

class QMenu;
class NMLogger;
class NMModelController;
class QtProperty;

class NMAction : public QAction
{
    Q_OBJECT
    Q_PROPERTY(QMap<QString, NMActionOutputType> Outputs READ getOutputs WRITE setOutputs)
    Q_PROPERTY(QString ModelName READ getModelName WRITE setModelName)
    Q_PROPERTY(NMModelController* ModelController READ getModelController WRITE setModelController)

public:

    enum NMActionTriggerType
    {
        NM_ACTION_TRIGGER_NIL,        // QVariant::Invalid
        NM_ACTION_TRIGGER_ID,         // QVariant::LongLong
        NM_ACTION_TRIGGER_REGION,     // QVariant::RectF
        NM_ACTION_TRIGGER_POLY,       // QVariant::PolygonF
        NM_ACTION_TRIGGER_COLUMN,     // QVariant::String
        NM_ACTION_TRIGGER_FILENAME,   // QVariant::String
        NM_ACTION_TRIGGER_TABLENAME,   // QVariant::String
        NM_ACTION_TRIGGER_LAYER_BUFFER // NMItkDataObjectWrapper
    };
    Q_ENUM(NMActionTriggerType)

    enum NMActionOutputType
    {
        NM_ACTION_DISPLAY_BUFFER,
        NM_ACTION_DISPLAY_FILENAME,
        //NM_ACTION_DISPLAY_TABLENAME
    };
    Q_ENUM(NMActionOutputType)

    enum NMActionInputType
    {
        NM_ACTION_INPUT_UNKNOWN,
        NM_ACTION_INPUT_OPEN_FILENAME,
        NM_ACTION_INPUT_SAVE_FILENAME,
        NM_ACTION_INPUT_EXISTING_DIRECTORY,
        NM_ACTION_INPUT_TEXT,
        NM_ACTION_INPUT_NUMERIC
    };
    Q_ENUM(NMActionInputType)


    typedef QMap<QString, NMActionOutputType> NMOutputMap;

public slots:
    NMGUIPropertyGetSet( Outputs, NMOutputMap )
    NMGUIPropertyGetSet( Logger, NMLogger*)

    void setModelName(const QString& modelName)
        {mModelName = modelName;}
    QString getModelName(void) const
        {return mModelName;}

    void setModelPath(const QString& modelPath)
        {mModelPath = modelPath;}
    QString getModelPath()
        {return mModelPath;}

    NMModelController* getModelController(void);
    void setModelController(NMModelController* ctrl);

    QString getTriggerKey(NMAction::NMActionTriggerType type);
    NMAction::NMActionTriggerType getTriggerType(const QString& key);
    void setTrigger(const QString& key, NMAction::NMActionTriggerType type);

    int getTriggerCount(void);

    void updateActionParameter(const QString& key, QVariant value, NMAction::NMActionInputType type);
    void callConfigDlg(void);
    void settingsFeeder(QtProperty* prop, const QStringList& valList);
    void updateSettings(QtProperty* prop, QVariant valueVar);

    void reloadUserConfig(void);
    void reloadModel(void);

signals:
    void updatedActionParameter(const QString& key, QVariant value);
    void signalRemoveUserTool(NMAction* act);


public:
    NMAction(QObject* parent);
    NMAction(const QString& text, QObject* parent);
    NMAction(const QIcon& icon, const QString& text, QObject* parent);
    ~NMAction();


protected slots:
    void requestRemoveTool(void);
    void requestAbort(void);
    void resetUserModel(void);
    void populateSettingsBrowser(void);


protected:
    void initMenu();
    void createConfigDialog();
    //    bool eventFilter(QObject *watched, QEvent *event);
    //    bool event(QEvent *event);

    typedef QMap<QString, NMActionTriggerType> NMMapTriggerType;
    typedef QMap<NMActionTriggerType, QString> NMMapTypeTrigger;
    typedef QMap<QString, NMActionInputType> NMMapInputType;

    bool mHasMenu;

    NMMapTriggerType mMapTriggerType;
    NMMapTypeTrigger mMapTypeTrigger;
    NMMapInputType mMapInputType;


    NMOutputMap mOutputs;
    QString mModelName;
    QString mModelPath;
    NMModelController* mModelController;

    QMenu* mMenu;
    NMLogger* mLogger;

    QDialog* mDialog;

};

Q_DECLARE_METATYPE(NMAction::NMOutputMap)

#endif // NMACTION_H
