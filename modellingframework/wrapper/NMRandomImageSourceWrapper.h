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
 * NMRandomImageSourceWrapper.h
 *
 *  Created on: 19/08/2012
 *      Author: alex
 */

#ifndef NMRANDOMIMAGESOURCEWRAPPER_H_
#define NMRANDOMIMAGESOURCEWRAPPER_H_

#include "nmlog.h"

#include <string>
#include <iostream>

#include <QStringList>
#include <QList>

#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"
#include "nmrandomimagesourcewrapper_export.h"

class NMRANDOMIMAGESOURCEWRAPPER_EXPORT NMRandomImageSourceWrapper : public NMProcess
{
	Q_OBJECT

	Q_PROPERTY(QList<QStringList> ImageSize READ getImageSize WRITE setImageSize)
	Q_PROPERTY(QList<QStringList> ImageSpacing READ getImageSpacing WRITE setImageSpacing)
	Q_PROPERTY(QList<QStringList> ImageOrigin READ getImageOrigin WRITE setImageOrigin)
	Q_PROPERTY(QStringList MaxValue READ getMaxValue WRITE setMaxValue)
	Q_PROPERTY(QStringList MinValue READ getMinValue WRITE setMinValue)

public:
	NMPropertyGetSet( ImageSize, QList<QStringList>);
	NMPropertyGetSet( ImageSpacing, QList<QStringList>);
	NMPropertyGetSet( ImageOrigin, QList<QStringList>);
	NMPropertyGetSet( MaxValue, QStringList);
	NMPropertyGetSet( MinValue, QStringList);

	NMRandomImageSourceWrapper(QObject* parent=0);
	~NMRandomImageSourceWrapper();

	QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
	void instantiateObject(void);

	void setNthInput(unsigned int numInput,
            QSharedPointer<NMItkDataObjectWrapper> imgWrapper){}

protected:

	void linkParameters(unsigned int step, const QMap<QString, NMModelComponent*>& repo);
	void extractNumericVector(unsigned int step, QList<QStringList> instring,
			std::vector<double>& numvec);
	bool extractNumericValue(unsigned int step,
			QStringList instring, double *val);

	void setVecParam(QString param, std::vector<double>& vec);
	void setParam(QString param, double val);
//	void setSize(void);
//	void setSpacing(void);
//	void setOrigin(void);
//	void setMax(void);
//	void setMin(void);

	std::string ctx;
	QList<QStringList> mImageSize;
	QList<QStringList> mImageSpacing;
	QList<QStringList> mImageOrigin;
	QStringList mMaxValue, mMinValue;
};


#endif /* NMRANDOMIMAGESOURCEWRAPPER_H_ */
