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
 * NMStreamingImageFileWriterWrapper.cpp
 *
 *  Created on: 17/05/2012
 *      Author: alex
 */


#include "NMMacros.h"
#include "NMStreamingImageFileWriterWrapper.h"
#include "otbImage.h"
#include "otbVectorImage.h"
#include "itkSmartPointer.h"
#include "itkProcessObject.h"
#include "otbAttributeTable.h"
#include "otbStreamingRATImageFileWriter.h"

#ifdef BUILD_RASSUPPORT
	#include "RasdamanConnector.hh"
	#include "otbRasdamanImageIO.h"
#endif


/** Helper Classes */
template <class PixelType, class PixelType2, unsigned int Dimension>
class NMStreamingImageFileWriterWrapper_Internal
{
public:

    typedef itk::RGBPixel< PixelType >                  RGBPixelType;
    typedef otb::Image< RGBPixelType, Dimension >       RGBImgType;

	typedef otb::Image<PixelType, Dimension> 			ImgType;
	typedef otb::VectorImage<PixelType, Dimension> 		VecImgType;

    typedef otb::StreamingRATImageFileWriter<RGBImgType>    RGBFilterType;
    typedef otb::StreamingRATImageFileWriter<ImgType> 		FilterType;
	typedef otb::StreamingRATImageFileWriter<VecImgType> 	VecFilterType;

    typedef typename RGBFilterType::Pointer             RGBFilterTypePointer;
    typedef typename FilterType::Pointer 				FilterTypePointer;
	typedef typename VecFilterType::Pointer 			VecFilterTypePointer;

	static void createInstance(itk::ProcessObject::Pointer& otbFilter,
            unsigned int numBands, bool rgbMode)
		{
			if (numBands == 1)
			{
				FilterTypePointer f = FilterType::New();
				otbFilter = f;
			}
            else if (numBands == 3 && rgbMode)
            {
                RGBFilterTypePointer r = RGBFilterType::New();
                otbFilter = r;
            }
			else
			{
				VecFilterTypePointer v = VecFilterType::New();
				otbFilter = v;
			}
		}

    static void setUpdateMode(itk::ProcessObject::Pointer& otbFilter,
                              unsigned int numBands, bool updateMode, bool rgbMode)
        {
            if (numBands == 1)
            {
                FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
                filter->SetUpdateMode(updateMode);
            }
            else if (numBands == 3 && rgbMode)
            {
                RGBFilterType* filter = dynamic_cast<RGBFilterType*>(otbFilter.GetPointer());
                filter->SetUpdateMode(updateMode);
            }
            else
            {
                VecFilterType* filter = dynamic_cast<VecFilterType*>(otbFilter.GetPointer());
                filter->SetUpdateMode(updateMode);
            }
        }

	static void setFileName(itk::ProcessObject::Pointer& otbFilter,
            unsigned int numBands, QString& fileName,

#ifdef BUILD_RASSUPPORT

            otb::RasdamanImageIO::Pointer& rio,
#endif
            bool rgbMode)
		{
			if (numBands == 1)
			{
				FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
				filter->SetFileName(fileName.toStdString());

#ifdef BUILD_RASSUPPORT
				if (rio.IsNotNull())
					filter->SetImageIO(rio);
#endif
			}
            else if (numBands == 3 && rgbMode)
            {
                RGBFilterType* filter = dynamic_cast<RGBFilterType*>(otbFilter.GetPointer());
                filter->SetFileName(fileName.toStdString());

#ifdef BUILD_RASSUPPORT
                if (rio.IsNotNull())
                    filter->SetImageIO(rio);
#endif
            }
			else
			{
				VecFilterType* filter = dynamic_cast<VecFilterType*>(otbFilter.GetPointer());
				filter->SetFileName(fileName.toStdString());

#ifdef BUILD_RASSUPPORT
				if (rio.IsNotNull())
					filter->SetImageIO(rio);
#endif
				}
		}

