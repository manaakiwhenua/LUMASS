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
 * NMSqlTableModel.h
 *
 *  Created on: 20/08/2015
 *      Author: Alexander Herzig
 */

#ifndef NMSQLTABLEMODEL_H
#define NMSQLTABLEMODEL_H

#define ctxNMSqlTableModel "NMSqlTableModel"
#include "nmlog.h"
#include <QSqlTableModel>

class NMSqlTableModel : public QSqlTableModel
{
    Q_OBJECT

public:
    NMSqlTableModel(QObject* parent=0, QSqlDatabase db=QSqlDatabase());

    QVariant data(const QModelIndex &idx, int role) const;

    /*!
     * \brief setDatabaseName - handy to save the DB name somewhere
     *                          accessible and where it is not
     *                          accidentally used by another Qt object
     * \param dbName
     */
    void setDatabaseName(const QString& dbName){mDatabaseName = dbName;}
    QString getDatabaseName(){return mDatabaseName;}

protected:
    QString mDatabaseName;

private:
    static const std::string ctx;
};

#endif // NMSQLTABLEMODEL_H
