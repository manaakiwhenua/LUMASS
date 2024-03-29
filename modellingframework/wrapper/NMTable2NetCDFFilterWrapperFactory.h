/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2023 Landcare Research New Zealand Ltd
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
 * NMTable2NetCDFFilterWrapperFactory.h
 *
 *  Created on: 2023-06-22
 *      Author: Alexander Herzig
 */

#ifndef NMTable2NetCDFFilterWrapperFactory_H_
#define NMTable2NetCDFFilterWrapperFactory_H_

#include <QObject>
#include "NMWrapperFactory.h"

#include "nmtable2netcdffilterwrapper_export.h"

class NMTABLE2NETCDFFILTERWRAPPER_EXPORT NMTable2NetCDFFilterWrapperFactory : public NMWrapperFactory
{
    Q_OBJECT
public:
    NMTable2NetCDFFilterWrapperFactory(QObject *parent = nullptr);

    NMProcess* createWrapper();
    bool isSinkProcess(void) {return false;}
    QString getWrapperClassName() {return "NMTable2NetCDFFilterWrapper";}
    QString getComponentAlias() {return QStringLiteral("Table2NetCDF");}
};

#endif // NMTable2NetCDFFilterWrapperFactory_H
