/******************************************************************************
* Created by Alexander Herzig
* Copyright 2025-2026 New Zealand Institute for Bioeconomy Science Limited
*
* This file is part of 'LUMASS', which is free software: you can redistribute
* it and/or modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation, either version 3 of the License,
* or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
********************************************************************************/

#pragma once


#ifndef ImportOTBSupplCoreExternTemplate_H
#define ImportOTBSupplCoreExternTemplate_H

#include "itkImageBase.h"
#include "otbImage.h"
#include "otbVectorImage.h"
#include "itkImageToImageFilter.h"
#include "itkImageConstIterator.h"
#include "itkImageConstIteratorWithIndex.h"
#include "itkImageRegionConstIterator.h"
#include "otbImageFileReader.h"

extern template class __declspec(dllimport) otb::Image<unsigned long, 1 >;
extern template class __declspec(dllimport) otb::Image<long, 1 >;
extern template class __declspec(dllimport) otb::Image<unsigned long long, 1 >;
extern template class __declspec(dllimport) otb::Image<long long, 1 >;
extern template class __declspec(dllimport) otb::Image<unsigned int, 1 > ;
extern template class __declspec(dllimport) otb::Image<int, 1 > ;
extern template class __declspec(dllimport) otb::Image<unsigned char, 1 > ;
extern template class __declspec(dllimport) otb::Image<char, 1 > ;
extern template class __declspec(dllimport) otb::Image<unsigned short, 1 > ;
extern template class __declspec(dllimport) otb::Image<short, 1 > ;
extern template class __declspec(dllimport) otb::Image<float, 1 > ;
extern template class __declspec(dllimport) otb::Image<double, 1 > ;

extern template class __declspec(dllimport) otb::Image<unsigned long, 2 >;
extern template class __declspec(dllimport) otb::Image<long, 2 >;
extern template class __declspec(dllimport) otb::Image<unsigned long long, 2 >;
extern template class __declspec(dllimport) otb::Image<long long, 2 >;
extern template class __declspec(dllimport) otb::Image<unsigned int, 2 >;
extern template class __declspec(dllimport) otb::Image<int, 2 >;
extern template class __declspec(dllimport) otb::Image<unsigned char, 2 >;
extern template class __declspec(dllimport) otb::Image<char, 2 >;
extern template class __declspec(dllimport) otb::Image<unsigned short, 2 >;
extern template class __declspec(dllimport) otb::Image<short, 2 >;
extern template class __declspec(dllimport) otb::Image<float, 2 >;
extern template class __declspec(dllimport) otb::Image<double, 2 >;

extern template class __declspec(dllimport) otb::VectorImage<unsigned long, 2 >;
extern template class __declspec(dllimport) otb::VectorImage<long, 2 >;
extern template class __declspec(dllimport) otb::VectorImage<unsigned long long, 2 >;
extern template class __declspec(dllimport) otb::VectorImage<long long, 2 >;
extern template class __declspec(dllimport) otb::VectorImage<unsigned int, 2 >;
extern template class __declspec(dllimport) otb::VectorImage<int, 2 >;
extern template class __declspec(dllimport) otb::VectorImage<unsigned char, 2 >;
extern template class __declspec(dllimport) otb::VectorImage<char, 2 >;
extern template class __declspec(dllimport) otb::VectorImage<unsigned short, 2 >;
extern template class __declspec(dllimport) otb::VectorImage<short, 2 >;
extern template class __declspec(dllimport) otb::VectorImage<float, 2 >;
extern template class __declspec(dllimport) otb::VectorImage<double, 2 >;

extern template class __declspec(dllimport) otb::Image<unsigned long, 3 >;
extern template class __declspec(dllimport) otb::Image<long, 3 >;
extern template class __declspec(dllimport) otb::Image<unsigned long long, 3 >;
extern template class __declspec(dllimport) otb::Image<long long, 3 >;
extern template class __declspec(dllimport) otb::Image<unsigned int, 3 >;
extern template class __declspec(dllimport) otb::Image<int, 3 >;
extern template class __declspec(dllimport) otb::Image<unsigned char, 3 >;
extern template class __declspec(dllimport) otb::Image<char, 3 >;
extern template class __declspec(dllimport) otb::Image<unsigned short, 3 >;
extern template class __declspec(dllimport) otb::Image<short, 3 >;
extern template class __declspec(dllimport) otb::Image<float, 3 >;
extern template class __declspec(dllimport) otb::Image<double, 3 >;

