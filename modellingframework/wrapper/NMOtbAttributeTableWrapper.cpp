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
 * NMOtbAttributeTableWrapper.cpp
 *
 *  Created on: 22/04/2012
 *      Author: alex
 */

#include "NMOtbAttributeTableWrapper.h"
#include "otbAttributeTable.h"

NMOtbAttributeTableWrapper::NMOtbAttributeTableWrapper(QObject* parent,
		otb::AttributeTable* tab)
{
	this->mAttributeTable = tab;
	this->setParent(parent);
}

NMOtbAttributeTableWrapper::NMOtbAttributeTableWrapper(
		const NMOtbAttributeTableWrapper& tableWrapper)
{
	this->mAttributeTable =
			const_cast<NMOtbAttributeTableWrapper*>(&tableWrapper)->getAttributeTable();
	this->setParent(tableWrapper.parent());
}

NMOtbAttributeTableWrapper::~NMOtbAttributeTableWrapper()
{
}

