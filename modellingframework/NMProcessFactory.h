 /****************************************************************************** 
 * Created by Alexander Herzig 
 * Copyright 2010,2011,2012 Landcare Research New Zealand Ltd 
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
 * NMProcessFactory.h
 *
 *  Created on: 5/06/2012
 *      Author: alex
 */

#ifndef NMPROCESSFACTORY_H_
#define NMPROCESSFACTORY_H_

#include <qobject.h>

#include "NMProcess.h"

#include "nmmodframe_export.h"

class NMMODFRAME_EXPORT NMProcessFactory: public QObject
{
public:
	static NMProcessFactory& instance(void);
    NMProcess* createProcess(const QString& procClass);
    NMProcess* createProcessFromAlias(const QString& alias);

    bool isSink(const QString& process);

private:
	NMProcessFactory(QObject* parent=0);
	virtual ~NMProcessFactory();
    NMProcessFactory(const NMProcessFactory& fab){}

    QString procNameFromAlias(const QString& alias);

    QStringList mSinks;
    QStringList mProcRegister;


};

#endif /* NMPROCESSFACTORY_H_ */
