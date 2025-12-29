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
*  itkNMConstShapedNeighborhoodIterator.cxx
*
*  Created on: 03/10/2025
*      Author: alex
*/


#include "itkNMConstShapedNeighborhoodIterator.h"
#include "nmitk_export.h"

template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::Image<unsigned long long, 1  >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<unsigned long long, 1        >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::Image<long long, 1           >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<long long, 1                 >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::Image<unsigned long, 1       >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<unsigned long, 1       >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::Image<long, 1                >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<long, 1                >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::Image<unsigned int, 1        >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<unsigned int, 1        >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::Image<int, 1                 >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<int, 1                 >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::Image<unsigned char, 1       >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<unsigned char, 1       >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::Image<char, 1                >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<char, 1                >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::Image<unsigned short, 1      >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<unsigned short, 1      >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::Image<short, 1               >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<short, 1               >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::Image<float, 1               >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<float, 1               >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::Image<double, 1              >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<double, 1              >>>;

template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::VectorImage<unsigned long long, 1  >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<unsigned long long, 1        >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::VectorImage<long long, 1           >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<long long, 1                 >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::VectorImage<unsigned long, 1       >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<unsigned long, 1       >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::VectorImage<long, 1                >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<long, 1                >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::VectorImage<unsigned int, 1  >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<unsigned int, 1  >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::VectorImage<int, 1           >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<int, 1           >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::VectorImage<unsigned char, 1 >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<unsigned char, 1 >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::VectorImage<char, 1          >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<char, 1          >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::VectorImage<unsigned short, 1>, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<unsigned short, 1>>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::VectorImage<short, 1         >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<short, 1         >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::VectorImage<float, 1         >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<float, 1         >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::VectorImage<double, 1        >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<double, 1        >>>;

template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<unsigned long long, 2  >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<unsigned long long, 2        >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<long long, 2           >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<long long, 2                 >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<unsigned long, 2       >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<unsigned long, 2       >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<long, 2                >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<long, 2                >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<unsigned int, 2        >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<unsigned int, 2        >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<int, 2                 >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<int, 2                 >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<unsigned char, 2       >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<unsigned char, 2       >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<char, 2                >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<char, 2                >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<unsigned short, 2      >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<unsigned short, 2      >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<short, 2               >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<short, 2               >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<float, 2               >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<float, 2               >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<double, 2              >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<double, 2              >>>;

template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::VectorImage<unsigned long long, 2  >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<unsigned long long, 2        >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::VectorImage<long long, 2           >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<long long, 2                 >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::VectorImage<unsigned long, 2       >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<unsigned long, 2       >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::VectorImage<long, 2                >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<long, 2                >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::VectorImage<unsigned int, 2  >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<unsigned int, 2  >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::VectorImage<int, 2           >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<int, 2           >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::VectorImage<unsigned char, 2 >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<unsigned char, 2 >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::VectorImage<char, 2          >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<char, 2          >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::VectorImage<unsigned short, 2>, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<unsigned short, 2>>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::VectorImage<short, 2         >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<short, 2         >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::VectorImage<float, 2         >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<float, 2         >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::VectorImage<double, 2        >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<double, 2        >>>;

template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<unsigned long long, 3  >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<unsigned long long, 3        >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<long long, 3           >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<long long, 3                 >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<unsigned long, 3       >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<unsigned long, 3       >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<long, 3                >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<long, 3                >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<unsigned int, 3        >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<unsigned int, 3        >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<int, 3                 >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<int, 3                 >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<unsigned char, 3       >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<unsigned char, 3       >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<char, 3                >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<char, 3                >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<unsigned short, 3      >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<unsigned short, 3      >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<short, 3               >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<short, 3               >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<float, 3               >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<float, 3               >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::Image<double, 3              >, itk::ZeroFluxNeumannBoundaryCondition<otb::Image<double, 3              >>>;

template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::VectorImage<unsigned long long, 3  >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<unsigned long long, 3        >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::VectorImage<long long, 3           >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<long long, 3                 >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::VectorImage<unsigned long, 3       >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<unsigned long, 3       >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator<otb::VectorImage<long, 3                >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<long, 3                >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::VectorImage<unsigned int, 3  >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<unsigned int, 3  >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::VectorImage<int, 3           >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<int, 3           >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::VectorImage<unsigned char, 3 >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<unsigned char, 3 >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::VectorImage<char, 3          >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<char, 3          >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::VectorImage<unsigned short, 3>, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<unsigned short, 3>>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::VectorImage<short, 3         >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<short, 3         >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::VectorImage<float, 3         >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<float, 3         >>>;
template class NMITK_EXPORT itk::NMConstShapedNeighborhoodIterator < otb::VectorImage<double, 3        >, itk::ZeroFluxNeumannBoundaryCondition<otb::VectorImage<double, 3        >>>;