	static void setNthInput(itk::ProcessObject::Pointer& otbFilter,
            unsigned int numBands, unsigned int idx, itk::DataObject* dataObj, bool rgbMode, bool writeImage)
		{
			if (numBands == 1)
			{
				FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
				ImgType* img = dynamic_cast<ImgType*>(dataObj);
				filter->SetInput(img);
                filter->SetWriteImage(writeImage);
			}
            else if (numBands == 3 && rgbMode)
            {
                RGBFilterType* filter = dynamic_cast<RGBFilterType*>(otbFilter.GetPointer());
                RGBImgType* img = dynamic_cast<RGBImgType*>(dataObj);
                filter->SetInput(img);
                filter->SetWriteImage(writeImage);
            }
			else
			{
				VecImgType* img = dynamic_cast<VecImgType*>(dataObj);
				VecFilterType* filter = dynamic_cast<VecFilterType*>(otbFilter.GetPointer());
				filter->SetInput(img);
                filter->SetWriteImage(writeImage);
			}
		}

	static void setNthAttributeTable(itk::ProcessObject::Pointer& otbFilter,
            unsigned int numBands, unsigned int idx, otb::AttributeTable::Pointer& tab, bool rgbMode)
		{
			if (numBands == 1)
			{
				FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
				filter->SetInputRAT(tab);
			}
            else if (numBands == 3 && rgbMode)
            {
                RGBFilterType* filter = dynamic_cast<RGBFilterType*>(otbFilter.GetPointer());
                filter->SetInputRAT(tab);
            }
			else
			{
				VecFilterType* filter = dynamic_cast<VecFilterType*>(otbFilter.GetPointer());
				filter->SetInputRAT(tab);
			}
		}

    static void setResamplingType(itk::ProcessObject::Pointer& otbFilter,
                                  unsigned int numBands, const QString& resamplingType, bool rgbMode)
        {
            if (numBands == 1)
            {
                FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
                filter->SetResamplingType(resamplingType.toStdString());
            }
            else if (numBands == 3 && rgbMode)
            {
                RGBFilterType* filter = dynamic_cast<RGBFilterType*>(otbFilter.GetPointer());
                filter->SetResamplingType(resamplingType.toStdString());
            }
            else
            {
                VecFilterType* filter = dynamic_cast<VecFilterType*>(otbFilter.GetPointer());
                filter->SetResamplingType(resamplingType.toStdString());
            }
        }

    static void setStreamingMethod(itk::ProcessObject::Pointer& otbFilter,
                                  unsigned int numBands, const QString& StreamingMethod, bool rgbMode)
        {
            if (numBands == 1)
            {
                FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
                filter->SetStreamingMethod(StreamingMethod.toStdString());
            }
            else if (numBands == 3 && rgbMode)
            {
                RGBFilterType* filter = dynamic_cast<RGBFilterType*>(otbFilter.GetPointer());
                filter->SetStreamingMethod(StreamingMethod.toStdString());
            }
            else
            {
                VecFilterType* filter = dynamic_cast<VecFilterType*>(otbFilter.GetPointer());
                filter->SetStreamingMethod(StreamingMethod.toStdString());
            }
        }

    static void setStreamingSize(itk::ProcessObject::Pointer& otbFilter,
                                  unsigned int numBands, const int streamSize, bool rgbMode)
        {
            if (numBands == 1)
            {
                FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
                filter->SetStreamingSize(streamSize);
            }
            else if (numBands == 3 && rgbMode)
            {
                RGBFilterType* filter = dynamic_cast<RGBFilterType*>(otbFilter.GetPointer());
                filter->SetStreamingSize(streamSize);
            }
            else
            {
                VecFilterType* filter = dynamic_cast<VecFilterType*>(otbFilter.GetPointer());
                filter->SetStreamingSize(streamSize);
            }
        }

};

