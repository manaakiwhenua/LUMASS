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
 * NMModelController.h
 *
 *  Created on: 11/06/2012
 *      Author: alex
 */

#ifndef NMMODELCONTROLLER_H_
#define NMMODELCONTROLLER_H_

#include "nmlog.h"
#include "NMModelComponent.h"
#include "NMModelSerialiser.h"
 
#include <qobject.h>
#include <QMap>
#include <QString>

#include <string>
#include <iostream>

#ifdef BUILD_RASSUPPORT
  #include "NMRasdamanConnectorWrapper.h"
#endif

class NMModelController: public QObject
{
	Q_OBJECT

public:
	NMModelController(QObject* parent=0);
	virtual ~NMModelController();

	void execute(void);
	NMModelComponent* getComponent(const QString& name);
	QString addComponent(NMModelComponent* comp,
			NMModelComponent* host=0);
	bool removeComponent(const QString& name);
	bool contains(const QString& compName);
	NMModelComponent* identifyRootComponent(void);

	const QMap<QString, NMModelComponent*>& getRepository(void)
			{return this->mComponentMap;}

protected:
	QMap<QString, NMModelComponent*> mComponentMap;
	NMModelComponent* mRootComponent;


private:
	std::string ctx;


};

#endif /* NMMODELCONTROLLER_H_ */
