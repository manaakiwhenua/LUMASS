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
 * NMItkDataObjectWrapper.cpp
 *
 *  Created on: 22/04/2012
 *      Author: alex
 */

#include "NMItkDataObjectWrapper.h"
#include "itkDataObject.h"

NMItkDataObjectWrapper::NMItkDataObjectWrapper(QObject* parent)//, itk::DataObject* obj)
{
	this->setParent(parent);
	this->mDataObject = 0;
	this->mNumBands = 0;
	this->mNumDimensions = 1;
}

NMItkDataObjectWrapper::NMItkDataObjectWrapper(QObject* parent, QString str)
{
	this->setParent(parent);
	this->mDataObject = 0;
	this->mStringObject = str;
	this->mNumBands = 0;
	this->mNumDimensions = 0;
}

NMItkDataObjectWrapper::NMItkDataObjectWrapper(QObject *parent, itk::DataObject* obj,
		otb::ImageIOBase::IOComponentType type, unsigned int numDims, unsigned int numBands)
{
	this->setParent(parent);
	this->mDataObject = obj;
	this->setItkComponentType(type);
	this->setNumDimensions(numDims);
	this->setNumBands(numBands);
}

NMItkDataObjectWrapper::NMItkDataObjectWrapper(
		const NMItkDataObjectWrapper& dataObjectWrapper)
{
	NMItkDataObjectWrapper* w = const_cast<NMItkDataObjectWrapper*>(&dataObjectWrapper);
	this->setParent(w->parent());
	this->mDataObject = w->getDataObject();
	this->mNMComponentType = w->getNMComponentType();
	this->mNumDimensions = w->getNumDimensions();
	this->mNumBands = w->getNumBands();
}

NMItkDataObjectWrapper::~NMItkDataObjectWrapper()
{
}

NMItkDataObjectWrapper& NMItkDataObjectWrapper::operator=(const NMItkDataObjectWrapper& dw)
{
	this->setParent(dw.parent());
	NMItkDataObjectWrapper* w = const_cast<NMItkDataObjectWrapper*>(&dw);
	this->mDataObject = w->getDataObject();
	this->mNMComponentType = w->getNMComponentType();
	this->mNumDimensions = w->getNumDimensions();
	this->mNumBands = w->getNumBands();

	return *this;
}

otb::ImageIOBase::IOComponentType NMItkDataObjectWrapper::getItkComponentType()
{
	otb::ImageIOBase::IOComponentType type;
	switch (this->mNMComponentType)
	{
		case NM_UCHAR: type = otb::ImageIOBase::UCHAR; break;
		case NM_CHAR: type = otb::ImageIOBase::CHAR; break;
		case NM_USHORT: type = otb::ImageIOBase::USHORT; break;
		case NM_SHORT: type = otb::ImageIOBase::SHORT; break;
		case NM_UINT: type = otb::ImageIOBase::UINT; break;
		case NM_INT: type = otb::ImageIOBase::INT; break;
		case NM_ULONG: type = otb::ImageIOBase::ULONG; break;
		case NM_LONG: type = otb::ImageIOBase::LONG; break;
		case NM_FLOAT: type = otb::ImageIOBase::FLOAT; break;
		case NM_DOUBLE: type = otb::ImageIOBase::DOUBLE; break;
		default: type = otb::ImageIOBase::UNKNOWNCOMPONENTTYPE; break;
	}

	return type;
}

void NMItkDataObjectWrapper::setItkComponentType(otb::ImageIOBase::IOComponentType type)
{
	switch (type)
	{
		case otb::ImageIOBase::UCHAR	: this->mNMComponentType = NM_UCHAR	; break;
		case otb::ImageIOBase::CHAR		: this->mNMComponentType = NM_CHAR	; break;
		case otb::ImageIOBase::USHORT	: this->mNMComponentType = NM_USHORT; break;
		case otb::ImageIOBase::SHORT	: this->mNMComponentType = NM_SHORT	; break;
		case otb::ImageIOBase::UINT		: this->mNMComponentType = NM_UINT	; break;
		case otb::ImageIOBase::INT	 : this->mNMComponentType =	NM_INT	;	break;
		case otb::ImageIOBase::ULONG : this->mNMComponentType =	NM_ULONG;    break;
		case otb::ImageIOBase::LONG	 : this->mNMComponentType =	NM_LONG	;   break;
		case otb::ImageIOBase::FLOAT : this->mNMComponentType =	NM_FLOAT; break;
		case otb::ImageIOBase::DOUBLE: this->mNMComponentType =	NM_DOUBLE;	break;
		default: this->mNMComponentType = NM_UNKNOWN; break;
	}
}

const QString
NMItkDataObjectWrapper::getComponentTypeString(NMItkDataObjectWrapper::NMComponentType type)
{
	QString ret;
	switch (type)
	{
	case NM_UCHAR: ret = "uchar"; break;
	case NM_CHAR: ret = "char"; break;
	case NM_USHORT: ret = "ushort"; break;
	case NM_SHORT: ret = "short"; break;
	case NM_UINT: ret = "uint"; break;
	case NM_INT: ret = "int"; break;
	case NM_ULONG: ret = "ulong"; break;
	case NM_LONG: ret = "long"; break;
	case NM_FLOAT: ret = "float"; break;
	case NM_DOUBLE: ret = "double"; break;
	default: ret = "unknown"; break;
	}
	return ret;
}

const
NMItkDataObjectWrapper::NMComponentType
NMItkDataObjectWrapper::getComponentTypeFromString(const QString& compType)
{
	NMItkDataObjectWrapper::NMComponentType type = NMItkDataObjectWrapper::NM_UNKNOWN;
	if (compType.compare("uchar", Qt::CaseInsensitive) == 0)
		type = NMItkDataObjectWrapper::NM_UCHAR;
	else if (compType.compare("char", Qt::CaseInsensitive) == 0)
		type = NMItkDataObjectWrapper::NM_CHAR;
	else if (compType.compare("ushort", Qt::CaseInsensitive) == 0)
		type = NMItkDataObjectWrapper::NM_USHORT;
	else if (compType.compare("short", Qt::CaseInsensitive) == 0)
		type = NMItkDataObjectWrapper::NM_SHORT;
	else if (compType.compare("uint", Qt::CaseInsensitive) == 0)
		type = NMItkDataObjectWrapper::NM_UINT;
	else if (compType.compare("int", Qt::CaseInsensitive) == 0)
		type = NMItkDataObjectWrapper::NM_INT;
	else if (compType.compare("ulong", Qt::CaseInsensitive) == 0)
		type = NMItkDataObjectWrapper::NM_ULONG;
	else if (compType.compare("long", Qt::CaseInsensitive) == 0)
		type = NMItkDataObjectWrapper::NM_LONG;
	else if (compType.compare("float", Qt::CaseInsensitive) == 0)
		type = NMItkDataObjectWrapper::NM_FLOAT;
	else if (compType.compare("ushort", Qt::CaseInsensitive) == 0)
		type = NMItkDataObjectWrapper::NM_FLOAT;
	else if (compType.compare("double", Qt::CaseInsensitive) == 0)
		type = NMItkDataObjectWrapper::NM_DOUBLE;

	return type;
}
