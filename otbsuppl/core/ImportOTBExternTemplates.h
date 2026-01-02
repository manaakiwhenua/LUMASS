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


#ifndef ImportOTBExternTemplate_H
#define ImportOTBExternTemplate_H

#include "otbImage.h"
#include "otbVectorImage.h"
#include "itkImageToImageFilter.h"

// import explicitly instantiated template classes exported in OTBImageBase, OTBImageIO, 
// to avoid re-definition errors when compiling lumass with msvc 17+


//extern template class __declspec(dllimport) itk::ImageBase<1>;
//extern template class __declspec(dllimport) itk::ImageBase<2>;
//extern template class __declspec(dllimport) itk::ImageBase<3>;

extern template class __declspec(dllimport) otb::Image<unsigned int, 2 > ;
extern template class __declspec(dllimport) otb::Image<int, 2 > ;
extern template class __declspec(dllimport) otb::Image<unsigned char, 2 > ;
extern template class __declspec(dllimport) otb::Image<char, 2 > ;
extern template class __declspec(dllimport) otb::Image<unsigned short, 2 > ;
extern template class __declspec(dllimport) otb::Image<short, 2 > ;
extern template class __declspec(dllimport) otb::Image<float, 2 > ;
extern template class __declspec(dllimport) otb::Image<double, 2 > ;
extern template class __declspec(dllimport) otb::VectorImage<unsigned int, 2 > ;
extern template class __declspec(dllimport) otb::VectorImage<int, 2 > ;
extern template class __declspec(dllimport) otb::VectorImage<unsigned char, 2 > ;
extern template class __declspec(dllimport) otb::VectorImage<char, 2 > ;
extern template class __declspec(dllimport) otb::VectorImage<unsigned short, 2 > ;
extern template class __declspec(dllimport) otb::VectorImage<short, 2 > ;
extern template class __declspec(dllimport) otb::VectorImage<float, 2 > ;
extern template class __declspec(dllimport) otb::VectorImage<double, 2 > ;

extern template class __declspec(dllimport) itk::ImageSource<otb::Image<unsigned int, 2 >>;
extern template class __declspec(dllimport) itk::ImageSource<otb::Image<int, 2 >>;
extern template class __declspec(dllimport) itk::ImageSource<otb::Image<unsigned char, 2 >>;
extern template class __declspec(dllimport) itk::ImageSource<otb::Image<char, 2 >>;
extern template class __declspec(dllimport) itk::ImageSource<otb::Image<unsigned short, 2 >>;
extern template class __declspec(dllimport) itk::ImageSource<otb::Image<short, 2 >>;
extern template class __declspec(dllimport) itk::ImageSource<otb::Image<float, 2 >>;
extern template class __declspec(dllimport) itk::ImageSource<otb::Image<double, 2 >>;

extern template class __declspec(dllimport) itk::ImageSource<otb::VectorImage<unsigned int, 2 >>;
extern template class __declspec(dllimport) itk::ImageSource<otb::VectorImage<int, 2 >>;
extern template class __declspec(dllimport) itk::ImageSource<otb::VectorImage<unsigned char, 2 >>;
extern template class __declspec(dllimport) itk::ImageSource<otb::VectorImage<char, 2 >>;
extern template class __declspec(dllimport) itk::ImageSource<otb::VectorImage<unsigned short, 2 >>;
extern template class __declspec(dllimport) itk::ImageSource<otb::VectorImage<short, 2 >>;
extern template class __declspec(dllimport) itk::ImageSource<otb::VectorImage<float, 2 >>;
extern template class __declspec(dllimport) itk::ImageSource<otb::VectorImage<double, 2 >>;

//extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<unsigned int, 2 >  , otb::Image<unsigned int, 2 >   >;
//extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<int, 2 >, otb::Image<int, 2 >            >;
//extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<unsigned char, 2 >, otb::Image<unsigned char, 2 >  >;
//extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<char, 2 >, otb::Image<char, 2 >           >;
//extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<unsigned short, 2 >, otb::Image<unsigned short, 2 > >;
//extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<short, 2 >, otb::Image<short, 2 >          >;
//extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<float, 2 >, otb::Image<float, 2 >          >;
//extern template class __declspec(dllimport) itk::ImageToImageFilter<otb::Image<double, 2 >, otb::Image<double, 2 >         >;


//extern template class __declspec(dllimport) otb::ImageFileReader<otb::Image<unsigned int, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileReader<otb::Image<int, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileReader<otb::Image<unsigned char, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileReader<otb::Image<char, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileReader<otb::Image<unsigned short, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileReader<otb::Image<short, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileReader<otb::Image<float, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileReader<otb::Image<double, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileReader<otb::VectorImage<unsigned int, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileReader<otb::VectorImage<int, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileReader<otb::VectorImage<unsigned char, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileReader<otb::VectorImage<char, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileReader<otb::VectorImage<unsigned short, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileReader<otb::VectorImage<short, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileReader<otb::VectorImage<float, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileReader<otb::VectorImage<double, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileReader<otb::VectorImage<std::complex<int>, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileReader<otb::VectorImage<std::complex<short>, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileReader<otb::VectorImage<std::complex<float>, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileReader<otb::VectorImage<std::complex<double>, 2>>;
//
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::Image<unsigned int, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::Image<int, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::Image<unsigned char, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::Image<char, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::Image<unsigned short, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::Image<short, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::Image<float, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::Image<double, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::Image<std::complex<int>, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::Image<std::complex<short>, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::Image<std::complex<float>, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::Image<std::complex<double>, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::VectorImage<unsigned int, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::VectorImage<int, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::VectorImage<unsigned char, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::VectorImage<char, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::VectorImage<unsigned short, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::VectorImage<short, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::VectorImage<float, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::VectorImage<double, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::VectorImage<std::complex<int>, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::VectorImage<std::complex<short>, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::VectorImage<std::complex<float>, 2>>;
//extern template class __declspec(dllimport) otb::ImageFileWriter<otb::VectorImage<std::complex<double>, 2>>;

#endif  /* ImportOTBExternTemplate_H */