extern template class __declspec(dllimport) otb::VectorImage<unsigned long, 3 >;
extern template class __declspec(dllimport) otb::VectorImage<long, 3 >;
extern template class __declspec(dllimport) otb::VectorImage<unsigned long long, 3 >;
extern template class __declspec(dllimport) otb::VectorImage<long long, 3 >;
extern template class __declspec(dllimport) otb::VectorImage<unsigned int, 3 >;
extern template class __declspec(dllimport) otb::VectorImage<int, 3 >;
extern template class __declspec(dllimport) otb::VectorImage<unsigned char, 3 >;
extern template class __declspec(dllimport) otb::VectorImage<char, 3 >;
extern template class __declspec(dllimport) otb::VectorImage<unsigned short, 3 >;
extern template class __declspec(dllimport) otb::VectorImage<short, 3 >;
extern template class __declspec(dllimport) otb::VectorImage<float, 3 >;
extern template class __declspec(dllimport) otb::VectorImage<double, 3 >;

extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<unsigned int>, 1>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<int>, 1>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<unsigned char>, 1>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<char>, 1>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<unsigned short>, 1>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<short>, 1>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<float>, 1>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<double>, 1>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<unsigned long>, 1>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<long>, 1>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<unsigned long long>, 1>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<long long>, 1>;

extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<unsigned int>, 2>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<int>, 2>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<unsigned char>, 2>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<char>, 2>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<unsigned short>, 2>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<short>, 2>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<float>, 2>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<double>, 2>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<unsigned long>, 2>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<long>, 2>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<unsigned long long>, 2>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<long long>, 2>;


extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<unsigned int>, 3>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<int>, 3>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<unsigned char>, 3>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<char>, 3>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<unsigned short>, 3>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<short>, 3>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<float>, 3>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<double>, 3>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<unsigned long>, 3>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<long>, 3>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<unsigned long long>, 3>;
extern template class __declspec(dllimport) otb::Image<itk::RGBPixel<long long>, 3>;

extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<unsigned long long, 1 >, otb::Image<unsigned long long, 1 >   >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<long long, 1 >, otb::Image<long long, 1 >            >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<unsigned long, 1 >, otb::Image<unsigned long, 1 >  >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<long, 1 >, otb::Image<long, 1 >           >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<unsigned int, 1 >, otb::Image<unsigned int, 1 >   >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<int, 1 >, otb::Image<int, 1 >            >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<unsigned char, 1 >, otb::Image<unsigned char, 1 >  >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<char, 1 >, otb::Image<char, 1 >           >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<unsigned short, 1 >, otb::Image<unsigned short, 1 > >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<short, 1 >, otb::Image<short, 1 >          >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<float, 1 >, otb::Image<float, 1 >          >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<double, 1 >, otb::Image<double, 1 >         >;

extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<unsigned long long, 2 >, otb::Image<unsigned long long, 2 >   >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<long long, 2 >, otb::Image<long long, 2 >            >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<unsigned long, 2 >, otb::Image<unsigned long, 2 >  >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<long, 2 >, otb::Image<long, 2 >           >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<unsigned int, 2 >  , otb::Image<unsigned int, 2 >   >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<int, 2 >, otb::Image<int, 2 >            >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<unsigned char, 2 >, otb::Image<unsigned char, 2 >  >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<char, 2 >, otb::Image<char, 2 >           >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<unsigned short, 2 >, otb::Image<unsigned short, 2 > >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<short, 2 >, otb::Image<short, 2 >          >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<float, 2 >, otb::Image<float, 2 >          >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<double, 2 >, otb::Image<double, 2 >         >;

extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<unsigned long long, 3 >, otb::Image<unsigned long long, 3 >   >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<long long, 3 >, otb::Image<long long, 3 >            >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<unsigned long, 3 >, otb::Image<unsigned long, 3 >  >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<long, 3 >, otb::Image<long, 3 >           >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<unsigned int, 3 >, otb::Image<unsigned int, 3 >   >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<int, 3 >, otb::Image<int, 3 >            >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<unsigned char, 3 >, otb::Image<unsigned char, 3 >  >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<char, 3 >, otb::Image<char, 3 >           >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<unsigned short, 3 >, otb::Image<unsigned short, 3 > >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<short, 3 >, otb::Image<short, 3 >          >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<float, 3 >, otb::Image<float, 3 >          >;
extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<double, 3 >, otb::Image<double, 3 >         >;

#endif  /* ImportOTBSupplCoreExternTemplate_H */
