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
 * NMModelComponentFactory.cpp
 *
 *  Created on: 5/06/2012
 *      Author: alex
 */

#include "NMModelComponentFactory.h"

NMModelComponentFactory::NMModelComponentFactory(QObject* parent)
{
	this->setParent(parent);
}

NMModelComponentFactory::~NMModelComponentFactory()
{
}

NMModelComponentFactory& NMModelComponentFactory::instance(void)
{
	static NMModelComponentFactory fab;
	return fab;
}

NMModelComponent* NMModelComponentFactory::createModelComponent(QString compClass)
{
	if (compClass.compare("NMModelComponent") == 0)
	{
		return new NMModelComponent(this);
	}
	else
		return 0;
}
