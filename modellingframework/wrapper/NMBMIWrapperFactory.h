/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2020 Landcare Research New Zealand Ltd
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
 * NMBMIWrapperFactory.h
 *
 *  Created on: 30/04
 *      Author: Alexander
 */

#ifndef NMBMIWrapperFactory_H_
#define NMBMIWrapperFactory_H_

#include <QObject>
#include "NMWrapperFactory.h"

#include "nmbmiwrapper_export.h"

class NMBMIWRAPPER_EXPORT NMBMIWrapperFactory : public NMWrapperFactory
{
    Q_OBJECT
public:
    NMBMIWrapperFactory(QObject *parent = nullptr);

    NMProcess* createWrapper();
    bool isSinkProcess(void) {return false;}
    QString getWrapperClassName() {return QStringLiteral("NMBMIWrapper");}
    QString getComponentAlias() {return QStringLiteral("BMIModel");}
};

#endif // NMBMIWrapperFactory_H
