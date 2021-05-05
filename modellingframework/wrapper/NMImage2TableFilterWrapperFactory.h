/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2021 Landcare Research New Zealand Ltd
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
 * NMImage2TableFilterWrapperFactory.h
 *
 *  Created on: 24/03
 *      Author: Alex Herzig
 */

#ifndef NMImage2TableFilterWrapperFactory_H_
#define NMImage2TableFilterWrapperFactory_H_

#include <QObject>
#include "NMWrapperFactory.h"

#include "nmimage2tablefilterwrapper_export.h"

class NMIMAGE2TABLEFILTERWRAPPER_EXPORT NMImage2TableFilterWrapperFactory : public NMWrapperFactory
{
    Q_OBJECT
public:
    NMImage2TableFilterWrapperFactory(QObject *parent = nullptr);

    NMProcess* createWrapper();
    bool isSinkProcess(void) {return false;}
    QString getWrapperClassName() {return "NMImage2TableFilterWrapper";}
};

#endif // NMImage2TableFilterWrapperFactory_H
