/******************************************************************************
 * Created by Alexander Herzig
 * Copyright /*$<Year>$*/ Landcare Research New Zealand Ltd
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
 * /*$<WrapperClassName>$*/.h
 *
 *  Created on: /*$<FileDate>$*/
 *      Author: /*$<Author>$*/
 */

#ifndef /*$<WrapperClassName>$*/_H_
#define /*$<WrapperClassName>$*/_H_

#include "nmlog.h"
#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"

#include <string>
#include <iostream>
#include <QStringList>
#include <QList>

/*$<WrapperTemplateTypePamphlet>$*/
class /*$<WrapperClassName>$*/_Internal;

class
/*$<WrapperClassName>$*/
		: public NMProcess
{
	Q_OBJECT

    /*
	Q_PROPERTY(QStringList RadiusList READ getRadiusList WRITE setRadiusList)
	Q_PROPERTY(QList<QList<QStringList> > Weights READ getWeights WRITE setWeights)
	Q_PROPERTY(QList<QStringList> Values READ getValues WRITE setValues)
    */

    /*$<WrapperPropertyList>$*/

public:

    /*
	NMPropertyGetSet ( RadiusList, QStringList)
	NMPropertyGetSet ( Weights, QList<QList<QStringList> >)
	NMPropertyGetSet ( Values, QList<QStringList>)
    */

    /*$<WrapperPropertyGetSetter>$*/

    /*$<WrapperClassName>$*/(QObject* parent=0);
    virtual ~/*$<WrapperClassName>$*/();

    /*$<WrapperTemplateTypePamphlet>$*/
    friend class /*$<WrapperClassName>$*/_Internal;

	NMItkDataObjectWrapper* getOutput(unsigned int idx);
	void instantiateObject(void);

	void setNthInput(unsigned int numInput,
			NMItkDataObjectWrapper* imgWrapper);

protected:
    void linkParameters(unsigned int step,
    		const QMap<QString, NMModelComponent*>& repo);

    /*
    QStringList 				mRadiusList;
    QList<QList<QStringList> >  mWeights;
    QList<QStringList> 			mValues;
    */

    /*$<PropertyVarDef>$*/

};

#endif /* /*$<WrapperClassName>$*/_H_ */
