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

#ifndef NMMODELACTION_H
#define NMMODELACTION_H

#include "NMAbstractAction.h"

class NMModelController;

class NMModelAction : public NMAbstractAction
{
    Q_OBJECT
    Q_PROPERTY(NMModelController* ModelController READ getModelController WRITE setModelController)

public:
    NMModelAction(QObject* parent);
    NMModelAction(const QString& text, QObject* parent);
    NMModelAction(const QIcon& icon, const QString& text, QObject* parent);


public slots:
    NMModelController* getModelController(void);
    void setModelController(NMModelController* ctrl);

    void updateSettings(const QString& key, QVariant value);

    void settingsFeeder(QtProperty* prop, const QStringList& valList);
    void reloadUserConfig(void);

    void reloadModel(void);
    void requestAbort(void);


protected slots:
    void requestRemoveTool(void);
    void resetUserModel(void);

protected:
    NMModelController* mModelController;

};

#endif // NMMODELACTION_H
