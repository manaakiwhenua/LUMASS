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
 * NMNeighbourhoodCountingWrapper.cpp
 *
 *  Created on: 8/08/2012
 *      Author: alex
 */

#include "NMNeighbourhoodCountingWrapper.h"
#include "NMMacros.h"

#include "itkProcessObject.h"
#include "itkDataObject.h"
#include "otbImage.h"
#include "otbNeighbourhoodCountingFilter.h"
//#include "otbNeighbourhoodCountingFilter_ExplicitInst.h"

#include <vector>
#include <string>

/** Helper Class for dealing with ugly templates */
template <class InPixelType, class OutPixelType, unsigned int Dimension>
class NMNeighbourhoodCountingWrapper_Internal
{
public:
	typedef otb::Image<InPixelType, Dimension> InImgType;
	typedef otb::Image<OutPixelType, Dimension> OutImgType;
	typedef otb::NeighbourhoodCountingFilter<InImgType, OutImgType> FilterType;
	typedef typename FilterType::Pointer FilterTypePointer;
	typedef typename InImgType::SizeType InImgSizeType;

	static void createInstance(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands)
	{
		FilterTypePointer f = FilterType::New();
		otbFilter = f;
	}

	static void setInternalTestValue(itk::ProcessObject::Pointer& otbFilter,
			int testvalue)
	{
		FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		filter->SetTestvalue(testvalue);
	}

	static void setInternalRadius(itk::ProcessObject::Pointer& otbFilter,
			std::vector<unsigned int> userradius)
	{
		FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		InImgSizeType radius;
		for (unsigned int r=0; r < Dimension; ++r)
		{
			radius[r] = userradius[r];
		}
		filter->SetRadius(radius);
	}

	static void setNthInput(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands, unsigned int idx, itk::DataObject* dataObj)
	{
		InImgType* img = dynamic_cast<InImgType*>(dataObj);
		FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		filter->SetInput(img);
	}

	static itk::DataObject* getOutput(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands, unsigned int idx)
	{
		FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		return dynamic_cast<OutImgType*>(filter->GetOutput(idx));
	}
};

