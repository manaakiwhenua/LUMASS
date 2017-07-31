/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2013 Landcare Research New Zealand Ltd
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
 * NMConditionalIterComponent.cpp
 *
 *  Created on: 12/02/2013
 *      Author: alex
 */

#include "NMConditionalIterComponent.h"
#include "NMModelController.h"

NMConditionalIterComponent::NMConditionalIterComponent(QObject* parent)
{
//	this->setParent(parent);
	this->initAttributes();
}

NMConditionalIterComponent::~NMConditionalIterComponent(void)
{
}

void
NMConditionalIterComponent::iterativeComponentUpdate(const QMap<QString, NMModelComponent*>& repo,
    		unsigned int minLevel, unsigned int maxLevel)
{

}

