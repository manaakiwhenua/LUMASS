/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2025 Landcare Research New Zealand Ltd
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
 * NMDifferenceImageFilterWrapperFactory.h
 *
 *  Created on: 5 Feb 2025
 *      Author: Alex Herzig
 */

#ifndef NMDifferenceImageFilterWrapperFactory_H_
#define NMDifferenceImageFilterWrapperFactory_H_

#include <QObject>
#include "NMWrapperFactory.h"

#include "nmdifferenceimagefilterwrapper_export.h"

class NMDIFFERENCEIMAGEFILTERWRAPPER_EXPORT NMDifferenceImageFilterWrapperFactory : public NMWrapperFactory
{
    Q_OBJECT
public:
    NMDifferenceImageFilterWrapperFactory(QObject *parent = nullptr);

    NMProcess* createWrapper();
    bool isSinkProcess(void) {return true;}
    QString getWrapperClassName() {return QStringLiteral("NMDifferenceImageFilterWrapper");}
    QString getComponentAlias() {return QStringLiteral("CompareImage");}

};

#endif // NMDifferenceImageFilterWrapperFactory_H
