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

template<class TInputImage, class TOutputImage, unsigned int Dimension=2>
class /*$<WrapperClassName>$*/_Internal;

class
/*$<WrapperClassName>$*/
		: public NMProcess
{
	Q_OBJECT

    /*$<WrapperPropertyList>$*/

public:

    /*$<WrapperPropertyGetSetter>$*/

public:
    /*$<WrapperClassName>$*/(QObject* parent=0);
    virtual ~/*$<WrapperClassName>$*/();

    template<class TInputImage, class TOutputImage, unsigned int Dimension>
    friend class /*$<WrapperClassName>$*/_Internal;

	NMItkDataObjectWrapper* getOutput(unsigned int idx);
	void instantiateObject(void);

	void setNthInput(unsigned int numInput,
			NMItkDataObjectWrapper* imgWrapper);

    /*$<RATGetSupportDecl>$*/

    /*$<RATSetSupportDecl>$*/

protected:
    void linkParameters(unsigned int step,
    		const QMap<QString, NMModelComponent*>& repo);

    /*$<PropertyVarDef>$*/

};

#endif /* /*$<WrapperClassName>$*/_H_ */