template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, unsigned char, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, char, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, unsigned short, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, short, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, unsigned int, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, int, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, unsigned long, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, long, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, float, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, double, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<char, unsigned char, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<char, char, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<char, unsigned short, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<char, short, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<char, unsigned int, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<char, int, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<char, unsigned long, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<char, long, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<char, float, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<char, double, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, unsigned char, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, char, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, unsigned short, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, short, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, unsigned int, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, int, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, unsigned long, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, long, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, float, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, double, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<short, unsigned char, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<short, char, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<short, unsigned short, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<short, short, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<short, unsigned int, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<short, int, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<short, unsigned long, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<short, long, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<short, float, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<short, double, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, unsigned char, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, char, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, unsigned short, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, short, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, unsigned int, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, int, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, unsigned long, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, long, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, float, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, double, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<int, unsigned char, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<int, char, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<int, unsigned short, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<int, short, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<int, unsigned int, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<int, int, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<int, unsigned long, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<int, long, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<int, float, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<int, double, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, unsigned char, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, char, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, unsigned short, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, short, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, unsigned int, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, int, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, unsigned long, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, long, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, float, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, double, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<long, unsigned char, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<long, char, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<long, unsigned short, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<long, short, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<long, unsigned int, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<long, int, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<long, unsigned long, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<long, long, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<long, float, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<long, double, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<float, unsigned char, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<float, char, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<float, unsigned short, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<float, short, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<float, unsigned int, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<float, int, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<float, unsigned long, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<float, long, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<float, float, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<float, double, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<double, unsigned char, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<double, char, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<double, unsigned short, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<double, short, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<double, unsigned int, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<double, int, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<double, unsigned long, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<double, long, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<double, float, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<double, double, 1>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, unsigned char, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, char, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, unsigned short, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, short, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, unsigned int, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, int, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, unsigned long, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, long, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, float, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, double, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<char, unsigned char, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<char, char, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<char, unsigned short, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<char, short, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<char, unsigned int, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<char, int, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<char, unsigned long, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<char, long, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<char, float, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<char, double, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, unsigned char, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, char, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, unsigned short, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, short, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, unsigned int, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, int, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, unsigned long, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, long, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, float, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, double, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<short, unsigned char, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<short, char, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<short, unsigned short, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<short, short, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<short, unsigned int, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<short, int, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<short, unsigned long, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<short, long, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<short, float, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<short, double, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, unsigned char, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, char, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, unsigned short, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, short, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, unsigned int, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, int, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, unsigned long, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, long, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, float, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, double, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<int, unsigned char, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<int, char, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<int, unsigned short, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<int, short, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<int, unsigned int, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<int, int, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<int, unsigned long, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<int, long, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<int, float, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<int, double, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, unsigned char, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, char, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, unsigned short, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, short, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, unsigned int, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, int, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, unsigned long, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, long, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, float, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, double, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<long, unsigned char, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<long, char, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<long, unsigned short, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<long, short, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<long, unsigned int, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<long, int, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<long, unsigned long, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<long, long, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<long, float, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<long, double, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<float, unsigned char, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<float, char, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<float, unsigned short, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<float, short, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<float, unsigned int, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<float, int, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<float, unsigned long, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<float, long, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<float, float, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<float, double, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<double, unsigned char, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<double, char, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<double, unsigned short, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<double, short, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<double, unsigned int, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<double, int, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<double, unsigned long, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<double, long, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<double, float, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<double, double, 2>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, unsigned char, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, char, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, unsigned short, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, short, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, unsigned int, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, int, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, unsigned long, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, long, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, float, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned char, double, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<char, unsigned char, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<char, char, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<char, unsigned short, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<char, short, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<char, unsigned int, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<char, int, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<char, unsigned long, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<char, long, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<char, float, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<char, double, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, unsigned char, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, char, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, unsigned short, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, short, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, unsigned int, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, int, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, unsigned long, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, long, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, float, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned short, double, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<short, unsigned char, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<short, char, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<short, unsigned short, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<short, short, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<short, unsigned int, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<short, int, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<short, unsigned long, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<short, long, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<short, float, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<short, double, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, unsigned char, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, char, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, unsigned short, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, short, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, unsigned int, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, int, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, unsigned long, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, long, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, float, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned int, double, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<int, unsigned char, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<int, char, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<int, unsigned short, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<int, short, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<int, unsigned int, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<int, int, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<int, unsigned long, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<int, long, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<int, float, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<int, double, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, unsigned char, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, char, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, unsigned short, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, short, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, unsigned int, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, int, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, unsigned long, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, long, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, float, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<unsigned long, double, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<long, unsigned char, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<long, char, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<long, unsigned short, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<long, short, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<long, unsigned int, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<long, int, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<long, unsigned long, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<long, long, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<long, float, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<long, double, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<float, unsigned char, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<float, char, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<float, unsigned short, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<float, short, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<float, unsigned int, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<float, int, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<float, unsigned long, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<float, long, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<float, float, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<float, double, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<double, unsigned char, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<double, char, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<double, unsigned short, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<double, short, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<double, unsigned int, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<double, int, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<double, unsigned long, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<double, long, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<double, float, 3>;
template class NMStreamingImageFileWriterWrapper_Internal<double, double, 3>;


