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
 * NMParameterTable.h
 *
 *  Created on: 04/02/2016
 *      Author: alex
 */

#ifndef NMPARAMETERTABLE_H
#define NMPARAMETERTABLE_H

#include <QObject>

#include "NMDataComponent.h"
#include "NMMacros.h"
#include "nmmodframe_export.h"

/*!
 * \brief The NMParameterTable class is a specialised data component for parameter tables.
 *        Given a file name to a xls csv or dbf file, the parameter creates SQLiteTable object
 *        and allows access to individual cells via the \ref NMModelComponent::getModelParameter
 *        interface.
 *
 */

class NMMODFRAME_EXPORT NMParameterTable : public NMDataComponent
{
    Q_OBJECT
    Q_PROPERTY(QString FileName READ getFileName WRITE setFileName)
    Q_PROPERTY(QString TableName READ getTableName WRITE setTableName)

public:
    NMPropertyGetSet( TableName, QString )
//    NMPropertyGetSet( FileName, QString )

public:
    NMParameterTable(QObject* parent=0);
    virtual ~NMParameterTable(void);

    void setFileName(QString fn);
    QString getFileName(void){return mFileName;}

    /**
     * @brief getModelParameter fetches a
     * @param paramSpec
     * @return
     */
    //virtual QVariant getModelParameter(const QString &paramSpec);

    virtual void linkComponents(unsigned int step, const QMap<QString, NMModelComponent*>& repo);
    virtual void update(const QMap<QString, NMModelComponent*>& repo);
    virtual void reset(void);

protected:

    QString mFileName;
    QString mTableName;

private:
    static const std::string ctx;
};

#endif // NMPARAMETERTABLE_H