template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, unsigned char, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, char, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, unsigned short, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, short, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, unsigned int, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, int, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, unsigned long, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, long, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, float, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, double, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<char, unsigned char, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<char, char, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<char, unsigned short, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<char, short, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<char, unsigned int, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<char, int, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<char, unsigned long, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<char, long, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<char, float, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<char, double, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, unsigned char, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, char, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, unsigned short, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, short, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, unsigned int, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, int, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, unsigned long, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, long, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, float, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, double, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<short, unsigned char, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<short, char, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<short, unsigned short, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<short, short, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<short, unsigned int, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<short, int, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<short, unsigned long, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<short, long, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<short, float, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<short, double, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, unsigned char, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, char, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, unsigned short, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, short, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, unsigned int, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, int, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, unsigned long, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, long, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, float, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, double, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<int, unsigned char, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<int, char, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<int, unsigned short, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<int, short, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<int, unsigned int, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<int, int, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<int, unsigned long, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<int, long, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<int, float, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<int, double, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, unsigned char, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, char, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, unsigned short, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, short, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, unsigned int, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, int, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, unsigned long, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, long, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, float, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, double, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<long, unsigned char, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<long, char, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<long, unsigned short, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<long, short, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<long, unsigned int, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<long, int, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<long, unsigned long, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<long, long, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<long, float, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<long, double, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<float, unsigned char, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<float, char, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<float, unsigned short, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<float, short, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<float, unsigned int, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<float, int, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<float, unsigned long, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<float, long, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<float, float, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<float, double, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<double, unsigned char, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<double, char, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<double, unsigned short, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<double, short, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<double, unsigned int, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<double, int, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<double, unsigned long, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<double, long, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<double, float, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<double, double, 1>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, unsigned char, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, char, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, unsigned short, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, short, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, unsigned int, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, int, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, unsigned long, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, long, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, float, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, double, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<char, unsigned char, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<char, char, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<char, unsigned short, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<char, short, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<char, unsigned int, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<char, int, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<char, unsigned long, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<char, long, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<char, float, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<char, double, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, unsigned char, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, char, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, unsigned short, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, short, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, unsigned int, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, int, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, unsigned long, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, long, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, float, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, double, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<short, unsigned char, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<short, char, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<short, unsigned short, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<short, short, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<short, unsigned int, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<short, int, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<short, unsigned long, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<short, long, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<short, float, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<short, double, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, unsigned char, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, char, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, unsigned short, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, short, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, unsigned int, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, int, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, unsigned long, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, long, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, float, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, double, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<int, unsigned char, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<int, char, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<int, unsigned short, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<int, short, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<int, unsigned int, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<int, int, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<int, unsigned long, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<int, long, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<int, float, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<int, double, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, unsigned char, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, char, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, unsigned short, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, short, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, unsigned int, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, int, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, unsigned long, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, long, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, float, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, double, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<long, unsigned char, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<long, char, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<long, unsigned short, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<long, short, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<long, unsigned int, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<long, int, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<long, unsigned long, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<long, long, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<long, float, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<long, double, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<float, unsigned char, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<float, char, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<float, unsigned short, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<float, short, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<float, unsigned int, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<float, int, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<float, unsigned long, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<float, long, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<float, float, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<float, double, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<double, unsigned char, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<double, char, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<double, unsigned short, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<double, short, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<double, unsigned int, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<double, int, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<double, unsigned long, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<double, long, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<double, float, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<double, double, 2>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, unsigned char, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, char, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, unsigned short, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, short, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, unsigned int, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, int, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, unsigned long, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, long, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, float, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned char, double, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<char, unsigned char, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<char, char, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<char, unsigned short, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<char, short, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<char, unsigned int, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<char, int, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<char, unsigned long, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<char, long, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<char, float, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<char, double, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, unsigned char, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, char, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, unsigned short, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, short, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, unsigned int, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, int, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, unsigned long, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, long, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, float, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned short, double, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<short, unsigned char, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<short, char, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<short, unsigned short, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<short, short, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<short, unsigned int, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<short, int, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<short, unsigned long, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<short, long, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<short, float, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<short, double, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, unsigned char, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, char, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, unsigned short, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, short, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, unsigned int, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, int, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, unsigned long, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, long, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, float, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned int, double, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<int, unsigned char, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<int, char, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<int, unsigned short, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<int, short, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<int, unsigned int, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<int, int, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<int, unsigned long, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<int, long, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<int, float, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<int, double, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, unsigned char, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, char, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, unsigned short, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, short, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, unsigned int, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, int, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, unsigned long, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, long, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, float, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<unsigned long, double, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<long, unsigned char, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<long, char, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<long, unsigned short, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<long, short, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<long, unsigned int, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<long, int, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<long, unsigned long, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<long, long, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<long, float, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<long, double, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<float, unsigned char, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<float, char, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<float, unsigned short, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<float, short, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<float, unsigned int, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<float, int, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<float, unsigned long, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<float, long, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<float, float, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<float, double, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<double, unsigned char, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<double, char, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<double, unsigned short, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<double, short, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<double, unsigned int, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<double, int, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<double, unsigned long, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<double, long, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<double, float, 3>;
template class NMNeighbourhoodCountingWrapper_Internal<double, double, 3>;


GetOutputWrap( NMNeighbourhoodCountingWrapper, NMNeighbourhoodCountingWrapper_Internal )
InstantiateObjectWrap( NMNeighbourhoodCountingWrapper, NMNeighbourhoodCountingWrapper_Internal )
SetNthInputWrap( NMNeighbourhoodCountingWrapper, NMNeighbourhoodCountingWrapper_Internal )

//// some more macros, we need to support all sorts of types and dimensions

/*!
 *  \brief set the radius of the kernel defining the neighbourhood
 */