#ifdef BUILD_RASSUPPORT
	#define callSetFileName( PixelType, wrapName ) \
		if (this->mOutputNumDimensions == 1)                                    \
		{                                                                   \
			wrapName<PixelType, PixelType, 1>::setFileName(this->mOtbProcess,   \
                    this->mOutputNumBands, fileName, rio, mRGBMode);                  \
		}                                                                   \
		else if (this->mOutputNumDimensions == 2)                               \
		{                                                                   \
			wrapName<PixelType, PixelType, 2>::setFileName(this->mOtbProcess,   \
                    this->mOutputNumBands, fileName, rio, mRGBMode);                  \
		}                                                                   \
		else if (this->mOutputNumDimensions == 3)                               \
		{                                                                   \
			wrapName<PixelType, PixelType, 3>::setFileName(this->mOtbProcess,   \
                    this->mOutputNumBands, fileName, rio, mRGBMode);                  \
		}
#else
	#define callSetFileName( PixelType, wrapName ) \
		if (this->mOutputNumDimensions == 1)                                    \
		{                                                                   \
			wrapName<PixelType, PixelType, 1>::setFileName(this->mOtbProcess,   \
                    this->mOutputNumBands, fileName, mRGBMode);                  \
		}                                                                   \
		else if (this->mOutputNumDimensions == 2)                               \
		{                                                                   \
			wrapName<PixelType, PixelType, 2>::setFileName(this->mOtbProcess,   \
                    this->mOutputNumBands, fileName, mRGBMode);                  \
		}                                                                   \
		else if (this->mOutputNumDimensions == 3)                               \
		{                                                                   \
			wrapName<PixelType, PixelType, 3>::setFileName(this->mOtbProcess,   \
                    this->mOutputNumBands, fileName, mRGBMode);                  \
		}
#endif // BUILD_RASSUPPORT

#define callOutputTypeSetTab( imgType, wrapName ) \
{ \
	if (this->mOutputNumDimensions == 1) \
	{ \
		wrapName< imgType, imgType, 1 >::setNthAttributeTable( \
                this->mOtbProcess, this->mOutputNumBands, tabIdx, tab, mRGBMode); \
	} \
	else if (this->mOutputNumDimensions == 2) \
	{ \
		wrapName< imgType, imgType, 2 >::setNthAttributeTable( \
                this->mOtbProcess, this->mOutputNumBands, tabIdx, tab, mRGBMode); \
	} \
	else if (this->mOutputNumDimensions == 3) \
	{ \
		wrapName< imgType, imgType, 3 >::setNthAttributeTable( \
                this->mOtbProcess, this->mOutputNumBands, tabIdx, tab, mRGBMode); \
	}\
}

