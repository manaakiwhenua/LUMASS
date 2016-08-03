/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2016 Landcare Research New Zealand Ltd
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
 * NMVectorImageToImageFilterWrapper.h
 *
 *  Created on: 2016-08-03
 *      Author: Alexander Herzig
 */

#ifndef NMVectorImageToImageFilterWrapper_H_
#define NMVectorImageToImageFilterWrapper_H_

#include <string>
#include <iostream>
#include <QStringList>
#include <QList>

#include "nmlog.h"
#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"

#include "nmmodframe_export.h"

template<class TInputImage, class TOutputImage, unsigned int Dimension=2>
class NMVectorImageToImageFilterWrapper_Internal;

class
NMVectorImageToImageFilterWrapper
		: public NMProcess
{
	Q_OBJECT

    
    Q_PROPERTY(QStringList Band READ getBand WRITE setBand)

public:

    
    NMPropertyGetSet( Band, QStringList )

public:
    NMVectorImageToImageFilterWrapper(QObject* parent=0);
    virtual ~NMVectorImageToImageFilterWrapper();

    template<class TInputImage, class TOutputImage, unsigned int Dimension>
    friend class NMVectorImageToImageFilterWrapper_Internal;

    QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
    void instantiateObject(void);

    void setNthInput(unsigned int numInput,
              QSharedPointer<NMItkDataObjectWrapper> imgWrapper);

    /*$<RATGetSupportDecl>$*/

    /*$<RATSetSupportDecl>$*/

protected:
    void linkParameters(unsigned int step,
    		const QMap<QString, NMModelComponent*>& repo);

    
    QStringList mBand;

};

#endif /* NMVectorImageToImageFilterWrapper_H_ */
