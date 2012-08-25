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
 * NMModelController.cpp
 *
 *  Created on: 11/06/2012
 *      Author: alex
 */

#include "NMModelController.h"
#include "NMModelSerialiser.h"

NMModelController::NMModelController(QObject* parent)
{
	this->setParent(parent);
	this->ctx = "NMModelController";
	this->mRootComponent = 0;
}

NMModelController::~NMModelController()
{
}

NMModelComponent* NMModelController::identifyRootComponent(void)
{
	NMDebugCtx(ctx, << "...");

	NMModelComponent* root = 0;

	QMapIterator<QString, NMModelComponent*> cit(this->mComponentMap);
	while(cit.hasNext())
	{
		cit.next();
		NMDebugAI(<< "checking '" << cit.value()->objectName().toStdString()
				<< "' ...");
		if (cit.value()->getHostComponent() == 0)
		{
			root = cit.value();
			NMDebug(<< " got it !" << std::endl);
			break;
		}
		NMDebug(<< " nope!" << std::endl);
	}

	this->mRootComponent = root;

	NMDebugCtx(ctx, << "done!");
	return root;
}

void
NMModelController::execute()
{
	this->mRootComponent = this->identifyRootComponent();
	if (this->mRootComponent == 0)
	{
		NMErr(ctx, << "couldn't find a root component!");
		return;
	}
//	this->mRootComponent->initialiseComponents();
	this->mRootComponent->update(this->mComponentMap);
}

QString NMModelController::addComponent(NMModelComponent* comp,
		NMModelComponent* host)
{
	if (this->mComponentMap.values().contains(comp))
	{
		NMErr(ctx, << "model component already present in repository!");
		return "failed";
	}

	QString cname = comp->objectName();
	QString tname = cname;
	unsigned int cnt = 1;
	while (this->mComponentMap.keys().contains(tname))
	{
		tname = QString(tr("%1%2")).arg(cname).arg(cnt);
		++cnt;
	}

	comp->setObjectName(tname);
	this->mComponentMap.insert(tname, comp);
	comp->setParent(this);

	// check, whether we've go a valid host
	if (host != 0)
	{
		if (this->mComponentMap.keys().contains(host->objectName()))
		{
			host->addModelComponent(comp);
		}
	}

	return tname;
}

bool
NMModelController::contains(const QString& compName)
{
	bool ret;
	if (this->mComponentMap.keys().contains(compName))
		ret = true;
	else
		ret = false;

	return ret;
}

bool NMModelController::removeComponent(const QString& name)
{
	NMModelComponent* comp = this->getComponent(name);
	if (comp == 0)
	{
		NMErr(ctx, << "component '" << name.toStdString() << "' is not controlled by this "
				<< "controller!");
		return false;
	}

	NMModelComponent* host = comp->getHostComponent();
	if (host != 0)
		host->removeModelComponent(name);
	comp->destroySubComponents(this->mComponentMap);
	this->mComponentMap.remove(name);
	delete comp;

	return true;
}


NMModelComponent*
NMModelController::getComponent(const QString& name)
{
	NMModelComponent* comp = 0;
	QMap<QString, NMModelComponent*>::iterator cit = this->mComponentMap.find(name);
	if (cit != this->mComponentMap.end())
		comp = cit.value();

	return comp;
}
