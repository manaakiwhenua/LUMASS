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
 * NMCubeSliceToImage2DFilterWrapperFactory.h
 *
 *  Created on: 2020-12-07
 *      Author: Alex Herzig
 */

#ifndef NMCubeSliceToImage2DFilterWrapperFactory_H_
#define NMCubeSliceToImage2DFilterWrapperFactory_H_

#include <QObject>
#include "NMWrapperFactory.h"

#include "nmcubeslicetoimage2dfilterwrapper_export.h"

class NMCUBESLICETOIMAGE2DFILTERWRAPPER_EXPORT NMCubeSliceToImage2DFilterWrapperFactory : public NMWrapperFactory
{
    Q_OBJECT
public:
    NMCubeSliceToImage2DFilterWrapperFactory(QObject *parent = nullptr);

    NMProcess* createWrapper();
    bool isSinkProcess(void) {return false;}
    QString getWrapperClassName() {return "NMCubeSliceToImage2DFilterWrapper";}
    QString getComponentAlias() {return QStringLiteral("CubeSliceToImage2D");}
};

#endif // NMCubeSliceToImage2DFilterWrapperFactory_H