#define callSetInternalRadius( inputType, outputType, wrapperName )			\
{	\
	if (this->mInputNumDimensions == 1)                                     \
	{                                                                       \
		wrapperName< inputType, outputType, 1>::setInternalRadius(       \
				this->mOtbProcess, kernelsize);                       \
	}                                                                       \
	else if (this->mInputNumDimensions == 2)                                \
	{                                                                       \
		wrapperName< inputType, outputType, 2>::setInternalRadius(       \
				this->mOtbProcess, kernelsize);                       \
	}                                                                       \
	else if (this->mInputNumDimensions == 3)                                \
	{                                                                       \
		wrapperName< inputType, outputType, 3>::setInternalRadius(       \
				this->mOtbProcess, kernelsize);                       \
	}                                                                       \
}


/*!
 * \brief set the value to test the neighbourhood pixels against
 */
#define callSetInternalTestValue( inputType, outputType, wrapperName )		\
{                                                                           \
	if (this->mInputNumDimensions == 1)                                     \
	{                                                                       \
		wrapperName< inputType, outputType, 1>::setInternalTestValue(       \
				this->mOtbProcess, this->mTestValue);                       \
	}                                                                       \
	else if (this->mInputNumDimensions == 2)                                \
	{                                                                       \
		wrapperName< inputType, outputType, 2>::setInternalTestValue(       \
				this->mOtbProcess, this->mTestValue);                       \
	}                                                                       \
	else if (this->mInputNumDimensions == 3)                                \
	{                                                                       \
		wrapperName< inputType, outputType, 3>::setInternalTestValue(       \
				this->mOtbProcess, this->mTestValue);                       \
	}                                                                       \
}


NMNeighbourhoodCountingWrapper::NMNeighbourhoodCountingWrapper(QObject* parent)
	: mTestValue(0), mKernelSizeX(1), mKernelSizeY(1), mKernelSizeZ(1)
{
	this->setParent(parent);
	this->ctx = "NMNeighbourhoodCountingWrapper";

	this->mInputComponentType = otb::ImageIOBase::SHORT;
	this->mOutputComponentType = otb::ImageIOBase::SHORT;
	this->mParameterHandling = NMProcess::NM_USE_UP;
	this->mParamPos = 0;
}

NMNeighbourhoodCountingWrapper::~NMNeighbourhoodCountingWrapper()
{
}


void
NMNeighbourhoodCountingWrapper::internalSetTestValue(void)
{
	if (!this->mbIsInitialised)
		return;

	switch (this->mInputComponentType)
	{
	case otb::ImageIOBase::UCHAR:                                               \
		outputTypeSwitch( unsigned char, callSetInternalTestValue, NMNeighbourhoodCountingWrapper_Internal );                                         \
		break;                                                                  \
	case otb::ImageIOBase::CHAR:                                                \
		outputTypeSwitch( char, callSetInternalTestValue, NMNeighbourhoodCountingWrapper_Internal );                                                  \
		break;                                                                  \
	case otb::ImageIOBase::USHORT:                                              \
		outputTypeSwitch( unsigned short, callSetInternalTestValue, NMNeighbourhoodCountingWrapper_Internal );                                        \
		break;                                                                  \
	case otb::ImageIOBase::SHORT:                                               \
		outputTypeSwitch( short, callSetInternalTestValue, NMNeighbourhoodCountingWrapper_Internal );                                                 \
		break;                                                                  \
	case otb::ImageIOBase::UINT:                                                \
		outputTypeSwitch( unsigned int, callSetInternalTestValue, NMNeighbourhoodCountingWrapper_Internal );                                          \
		break;                                                                  \
	case otb::ImageIOBase::INT:                                                 \
		outputTypeSwitch( int, callSetInternalTestValue, NMNeighbourhoodCountingWrapper_Internal );                                                   \
		break;                                                                  \
	case otb::ImageIOBase::ULONG:                                               \
		outputTypeSwitch( unsigned long, callSetInternalTestValue, NMNeighbourhoodCountingWrapper_Internal );                                         \
		break;                                                                  \
	case otb::ImageIOBase::LONG:                                                \
		outputTypeSwitch( long, callSetInternalTestValue, NMNeighbourhoodCountingWrapper_Internal );                                                  \
		break;                                                                  \
	case otb::ImageIOBase::FLOAT:                                               \
		outputTypeSwitch( float, callSetInternalTestValue, NMNeighbourhoodCountingWrapper_Internal );                                                 \
		break;                                                                  \
	case otb::ImageIOBase::DOUBLE:                                              \
		outputTypeSwitch( double, callSetInternalTestValue, NMNeighbourhoodCountingWrapper_Internal );                                                \
		break;																	\
	default:
		break;
	}
}

