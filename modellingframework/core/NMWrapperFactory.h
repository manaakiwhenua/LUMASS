/******************************************************************************
* Created by Alexander Herzig
* Copyright 2025-2026 New Zealand Institute for Bioeconomy Science Limited
*
* This file is part of 'LUMASS', which is free software: you can redistribute
* it and/or modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation, either version 3 of the License,
* or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
********************************************************************************/

#ifndef NMWRAPPERFACTORY_H
#define NMWRAPPERFACTORY_H

#include <QObject>
#include "NMProcess.h"

#if defined _WIN32
    #define WINCALL __stdcall
#else
    #define WINCALL
#endif

#include "nmmodframecore_export.h"

class NMMODFRAMECORE_EXPORT NMWrapperFactory : public QObject
{
    Q_OBJECT
public:
    NMWrapperFactory(QObject *parent = nullptr);
    virtual NMProcess* createWrapper(void)=0;
    virtual bool isSinkProcess(void)=0;
    virtual QString getWrapperClassName(void)=0;
    virtual QString getComponentAlias(void)=0;
    virtual QString getComponentInfo(void) {return QStringLiteral("");}

};

#endif // NMWRAPPERFACTORY_H