#define callFilterSetInput( imgType, wrapName ) \
{ \
	if (this->mOutputNumDimensions == 1) \
	{ \
		wrapName< imgType, imgType, 1 >::setNthInput( \
                this->mOtbProcess, this->mOutputNumBands, numInput, img, mRGBMode, mWriteImage); \
	} \
	else if (this->mOutputNumDimensions == 2) \
	{ \
		wrapName< imgType, imgType, 2 >::setNthInput( \
                this->mOtbProcess, this->mOutputNumBands, numInput, img, mRGBMode, mWriteImage); \
	} \
	else if (this->mOutputNumDimensions == 3) \
	{ \
		wrapName< imgType, imgType, 3 >::setNthInput( \
                this->mOtbProcess, this->mOutputNumBands, numInput, img, mRGBMode, mWriteImage); \
	}\
}

#define callSetUpdateMode( imgType, wrapName ) \
{ \
    if (this->mOutputNumDimensions == 1) \
    { \
        wrapName< imgType, imgType, 1 >::setUpdateMode( \
                this->mOtbProcess, this->mOutputNumBands, this->mUpdateMode, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 2) \
    { \
        wrapName< imgType, imgType, 2 >::setUpdateMode( \
                this->mOtbProcess, this->mOutputNumBands, this->mUpdateMode, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 3) \
    { \
        wrapName< imgType, imgType, 3 >::setUpdateMode( \
                this->mOtbProcess, this->mOutputNumBands, this->mUpdateMode, mRGBMode); \
    }\
}

#define callSetResamplingType( imgType, wrapName ) \
{ \
    if (this->mOutputNumDimensions == 1) \
    { \
        wrapName< imgType, imgType, 1 >::setResamplingType( \
                this->mOtbProcess, this->mOutputNumBands, mPyramidResamplingType, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 2) \
    { \
        wrapName< imgType, imgType, 2 >::setResamplingType( \
                this->mOtbProcess, this->mOutputNumBands, mPyramidResamplingType, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 3) \
    { \
        wrapName< imgType, imgType, 3 >::setResamplingType( \
                this->mOtbProcess, this->mOutputNumBands, mPyramidResamplingType, mRGBMode); \
    }\
}

#define callSetStreamingMethod( imgType, wrapName ) \
{ \
    if (this->mOutputNumDimensions == 1) \
    { \
        wrapName< imgType, imgType, 1 >::setStreamingMethod( \
                this->mOtbProcess, this->mOutputNumBands, mStreamingMethodType, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 2) \
    { \
        wrapName< imgType, imgType, 2 >::setStreamingMethod( \
                this->mOtbProcess, this->mOutputNumBands, mStreamingMethodType, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 3) \
    { \
        wrapName< imgType, imgType, 3 >::setStreamingMethod( \
                this->mOtbProcess, this->mOutputNumBands, mStreamingMethodType, mRGBMode); \
    }\
}

#define callSetStreamingSize( imgType, wrapName ) \
{ \
    if (this->mOutputNumDimensions == 1) \
    { \
        wrapName< imgType, imgType, 1 >::setStreamingSize( \
                this->mOtbProcess, this->mOutputNumBands, mStreamingSize, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 2) \
    { \
        wrapName< imgType, imgType, 2 >::setStreamingSize( \
                this->mOtbProcess, this->mOutputNumBands, mStreamingSize, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 3) \
    { \
        wrapName< imgType, imgType, 3 >::setStreamingSize( \
                this->mOtbProcess, this->mOutputNumBands, mStreamingSize, mRGBMode); \
    }\
}

#define callInitWriter( imgType, wrapName ) \
{ \
    if (this->mOutputNumDimensions == 1) \
    { \
        wrapName< imgType, imgType, 1 >::createInstance( \
                this->mOtbProcess, this->mOutputNumBands, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 2) \
    { \
        wrapName< imgType, imgType, 2 >::createInstance( \
                this->mOtbProcess, this->mOutputNumBands, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 3) \
    { \
        wrapName< imgType, imgType, 3 >::createInstance( \
                this->mOtbProcess, this->mOutputNumBands, mRGBMode); \
    }\
}

//InstantiateObjectWrap( NMStreamingImageFileWriterWrapper, NMStreamingImageFileWriterWrapper_Internal )
//SetNthInputWrap( NMStreamingImageFileWriterWrapper, NMStreamingImageFileWriterWrapper_Internal )

NMStreamingImageFileWriterWrapper
::NMStreamingImageFileWriterWrapper(QObject* parent)
{
	this->setParent(parent);
	this->setObjectName(tr("NMStreamingImageFileWriterWrapper"));
	this->mbIsInitialised = false;
	this->mInputComponentType = otb::ImageIOBase::FLOAT;
	this->mOutputComponentType = otb::ImageIOBase::FLOAT;
	this->mInputNumBands = 1;
	this->mOutputNumBands = 1;
	this->mInputNumDimensions = 2;
	this->mOutputNumDimensions = 2;
    this->mWriteTable = true;
    this->mWriteImage = true;
    this->mUpdateMode = false;
    this->mRGBMode = false;

    this->mStreamingSize = 512;

    this->mPyramidResamplingType = QString(tr("NEAREST"));
    mPyramidResamplingEnum.clear();
    mPyramidResamplingEnum
            << "NONE" << "NEAREST" << "GAUSS" << "CUBIC"
            << "AVERAGE" << "MODE";

    this->mStreamingMethodType = QString(tr("STRIPPED"));
    this->mStreamingMethodEnum.clear();
    this->mStreamingMethodEnum << "STRIPPED" << "TILED" << "NO_STREAMING";

#ifdef BUILD_RASSUPPORT
	this->mRasConnector = 0;
	this->mParameterHandling = NMProcess::NM_USE_UP;
	this->mParamPos = 0;
#endif
}

NMStreamingImageFileWriterWrapper
::NMStreamingImageFileWriterWrapper(QObject* parent,
			otb::ImageIOBase::IOComponentType componentType,
			unsigned int numDims, unsigned int numBands)
{
	this->setParent(parent);
	this->setObjectName(tr("NMStreamingImageFileWriterWrapper"));
	this->mbIsInitialised = false;
	this->mInputComponentType = componentType;
	this->mOutputComponentType = componentType;
	this->mInputNumBands = numBands;
	this->mOutputNumBands = numBands;
	this->mInputNumDimensions = numDims;
	this->mOutputNumDimensions = numDims;
    this->mWriteImage = true;
    this->mWriteTable = true;
    this->mUpdateMode = false;
    this->mRGBMode = false;

    this->mStreamingSize = 512;

    this->mPyramidResamplingType = QString(tr("NEAREST"));
    mPyramidResamplingEnum.clear();
    mPyramidResamplingEnum
            << "NONE" << "NEAREST" << "GAUSS" << "CUBIC"
            << "AVERAGE" << "MODE";

    this->mStreamingMethodType = QString(tr("STRIPPED"));
    this->mStreamingMethodEnum.clear();
    this->mStreamingMethodEnum << "STRIPPED" << "TILED";


#ifdef BUILD_RASSUPPORT
	this->mRasConnector = 0;
    this->mParameterHandling = NMProcess::NM_USE_UP;
    this->mParamPos = 0;
#endif
}

NMStreamingImageFileWriterWrapper::~NMStreamingImageFileWriterWrapper()
{
}

void
NMStreamingImageFileWriterWrapper
::setInternalFileName(QString fileName)
{
	if (!this->mbIsInitialised)
		return;

#ifdef BUILD_RASSUPPORT
	otb::RasdamanImageIO::Pointer rio;
	if (mRasConnector != 0 && this->mRasConnector->getConnector() != 0)
	{
		rio = otb::RasdamanImageIO::New();
		rio->setRasdamanConnector(const_cast<RasdamanConnector*>(
				this->mRasConnector->getConnector()));
	}
#endif

	switch(this->mOutputComponentType)
	{
	MacroPerType( callSetFileName, NMStreamingImageFileWriterWrapper_Internal )
	default:
		break;
	}
}

void
NMStreamingImageFileWriterWrapper
::instantiateObject(void)
{
    if (this->mbIsInitialised)
        return;

    bool init = true;
    switch(this->mOutputComponentType)
    {
    MacroPerType( callInitWriter, NMStreamingImageFileWriterWrapper_Internal )
    default:
        init = false;
        break;
    }
    this->mbIsInitialised = init;
    NMDebugAI( << "... " << this->objectName().toStdString() << " - " << init << std::endl);
}

void
NMStreamingImageFileWriterWrapper
::setInternalResamplingType()
{
    if (!this->mbIsInitialised)
        return;

    switch(this->mOutputComponentType)
    {
    MacroPerType( callSetResamplingType, NMStreamingImageFileWriterWrapper_Internal )
    default:
        break;
    }
}

void
NMStreamingImageFileWriterWrapper
::setInternalStreamingMethod()
{
    if (!this->mbIsInitialised)
        return;

    switch(this->mOutputComponentType)
    {
    MacroPerType( callSetStreamingMethod, NMStreamingImageFileWriterWrapper_Internal )
    default:
        break;
    }
}

void
NMStreamingImageFileWriterWrapper
::setInternalStreamingSize()
{
    if (!this->mbIsInitialised)
        return;

    switch(this->mOutputComponentType)
    {
    MacroPerType( callSetStreamingSize, NMStreamingImageFileWriterWrapper_Internal )
    default:
        break;
    }
}

void
NMStreamingImageFileWriterWrapper
::linkParameters(unsigned int step, const QMap<QString, NMModelComponent*>& repo)
{
    //if (step > this->mFileNames.size()-1)
    //	step = 0;
//    step = this->mapHostIndexToPolicyIndex(step, mFileNames.size());

//	if (step < this->mFileNames.size())
//	{
//		this->setInternalFileName(this->mFileNames.at(step));
//	}

    QVariant param = this->getParameter("FileNames");
    if (param.isValid())
    {
        this->setInternalFileName(param.toString());
        QString fnProvNAttr = QString("nm:FileNames=\"%1\"")
                              .arg(param.toString());
        this->addRunTimeParaProvN(fnProvNAttr);
    }

    QVariant param2 = this->getParameter("InputTables");
    if (param2.isValid())
    {
        this->setInternalInputTable(param2.toString(), repo);
        QString tabProvNAttr = QString("nm:TableNames=\"%1\"")
                              .arg(param2.toString());
        this->addRunTimeParaProvN(tabProvNAttr);
    }

    this->setInternalUpdateMode();
    QString updateProvNAttr = QString("nm:UpdateMode=\"%1\"")
                          .arg(mUpdateMode ? "true" : "false");
    this->addRunTimeParaProvN(updateProvNAttr);
    QString writeImgProvNAttr = QString("nm:WriteImage=\"%1\"")
                          .arg(mWriteImage ? "true" : "false");
    this->addRunTimeParaProvN(writeImgProvNAttr);
    QString writeTabProvNAttr = QString("nm:WriteTable=\"%1\"")
                          .arg(mWriteTable ? "true" : "false");
    this->addRunTimeParaProvN(writeTabProvNAttr);
    QString rgbModeProvNAttr = QString("nm:RGBMode=\"%1\"")
                          .arg(mRGBMode ? "true" : "false");
    this->addRunTimeParaProvN(rgbModeProvNAttr);


    this->setInternalResamplingType();
    QString resTypeProvNAttr = QString("nm:ResamplingType=\"%1\"")
                              .arg(mPyramidResamplingType);
    this->addRunTimeParaProvN(resTypeProvNAttr);

    this->setInternalStreamingMethod();
    QString streamMethodProvNAttr = QString("nm:StreamingMethod=\"%1\"")
                          .arg(mStreamingMethodType);
    this->addRunTimeParaProvN(streamMethodProvNAttr);

    this->setInternalStreamingSize();
    QString streamSizeProvNAttr = QString("nm:StreamingSize=\"%1\"")
                          .arg(mStreamingSize);
    this->addRunTimeParaProvN(streamSizeProvNAttr);

}

void
NMStreamingImageFileWriterWrapper
::setInternalUpdateMode()
{
    if (!this->mbIsInitialised)
        return;

    switch(this->mOutputComponentType)
    {
    MacroPerType( callSetUpdateMode, NMStreamingImageFileWriterWrapper_Internal )
    default:
        break;
    }
}

void
NMStreamingImageFileWriterWrapper
::setRAT(unsigned idx, QSharedPointer<NMItkDataObjectWrapper> imgWrapper)
{
    // we only set the RAT from the input component, if
    // - we're supposed to write a table at all
    // - we're not writing a table provided by a non-input component
    // - and the wrapper we've received is actually not null
    QVariant varInputTables = this->getParameter("InputTables");
    QString inputtable;
    if (varInputTables.isValid() && !varInputTables.isNull())
    {
        inputtable = varInputTables.toString();
    }

    if (    mWriteTable
        &&  inputtable.isEmpty()
        &&  !imgWrapper.isNull()
       )
    {
        otb::AttributeTable::Pointer tab = imgWrapper->getOTBTab();

        unsigned int tabIdx = 1;
        if (tab.IsNotNull())
        {
            switch(this->mOutputComponentType)
            {
            MacroPerType( callOutputTypeSetTab, NMStreamingImageFileWriterWrapper_Internal )
            default:
                break;
            }
        }
    }
}

void
NMStreamingImageFileWriterWrapper
::setInternalInputTable(const QString &tabelSpec,
                        const QMap<QString, NMModelComponent*>& repo)
{
    if (!this->mbIsInitialised)
        return;

    QStringList speclst = tabelSpec.split(":");
    QString compName = speclst.at(0);
    if (repo.contains(compName))
    {
        int idx = 0;
        if (speclst.size() == 2)
        {
            bool bok;
            idx = speclst.at(1).toInt(&bok);
        }
        NMModelComponent* comp = repo.value(compName);
        QSharedPointer<NMItkDataObjectWrapper> dw = comp->getOutput(idx);

        if (!dw.isNull())
        {
            otb::AttributeTable::Pointer tab = dw->getOTBTab();
                    //comp->getOutput(idx)->getOTBTab();

            unsigned int tabIdx = 1;
            if (tab.IsNotNull())
            {
                switch(this->mOutputComponentType)
                {
                MacroPerType( callOutputTypeSetTab, NMStreamingImageFileWriterWrapper_Internal )
                default:
                    break;
                }
            }
        }
    }
}

void
NMStreamingImageFileWriterWrapper
::setNthInput(unsigned int numInput,
        QSharedPointer<NMItkDataObjectWrapper> imgWrapper)
{
    if (!this->mbIsInitialised)
		return;

	itk::DataObject* img = imgWrapper->getDataObject();
	switch(this->mOutputComponentType)
	{
	MacroPerType( callFilterSetInput, NMStreamingImageFileWriterWrapper_Internal )
	default:
		break;
	}
}


QSharedPointer<NMItkDataObjectWrapper>
NMStreamingImageFileWriterWrapper::getOutput(unsigned int idx)
{
    QSharedPointer<NMItkDataObjectWrapper> dw;
    dw.clear();
    return dw;
}



#ifdef BUILD_RASSUPPORT
void NMStreamingImageFileWriterWrapper::setRasConnector(NMRasdamanConnectorWrapper* rw)
{
    if (rw != this->mRasConnector)
    {
        this->mRasConnector = rw;
        emit nmChanged();
    }
}

NMRasdamanConnectorWrapper* NMStreamingImageFileWriterWrapper::getRasConnector(void)
{
    return this->mRasConnector;
}
#endif


