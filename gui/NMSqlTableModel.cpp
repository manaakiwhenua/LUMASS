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

#include "NMSqlTableModel.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlIndex>
#include <QUuid>
#include <QIcon>


const std::string NMSqlTableModel::ctx = "NMSqlTableModel";

NMSqlTableModel::NMSqlTableModel(QObject *parent, QSqlDatabase db)
    : QSqlTableModel(parent, db), mDatabaseName("")
{
}

bool
NMSqlTableModel::select()
{
    QString selSTmt = this->selectStatement();
    return QSqlTableModel::select();
}

QVariant
NMSqlTableModel::data(const QModelIndex &idx, int role) const
{
    QVariant var = QSqlTableModel::data(idx, role);
    QVariant value = QSqlTableModel::data(idx, Qt::DisplayRole);
    QString colname = QSqlTableModel::headerData(idx.column(), Qt::Horizontal).toString();

    if (idx.isValid())
    {
        switch(role)
        {
        case Qt::TextAlignmentRole:
            {
                switch(value.type())
                {
                case QVariant::Double:
                case QVariant::Int:
                case QVariant::UInt:
                case QVariant::LongLong:
                case QVariant::ULongLong:
                    return QVariant(Qt::AlignRight | Qt::AlignVCenter);
                    break;
                default:
                        return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
                        break;
                }
            }
            break;

        case Qt::DecorationRole:
            {
                if (     value.type() == QVariant::ByteArray
                    &&   colname.compare("Geometry", Qt::CaseInsensitive) == 0
                   )
                {
                    QIcon icn = QIcon(":vector_layer.png");
                    QVariant retVar = QVariant::fromValue(icn);
                    return retVar;
                }
                else
                {
                    return QVariant();
                }

            }
            break;

        case Qt::DisplayRole:
            {
                if (     value.type() == QVariant::ByteArray
                    &&   colname.compare("Geometry", Qt::CaseInsensitive) != 0
                   )
                {
                    return QVariant("BLOB");
                }

            }
            break;

        default:
            break;
        }
    }

    return var;
}

QString
NMSqlTableModel::getNMPrimaryKey()
{
    QString primaryKey = "";

    QSqlIndex pk = this->primaryKey();
    if (!pk.isEmpty() && !pk.fieldName(0).isEmpty())
    {
        primaryKey = pk.fieldName(0);
    }
    else
    {
        primaryKey = this->headerData(0, Qt::Horizontal, Qt::DisplayRole).toString();
    }

    return primaryKey;
}
