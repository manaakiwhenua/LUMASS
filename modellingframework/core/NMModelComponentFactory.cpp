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
#include "NMSequentialIterComponent.h"
#include "NMConditionalIterComponent.h"
#include "NMDataComponent.h"
#include "NMDataRefComponent.h"
#include "NMParameterTable.h"

NMModelComponentFactory::NMModelComponentFactory(QObject* parent)
{
	this->setParent(parent);

    mCompRegister << "NMSequentialIterComponent"
                  << "NMDataComponent"
                  << "NMDataRefComponent"
                  << "NMParameterTable";
}

NMModelComponentFactory::~NMModelComponentFactory()
{
}

NMModelComponentFactory& NMModelComponentFactory::instance(void)
{
	static NMModelComponentFactory fab;
	return fab;
}

QString NMModelComponentFactory::compNameFromAlias(const QString &alias)
{
    QString compName;

    if (alias.compare(QString::fromLatin1("DataBuffer")) == 0)
    {
        compName = QString::fromLatin1("NMDataComponent");
    }
    else if (alias.compare(QString::fromLatin1("DataBufferReference")) == 0)
    {
        compName = QString::fromLatin1("NMDataRefComponent");
    }
    else if (alias.compare(QString::fromLatin1("ParameterTable")) == 0)
    {
        compName = QString::fromLatin1("NMParameterTable");
    }

    return compName;
}

NMModelComponent* NMModelComponentFactory::createModelComponent(const QString& compClass)
{
    QString cn = compClass;
    if (!mCompRegister.contains(compClass))
    {
        cn = this->compNameFromAlias(compClass);
    }

    if (cn.compare("NMSequentialIterComponent") == 0)
	{
		return qobject_cast<NMModelComponent*>(
				new NMSequentialIterComponent(this));
	}
    else if (cn.compare("NMConditionalIterComponent") == 0)
	{
		return qobject_cast<NMModelComponent*>(
				new NMConditionalIterComponent(this));
	}
    else if (cn.compare("NMDataComponent") == 0)
	{
		return qobject_cast<NMModelComponent*>(
				new NMDataComponent(this));
	}
    else if (cn.compare("NMDataRefComponent") == 0)
    {
        return qobject_cast<NMModelComponent*>(
                    new NMDataRefComponent(this));
    }
    else if (cn.compare("NMParameterTable") == 0)
    {
        return qobject_cast<NMModelComponent*>(
                 new NMParameterTable(this));
    }
    return 0;
}
