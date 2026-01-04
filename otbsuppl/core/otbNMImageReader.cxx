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

/******************************************************************************
* Created by Alexander Herzig
* Copyright 2025 New Zealand Institute for Bioeconomy Science Ltd.
*
* This file is part of 'LUMASS', which is free software: you can redistribute
* it and/or modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation, either version 3 of the License,
* or (at your option) any later version.
*
* This programs distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
/*
*  otbNMImageReader.cxx
*
*  Created on: 03/10/2025
*      Author: alex
*/

#include "otbNMImageReader.h"
#include "nmotbsupplcorereader_export.h"

// explicit instantiation

template class NMOTBSUPPLCOREREADER_EXPORT itk::ImageBase<1>;
template class NMOTBSUPPLCOREREADER_EXPORT itk::ImageBase<2>;
template class NMOTBSUPPLCOREREADER_EXPORT itk::ImageBase<3>;

template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<unsigned int, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<int, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<unsigned char, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<char, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<unsigned short, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<short, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<float, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<double, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<unsigned long, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<long, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<unsigned long long, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<long long, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<unsigned int, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<int, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<unsigned char, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<char, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<unsigned short, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<short, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<float, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<double, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<unsigned long, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<long, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<unsigned long long, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<long long, 1>;

template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<unsigned int, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<int, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<unsigned char, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<char, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<unsigned short, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<short, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<float, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<double, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<unsigned long, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<long, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<unsigned long long, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<long long, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<unsigned int, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<int, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<unsigned char, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<char, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<unsigned short, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<short, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<float, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<double, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<unsigned long, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<long, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<unsigned long long, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<long long, 2>;

template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<unsigned int, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<int, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<unsigned char, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<char, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<unsigned short, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<short, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<float, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<double, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<unsigned long, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<long, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<unsigned long long, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<long long, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<unsigned int, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<int, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<unsigned char, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<char, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<unsigned short, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<short, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<float, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<double, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<unsigned long, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<long, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<unsigned long long, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::VectorImage<long long, 3>;

template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<unsigned int>, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<int>, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<unsigned char>, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<char>, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<unsigned short>, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<short>, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<float>, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<double>, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<unsigned long>, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<long>, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<unsigned long long>, 1>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<long long>, 1>;

template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<unsigned int>, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<int>, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<unsigned char>, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<char>, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<unsigned short>, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<short>, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<float>, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<double>, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<unsigned long>, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<long>, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<unsigned long long>, 2>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<long long>, 2>;

template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<unsigned int>, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<int>, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<unsigned char>, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<char>, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<unsigned short>, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<short>, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<float>, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<double>, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<unsigned long>, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<long>, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<unsigned long long>, 3>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::Image<itk::RGBPixel<long long>, 3>;

template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<unsigned long long, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<long long, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<unsigned long, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<long, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<unsigned int, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<int, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<unsigned char, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<char, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<unsigned short, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<short, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<float, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<double, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<unsigned long long, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<long long, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<unsigned long, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<long, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<unsigned int, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<int, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<unsigned char, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<char, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<unsigned short, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<short, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<float, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<double, 1>>;

template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<unsigned long long, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<long long, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<unsigned long, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<long, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<unsigned int, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<int, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<unsigned char, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<char, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<unsigned short, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<short, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<float, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<double, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<unsigned long long, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<long long, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<unsigned long, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<long, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<unsigned int, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<int, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<unsigned char, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<char, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<unsigned short, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<short, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<float, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<double, 2>>;

template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<unsigned long long, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<long long, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<unsigned long, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<long, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<unsigned int, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<int, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<unsigned char, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<char, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<unsigned short, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<short, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<float, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<double, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<unsigned long long, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<long long, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<unsigned long, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<long, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<unsigned int, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<int, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<unsigned char, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<char, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<unsigned short, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<short, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<float, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::VectorImage<double, 3>>;

template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<unsigned int>, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<int>, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<unsigned char>, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<char>, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<unsigned short>, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<short>, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<float>, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<double>, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<unsigned long>, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<long>, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<unsigned long long>, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<long long>, 1>>;

template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<unsigned int>, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<int>, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<unsigned char>, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<char>, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<unsigned short>, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<short>, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<float>, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<double>, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<unsigned long>, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<long>, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<unsigned long long>, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<long long>, 2>>;

template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<unsigned int>, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<int>, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<unsigned char>, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<char>, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<unsigned short>, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<short>, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<float>, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<double>, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<unsigned long>, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<long>, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<unsigned long long>, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::ImageFileReader<otb::Image<itk::RGBPixel<long long>, 3>>;



template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<unsigned int, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<int, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<unsigned char, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<char, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<unsigned short, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<short, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<float, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<double, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<unsigned long, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<long, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<unsigned long long, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<long long, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<unsigned int, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<int, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<unsigned char, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<char, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<unsigned short, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<short, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<float, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<double, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<unsigned long, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<long, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<unsigned long long, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<long long, 1>>;

template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<unsigned int, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<int, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<unsigned char, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<char, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<unsigned short, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<short, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<float, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<double, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<unsigned long, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<long, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<unsigned long long, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<long long, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<unsigned int, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<int, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<unsigned char, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<char, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<unsigned short, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<short, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<float, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<double, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<unsigned long, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<long, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<unsigned long long, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<long long, 2>>;

template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<unsigned int, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<int, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<unsigned char, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<char, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<unsigned short, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<short, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<float, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<double, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<unsigned long, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<long, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<unsigned long long, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<long long, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<unsigned int, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<int, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<unsigned char, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<char, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<unsigned short, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<short, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<float, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<double, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<unsigned long, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<long, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<unsigned long long, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::VectorImage<long long, 3>>;

template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<unsigned int>, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<int>, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<unsigned char>, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<char>, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<unsigned short>, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<short>, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<float>, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<double>, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<unsigned long>, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<long>, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<unsigned long long>, 1>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<long long>, 1>>;

template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<unsigned int>, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<int>, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<unsigned char>, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<char>, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<unsigned short>, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<short>, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<float>, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<double>, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<unsigned long>, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<long>, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<unsigned long long>, 2>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<long long>, 2>>;

template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<unsigned int>, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<int>, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<unsigned char>, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<char>, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<unsigned short>, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<short>, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<float>, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<double>, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<unsigned long>, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<long>, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<unsigned long long>, 3>>;
template class NMOTBSUPPLCOREREADER_EXPORT otb::NMImageReader<otb::Image<itk::RGBPixel<long long>, 3>>;


