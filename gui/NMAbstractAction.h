/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2018 Landcare Research New Zealand Ltd
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

#ifndef NMABSTRACTACTION_H
#define NMABSTRACTACTION_H

#include "NMMacros.h"
#include "otbSQLiteTable.h"

#include <QObject>
#include <QAction>
#include <QString>

class QMenu;
class QIcon;
class QtProperty;
class NMLogger;


class NMAbstractAction : public QAction
{
    Q_OBJECT
    Q_PROPERTY(QMap<QString, NMActionOutputType> Outputs READ getOutputs WRITE setOutputs)
    Q_PROPERTY(QString ModelName READ getModelName WRITE setModelName)


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
        NM_ACTION_DISPLAY_FILENAME
        //NM_ACTION_DISPLAY_TABLENAME
    };
    Q_ENUM(NMActionOutputType)

    enum NMActionInputType
    {
        NM_ACTION_INPUT_UNKNOWN,
        NM_ACTION_INPUT_OPEN_FILENAME,      // file FileName
        NM_ACTION_INPUT_SAVE_FILENAME,      // file FileName
        NM_ACTION_INPUT_EXISTING_DIRECTORY, // directory FileName
        NM_ACTION_INPUT_TEXT,               // ´some text´
        NM_ACTION_INPUT_NUMERIC,            // integer or real number
        NM_ACTION_INPUT_ENUM,               // {´name1´, ´name2´, ..., ´nameN´}
        NM_ACTION_INPUT_BOOLEAN             // {´yes´, ´no´}
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


    QString getTriggerKey(NMAbstractAction::NMActionTriggerType type);
    NMAbstractAction::NMActionTriggerType getTriggerType(const QString& key);
    void setTrigger(const QString& key, NMAbstractAction::NMActionTriggerType type);
    int getTriggerCount(void);

    void setModelPath(const QString& modelPath)
        {mModelPath = modelPath;}
    QString getModelPath()
        {return mModelPath;}

    void updateActionParameter(const QString& key, QVariant value, NMAbstractAction::NMActionInputType type);
    void callConfigDlg(void);
    void updateSettings(QtProperty* prop, QVariant valueVar);

    virtual void settingsFeeder(QtProperty* prop, const QStringList& valList);
    virtual void reloadUserConfig(void);

signals:
    void updatedActionParameter(const QString& key, QVariant value);
    void signalRemoveUserTool(NMAbstractAction* act);


protected slots:

    virtual void requestRemoveTool(void);
    void populateSettingsBrowser(void);

protected:
    NMAbstractAction(QObject* parent);
    NMAbstractAction(const QString& text, QObject* parent);
    NMAbstractAction(const QIcon &icon, const QString &text, QObject* parent);
    ~NMAbstractAction();

    void initMenu();
    void createConfigDialog();
    otb::SQLiteTable::Pointer getToolTable(void);

    /*! action: populates emptyList for '<param>Enum' column in tool table;
     *  return value: index of current enum value, -1 upon failure;
     */
    int fetchEnumValues(const QString& param, QStringList& emptyList);

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

    QMenu* mMenu;
    NMLogger* mLogger;

    QDialog* mDialog;


};

Q_DECLARE_METATYPE(NMAbstractAction::NMOutputMap)

#endif // NMABSTRACTACTION_H
