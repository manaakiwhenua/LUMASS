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
 * NMSQLiteProcessorWrapper.h
 *
 *  Created on: 2015-11-08
 *      Author: Alexander Herzig
 */

#ifndef NMSQLiteProcessorWrapper_H_
#define NMSQLiteProcessorWrapper_H_

#include "nmlog.h"
#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"

#include <string>
#include <iostream>
#include <QStringList>
#include <QList>
#include "nmsqliteprocessorwrapper_export.h"

template<class TInputImage, unsigned int Dimension=2>
class NMSQLiteProcessorWrapper_Internal;

class NMSQLITEPROCESSORWRAPPER_EXPORT
NMSQLiteProcessorWrapper
        : public NMProcess
{
    Q_OBJECT


    Q_PROPERTY(QStringList SQLStatement READ getSQLStatement WRITE setSQLStatement)

public:


    NMPropertyGetSet( SQLStatement, QStringList )

public:
    NMSQLiteProcessorWrapper(QObject* parent=0);
    virtual ~NMSQLiteProcessorWrapper();

    template<class TInputImage, unsigned int Dimension>
    friend class NMSQLiteProcessorWrapper_Internal;

    QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
    void instantiateObject(void);

    void setNthInput(unsigned int numInput,
              QSharedPointer<NMItkDataObjectWrapper> imgWrapper, const QString& name);

    QSharedPointer<NMItkDataObjectWrapper> getRAT(unsigned int idx);


    void setRAT(unsigned int idx,
        QSharedPointer<NMItkDataObjectWrapper> imgWrapper);


protected:
    void linkParameters(unsigned int step,
            const QMap<QString, NMModelComponent*>& repo);


    QStringList mSQLStatement;

};

#endif /* NMSQLiteProcessorWrapper_H_ */
