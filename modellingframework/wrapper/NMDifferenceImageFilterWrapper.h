/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2025 Landcare Research New Zealand Ltd
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
 * NMDifferenceImageFilterWrapper.h
 *
 *  Created on: 2025-02-04
 *      Author: makeWrapperConfigFile, Alex Herzig
 */

#ifndef NMDifferenceImageFilterWrapper_H_
#define NMDifferenceImageFilterWrapper_H_

#include <string>
#include <iostream>
#include <QStringList>
#include <QList>

#include "nmlog.h"
#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"

#include "nmdifferenceimagefilterwrapper_export.h"

template<class TInputImage, class TOutputImage, unsigned int Dimension=2>
class NMDifferenceImageFilterWrapper_Internal;

class
NMDifferenceImageFilterWrapper
        : public NMProcess
{
    Q_OBJECT

    
    Q_PROPERTY(QStringList ToleranceRadius READ getToleranceRadius WRITE setToleranceRadius)
    Q_PROPERTY(QStringList DifferenceThreshold READ getDifferenceThreshold WRITE setDifferenceThreshold)
    Q_PROPERTY(QStringList ResultFileName READ getResultFileName WRITE setResultFileName)
    Q_PROPERTY(bool PrintResults READ getPrintResults WRITE setPrintResults)

public:

    
    NMPropertyGetSet( ToleranceRadius, QStringList )
    NMPropertyGetSet( DifferenceThreshold, QStringList )
    NMPropertyGetSet( ResultFileName, QStringList )
    NMPropertyGetSet( PrintResults, bool )

public:
    NMDifferenceImageFilterWrapper(QObject* parent=0);
    virtual ~NMDifferenceImageFilterWrapper();

    template<class TInputImage, class TOutputImage, unsigned int Dimension>
    friend class NMDifferenceImageFilterWrapper_Internal;

    QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
    void instantiateObject(void);

    void setNthInput(unsigned int numInput,
              QSharedPointer<NMItkDataObjectWrapper> imgWrapper, const QString& name);

    void update();

    /*$<RATGetSupportDecl>$*/

    /*$<RATSetSupportDecl>$*/

protected:
    void linkParameters(unsigned int step,
            const QMap<QString, NMModelComponent*>& repo);

    void printResults(void);

    QStringList mToleranceRadius;
    QStringList mDifferenceThreshold;
    QStringList mResultFileName;
    bool mPrintResults;
    bool mHasCompleted;

};

#endif /* NMDifferenceImageFilterWrapper_H_ */
