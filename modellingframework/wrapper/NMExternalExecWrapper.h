/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2015 Landcare Research New Zealand Ltd
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
/*
 * NMExternalExecWrapper.h
 *
 *  Created on: 2015-09-24
 *      Author: Alexander Herzig
 */

#ifndef NMExternalExec_H_
#define NMExternalExec_H_

#include <string>
#include <iostream>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QProcess>

#include "NMMacros.h"
#include "NMProcess.h"

#include <qobject.h>

#include "NMItkDataObjectWrapper.h"
#include "nmmodframe_export.h"

class NMMODFRAME_EXPORT NMExternalExecWrapper : public NMProcess
{
    Q_OBJECT

    Q_PROPERTY(QStringList Command READ getCommand WRITE setCommand)
    Q_PROPERTY(QStringList Arguments READ getArguments WRITE setArguments)

public:

    NMPropertyGetSet( Command, QStringList )
    NMPropertyGetSet( Arguments, QStringList )

public:

    NMExternalExecWrapper(QObject* parent=0);
    virtual ~NMExternalExecWrapper();

    QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
    void instantiateObject(void);

    void setNthInput(unsigned int numInput,
              QSharedPointer<NMItkDataObjectWrapper> imgWrapper);

    void linkParameters(unsigned int step,
                        const QMap<QString, NMModelComponent*>& repo);

    void update(void);

protected:

    QString mCurCmd;
    QString mCurArgs;
    QStringList mCurArgList;

    QStringList mCommand;
    QStringList mArguments;

    QProcess mCmdProcess;

private:
    static const std::string ctx;

};


#endif /* NMExternalExec_H_ */
