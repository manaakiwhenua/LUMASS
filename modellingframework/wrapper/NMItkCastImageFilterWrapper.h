/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2014 Landcare Research New Zealand Ltd
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
 * NMItkCastImageFilterWrapper.h
 *
 *  Created on: 2014-04-02
 *      Author: Alexander Herzig
 */

#ifndef NMItkCastImageFilterWrapper_H_
#define NMItkCastImageFilterWrapper_H_

#include "nmlog.h"
#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"

#include <string>
#include <iostream>
#include <QStringList>
#include <QList>

#include "nmitkcastimagefilterwrapper_export.h"

template<class TInputImage, class TOutputImage, unsigned int Dimension=2>
class NMItkCastImageFilterWrapper_Internal;

class NMITKCASTIMAGEFILTERWRAPPER_EXPORT NMItkCastImageFilterWrapper
        : public NMProcess
{
    Q_OBJECT



public:



public:
    NMItkCastImageFilterWrapper(QObject* parent=0);
    virtual ~NMItkCastImageFilterWrapper();

    template<class TInputImage, class TOutputImage, unsigned int Dimension>
    friend class NMItkCastImageFilterWrapper_Internal;

    QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
    void instantiateObject(void);

    void setNthInput(unsigned int numInput,
            QSharedPointer<NMItkDataObjectWrapper> imgWrapper, const QString& name);

    /*$<RATGetSupportDecl>$*/

    /*$<RATSetSupportDecl>$*/

protected:
    void linkParameters(unsigned int step,
            const QMap<QString, NMModelComponent*>& repo);



};

#endif /* NMItkCastImageFilterWrapper_H_ */
