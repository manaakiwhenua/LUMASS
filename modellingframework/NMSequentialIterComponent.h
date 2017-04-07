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
 * NMSequentialIterComponent.h
 *
 *  Created on: 12/02/2013
 *      Author: alex
 */

#ifndef NMSEQUENTIALITERCOMPONENT_H_
#define NMSEQUENTIALITERCOMPONENT_H_

#include "nmlog.h"
#include "NMMacros.h"

#include "NMIterableComponent.h"

#include <QMap>

#include "nmmodframe_export.h"

class NMMODFRAME_EXPORT NMSequentialIterComponent: public NMIterableComponent
{
	Q_OBJECT
    Q_PROPERTY(unsigned int NumIterations READ getNumIterations WRITE setNumIterations NOTIFY NMModelComponentChanged)
    Q_PROPERTY(QStringList NumIterationsExpression READ getNumIterationsExpression WRITE setNumIterationsExpression NOTIFY NMModelComponentChanged)


public:
	signals:
        void NMModelComponentChanged(void);
        void NumIterationsChanged(unsigned int numiter);

public:
	NMSequentialIterComponent(QObject* parent=0);
	virtual ~NMSequentialIterComponent(void);

    NMPropertyGetSet(NumIterationsExpression, QStringList)

    //NMPropertyGetSet(NumIterations, unsigned int)
    void setNumIterations(unsigned int numiter);
    unsigned int getNumIterations(void)
        {return this->mNumIterations;}

protected:
	unsigned int mNumIterations;
    QStringList mNumIterationsExpression;

    void iterativeComponentUpdate(const QMap<QString, NMModelComponent*>& repo,
    		unsigned int minLevel, unsigned int maxLevel);

    // note: step is 1-based as mIterationStep && mIterationStepRun!
    unsigned int evalNumIterationsExpression(const unsigned int& step);

private:
	static const std::string ctx;

};

#endif /* NMSEQUENTIALITERCOMPONENT_H_ */
