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
 *  Created on: 2016-06-23
 *  Author: Alexander Herzig
 */

#ifndef NMTableReader_H_
#define NMTableReader_H_

#include <QObject>

#include "NMProcess.h"
#include "otbAttributeTable.h"
#include "NMItkDataObjectWrapper.h"

#include "nmmodframe_export.h"

class NMMODFRAME_EXPORT NMTableReader : public NMProcess
{
    Q_OBJECT

    Q_PROPERTY(QStringList FileNames READ getFileNames WRITE setFileNames)
    Q_PROPERTY(QStringList TableNames READ getTableNames WRITE setTableNames)

public:

    NMPropertyGetSet( FileNames, QStringList )
    NMPropertyGetSet( TableNames, QStringList )

public:
    NMTableReader(QObject* parent=0);
    virtual ~NMTableReader();

    QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
    void instantiateObject(void);

    void setNthInput(unsigned int numInput,
              QSharedPointer<NMItkDataObjectWrapper> imgWrapper);

    void linkParameters(unsigned int step,
                        const QMap<QString, NMModelComponent*>& repo);

    void update(void);


protected:

    QStringList mFileNames;
    QStringList mTableNames;

    QString mCurFileName;
    QString mOldFileName;
    QString mCurTableName;

    otb::AttributeTable::Pointer mTable;

    bool mbUpdate;

private:

    static const std::string ctx;

};


#endif // include guard
