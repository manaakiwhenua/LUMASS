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
* NMSQLProcessor.h
*
*  Created on: 22/10/2015
*  Author: Alexander Herzig
*
*  Source of Inspiration:
*/

#ifndef NMSQLPROCESSOR_H
#define NMSQLPROCESSOR_H

#include <QObject>
#include "NMProcess.h"
#include "nmmodframe_export.h"


class NMMODFRAME_EXPORT NMSQLProcessor : public NMProcess
{
    Q_OBJECT
    Q_PROPERTY(QList<QStringList> SQLStmt READ getSQLStmt WRITE setSQLStmt)

public:

    NMPropertyGetSet(SQLStmt, QList<QStringList>)

    NMSQLProcessor(QObject* parent=0);
    virtual ~NMSQLProcessor();

    virtual void setNthInput(unsigned int numInput,
            QSharedPointer<NMItkDataObjectWrapper> img);
    virtual QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);

    virtual void instantiateObject(void){}
    virtual void update(void);

protected:

    virtual void linkParameters(unsigned int step, const QMap<QString, NMModelComponent*>& repo);

    QList<QStringList> mSQLStmt;
    QList<QSharedPointer<NMItkDataObjectWrapper> > mInputDataWrapper;
    QList<QSharedPointer<NMItkDataObjectWrapper> > mOutputDataWrapper;

private:
    static const std::string ctx;
};

#endif // NMSQLPROCESSOR_H