void
NMNeighbourhoodCountingWrapper::internalSetRadius(void)
{
	if (!this->mbIsInitialised)
		return;

	std::vector<unsigned int> kernelsize;
	kernelsize.push_back(this->mKernelSizeX);
	kernelsize.push_back(this->mKernelSizeY);
	kernelsize.push_back(this->mKernelSizeZ);

	switch (this->mInputComponentType)
	{
	case otb::ImageIOBase::UCHAR:                                               \
		outputTypeSwitch( unsigned char, callSetInternalRadius, NMNeighbourhoodCountingWrapper_Internal );                                         \
		break;                                                                  \
	case otb::ImageIOBase::CHAR:                                                \
		outputTypeSwitch( char, callSetInternalRadius, NMNeighbourhoodCountingWrapper_Internal );                                                  \
		break;                                                                  \
	case otb::ImageIOBase::USHORT:                                              \
		outputTypeSwitch( unsigned short, callSetInternalRadius, NMNeighbourhoodCountingWrapper_Internal );                                        \
		break;                                                                  \
	case otb::ImageIOBase::SHORT:                                               \
		outputTypeSwitch( short, callSetInternalRadius, NMNeighbourhoodCountingWrapper_Internal );                                                 \
		break;                                                                  \
	case otb::ImageIOBase::UINT:                                                \
		outputTypeSwitch( unsigned int, callSetInternalRadius, NMNeighbourhoodCountingWrapper_Internal );                                          \
		break;                                                                  \
	case otb::ImageIOBase::INT:                                                 \
		outputTypeSwitch( int, callSetInternalRadius, NMNeighbourhoodCountingWrapper_Internal );                                                   \
		break;                                                                  \
	case otb::ImageIOBase::ULONG:                                               \
		outputTypeSwitch( unsigned long, callSetInternalRadius, NMNeighbourhoodCountingWrapper_Internal );                                         \
		break;                                                                  \
	case otb::ImageIOBase::LONG:                                                \
		outputTypeSwitch( long, callSetInternalRadius, NMNeighbourhoodCountingWrapper_Internal );                                                  \
		break;                                                                  \
	case otb::ImageIOBase::FLOAT:                                               \
		outputTypeSwitch( float, callSetInternalRadius, NMNeighbourhoodCountingWrapper_Internal );                                                 \
		break;                                                                  \
	case otb::ImageIOBase::DOUBLE:                                              \
		outputTypeSwitch( double, callSetInternalRadius, NMNeighbourhoodCountingWrapper_Internal );                                                \
		break;																	\
	default:
		break;
	}
}


void
NMNeighbourhoodCountingWrapper::linkParameters(unsigned int step,
		const QMap<QString, NMModelComponent*>& repo)
{
	NMDebugCtx(ctx, << "...");

    //if (step > this->mTestValueList.size()-1)
    //	step = 0;

    if (this->mTestValueList.size())
	{
        int pos = this->mapHostIndexToPolicyIndex(step, mTestValueList.size());

		bool bok;
        int val = this->mTestValueList.at(pos).toInt(&bok);
		if (bok)
			this->mTestValue = val;
	}
	this->internalSetTestValue();
	this->internalSetRadius();

	NMDebugCtx(ctx, << "done!");
}



