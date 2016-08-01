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
 * NMConditionalIterComponent.h
 *
 *  Created on: 12/02/2013
 *      Author: alex
 */

#ifndef NMCONDITIONALITERCOMPONENT_H_
#define NMCONDITIONALITERCOMPONENT_H_

#include "nmlog.h"
#include "NMMacros.h"
#include "NMProcess.h"

#include <NMIterableComponent.h>
#include <QMap>

#include "nmmodframe_export.h"

class NMMODFRAME_EXPORT NMConditionalIterComponent: public NMIterableComponent
{
	Q_OBJECT


public:
	NMConditionalIterComponent(QObject* parent=0);
	virtual ~NMConditionalIterComponent(void);

protected:
    void iterativeComponentUpdate(const QMap<QString, NMModelComponent*>& repo,
    		unsigned int minLevel, unsigned int maxLevel);

private:
	static const std::string ctx;
};

#endif /* NMCONDITIONALITERCOMPONENT_H_ */
