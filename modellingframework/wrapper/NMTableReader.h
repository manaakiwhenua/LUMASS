/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2016 Landcare Research New Zealand Ltd
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
 * NMTableReader.h
 *
 *  Created on: 2016-06-28
 *      Author: Alexander Herzig
 */

#ifndef NMTableReader_H_
#define NMTableReader_H_

#include <string>
#include <iostream>
#include <QStringList>
#include <QList>
#include <QSharedPointer>

#include "nmlog.h"
#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"

#include "nmmodframecore_export.h"

class NMMODFRAMECORE_EXPORT
NMTableReader
        : public NMProcess
{
    Q_OBJECT


    Q_PROPERTY(bool CreateTable READ getCreateTable WRITE setCreateTable)
    Q_PROPERTY(QStringList FileName READ getFileName WRITE setFileName)
    Q_PROPERTY(QStringList TableName READ getTableName WRITE setTableName)
    //Q_PROPERTY(QStringList RowIdColname READ getRowIdColname WRITE setRowIdColname)

public:

    NMPropertyGetSet( CreateTable, bool)
    NMPropertyGetSet( FileName, QStringList )
    NMPropertyGetSet( TableName, QStringList )
    NMPropertyGetSet( RowIdColname, QStringList )

public:
    NMTableReader(QObject* parent=0);
    virtual ~NMTableReader();

    QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
    void instantiateObject(void);

    void setNthInput(unsigned int numInput,
              QSharedPointer<NMItkDataObjectWrapper> imgWrapper, const QString& name){}

protected:
    void linkParameters(unsigned int step,
            const QMap<QString, NMModelComponent*>& repo);

    bool mCreateTable;
    QStringList mFileName;
    QStringList mTableName;
    QStringList mRowIdColname;

};

#endif /* NMTableReader_H_ */
