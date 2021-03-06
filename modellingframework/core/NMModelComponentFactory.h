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
 * NMModelComponentFactory.h
 *
 *  Created on: 5/06/2012
 *      Author: alex
 */

#ifndef NMMODELCOMPONENTFACTORY_H_
#define NMMODELCOMPONENTFACTORY_H_

#include <qobject.h>
#include "NMModelComponent.h"
#include "nmmodframecore_export.h"

class NMMODFRAMECORE_EXPORT NMModelComponentFactory: public QObject
{
public:
	static NMModelComponentFactory& instance(void);
	NMModelComponent* createModelComponent(const QString& compClass);

private:
	NMModelComponentFactory(QObject* parent=0);
    NMModelComponentFactory(const NMModelComponentFactory& fab){}
	virtual ~NMModelComponentFactory();

    QString compNameFromAlias(const QString& alias);

    QStringList mCompRegister;
};

#endif /* NMMODELCOMPONENTFACTORY_H_ */
