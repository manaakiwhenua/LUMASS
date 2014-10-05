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
 * NMItkDataObjectWrapper.h
 *
 *  Created on: 22/04/2012
 *      Author: alex
 */

#ifndef NMITKDATAOBJECTWRAPPER_H_
#define NMITKDATAOBJECTWRAPPER_H_
#include "NMMacros.h"
#include <qobject.h>
#include <QMetaType>
#include "itkDataObject.h"
#include "otbImageIOBase.h"
#include "otbAttributeTable.h"

class NMItkDataObjectWrapper: public QObject
{
	Q_OBJECT
    Q_ENUMS(NMComponentType NMRegionType)
	Q_PROPERTY(NMComponentType NMComponentType READ getNMComponentType WRITE setNMComponentType)
	Q_PROPERTY(unsigned int NumDimensions READ getNumDimensions WRITE setNumDimensions)
	Q_PROPERTY(unsigned int NumBands READ getNumBands WRITE setNumBands)

public:
	NMItkDataObjectWrapper(QObject *parent=0);
	NMItkDataObjectWrapper(QObject* parent, QString str);
	NMItkDataObjectWrapper(QObject *parent, itk::DataObject* obj,
			otb::ImageIOBase::IOComponentType type,  unsigned int numDims,
			unsigned int numBands);
	NMItkDataObjectWrapper(const NMItkDataObjectWrapper& dataObjectWrapper);
	virtual ~NMItkDataObjectWrapper();

	enum NMComponentType {NM_UNKNOWN, NM_UCHAR, NM_CHAR, NM_USHORT, NM_SHORT,
		NM_UINT, NM_INT, NM_ULONG, NM_LONG, NM_FLOAT, NM_DOUBLE, NM_STRING};

    enum NMRegionType {NM_BUFFERED_REGION, NM_REQUESTED_REGION, NM_LARGESTPOSSIBLE_REGION};

	NMPropertyGetSet(NMComponentType, NMComponentType);
	NMPropertyGetSet(NumDimensions, unsigned int);
	NMPropertyGetSet(NumBands, unsigned int);
    NMPropertyGetSet(IsRGBImage, bool)

	void setDataObject(itk::DataObject* obj)
		{this->mDataObject = obj;}

	void setOTBTab(otb::AttributeTable::Pointer otbtab)
		{this->mOTBTab = otbtab;}

	otb::AttributeTable::Pointer getOTBTab(void)
		{return this->mOTBTab;}

	itk::DataObject* getDataObject(void)
		{return this->mDataObject;}

	void setStringObject(const QString& str)
		{this->mStringObject = str;}
	const QString& getStringObject(void)
		{return this->mStringObject;}

    void setImageRegion(NMRegionType regType, void* regObj);

	NMItkDataObjectWrapper& operator=(const NMItkDataObjectWrapper& dw);

	otb::ImageIOBase::IOComponentType getItkComponentType(void);
	void setItkComponentType(otb::ImageIOBase::IOComponentType);

	static const QString getComponentTypeString(NMItkDataObjectWrapper::NMComponentType type);
	static const NMItkDataObjectWrapper::NMComponentType
		getComponentTypeFromString(const QString& compType);

signals:
    void nmChanged();

private:
	itk::DataObject::Pointer mDataObject;
	otb::AttributeTable::Pointer mOTBTab;

	NMComponentType mNMComponentType;
	unsigned int mNumDimensions;
	unsigned int mNumBands;
    bool mIsRGBImage;

	QString mStringObject;

};

Q_DECLARE_METATYPE(NMItkDataObjectWrapper)
Q_DECLARE_METATYPE(NMItkDataObjectWrapper::NMComponentType)

#endif /* NMITKDATAOBJECTWRAPPER_H_ */
