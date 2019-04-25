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
 * NMOtbAttributeTableWrapper.h
 *
 *  Created on: 22/04/2012
 *      Author: alex
 */

#ifndef NMOTBATTRIBUTETABLEWRAPPER_H_
#define NMOTBATTRIBUTETABLEWRAPPER_H_

#include <qobject.h>
#include <QMetaType>
#include "otbAttributeTable.h"
//#include "nmmodframewrapper_export.h"

class NMOtbAttributeTableWrapper: public QObject
{
	Q_OBJECT
public:
	NMOtbAttributeTableWrapper(QObject* parent=0,
			otb::AttributeTable* tab=0);
	NMOtbAttributeTableWrapper(const NMOtbAttributeTableWrapper& tableWrapper);
	virtual ~NMOtbAttributeTableWrapper();

	void setAttributeTable(otb::AttributeTable* tab)
		{this->mAttributeTable = tab;};

	otb::AttributeTable* getAttributeTable(void)
		{return this->mAttributeTable;};

private:
	otb::AttributeTable::Pointer mAttributeTable;

};

Q_DECLARE_METATYPE(NMOtbAttributeTableWrapper)

#endif /* NMOTBATTRIBUTETABLEWRAPPER_H_ */
