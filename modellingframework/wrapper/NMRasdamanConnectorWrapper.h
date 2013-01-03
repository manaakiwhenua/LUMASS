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
 * NMRasdamanConnectorWrapper.h
 *
 *  Created on: 18/05/2012
 *      Author: alex
 */

#ifdef BUILD_RASSUPPORT

#ifndef NMRASDAMANCONNECTORWRAPPER_H_
#define NMRASDAMANCONNECTORWRAPPER_H_

#include <qobject.h>
#include <QMetaType>

#include "RasdamanConnector.h"

class NMRasdamanConnectorWrapper: public QObject
{
	Q_OBJECT

public:
	NMRasdamanConnectorWrapper(QObject* parent=0)
		{
			this->setParent(parent);
			this->mpRasconn = 0;
		}
	NMRasdamanConnectorWrapper(const NMRasdamanConnectorWrapper& rasWrapper)
	{
		this->setParent(rasWrapper.parent());
		NMRasdamanConnectorWrapper* w = const_cast<NMRasdamanConnectorWrapper*>(&rasWrapper);
		this->mpRasconn = const_cast<RasdamanConnector*>(w->getConnector());
	}
	virtual ~NMRasdamanConnectorWrapper()
	{
	}

	void setConnector(RasdamanConnector* pConn)
		{this->mpRasconn = pConn;}
	const RasdamanConnector* getConnector(void)
		{return this->mpRasconn;}

protected:
	RasdamanConnector* mpRasconn;

};

Q_DECLARE_METATYPE(NMRasdamanConnectorWrapper*)
Q_DECLARE_METATYPE(NMRasdamanConnectorWrapper)

#endif /* NMRASDAMANCONNECTORWRAPPER_H_ */

#endif /* BUILD_RASSUPPORT */
