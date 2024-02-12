/******************************************************************************
 * Created by Alexander Herzig
 * Copyright /*$<Year>$*/ Landcare Research New Zealand Ltd
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
 * /*$<WrapperFactoryName>$*/.h
 *
 *  Created on: /*$<FileDate>$*/
 *      Author: /*$<Author>$*/
 */

#ifndef /*$<WrapperFactoryName>$*/_H_
#define /*$<WrapperFactoryName>$*/_H_

#include <QObject>
#include "NMWrapperFactory.h"

#include "/*$<WrapperClassNameLower>$*/_export.h"

class /*$<WrapperClassNameUpper>$*/_EXPORT /*$<WrapperFactoryName>$*/ : public NMWrapperFactory
{
    Q_OBJECT
public:
    /*$<WrapperFactoryName>$*/(QObject *parent = nullptr);

    NMProcess* createWrapper();
    bool isSinkProcess(void) {return /*$<ProcessIsSink>$*/;}
    QString getWrapperClassName() {return QStringLiteral("/*$<WrapperClassName>$*/");}
    QString getComponentAlias() {return QStringLiteral("/*$<ComponentAlias>$*/");}

};

#endif // /*$<WrapperFactoryName>$*/_H
