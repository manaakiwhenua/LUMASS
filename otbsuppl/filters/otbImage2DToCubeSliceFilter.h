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
 * otbImage2DToCubeSliceFilter.h
 *
 * Created on: 27/05/2012
 *     Author: alex
 *
 */

#ifndef IMAGE2DTOCUBESLICEFILTER_H
#define IMAGE2DTOCUBESLICEFILTER_H

#include <string>
#include <iostream>
#include "itkImageToImageFilter.h"
#include "nmlog.h"
#define ctxImage2DToCubeSliceFilter "Image2DToCubeSliceFilter"

#include "otbsupplfilters_export.h"

namespace otb {

template <class TInputImage, class TOutputImage>
class OTBSUPPLFILTERS_EXPORT Image2DToCubeSliceFilter
	: public itk::ImageToImageFilter<TInputImage, TOutputImage>
{
public:

  typedef Image2DToCubeSliceFilter							Self;
  typedef itk::ImageToImageFilter<TInputImage, TOutputImage>		Superclass;
  typedef itk::SmartPointer<Self>								Pointer;
  typedef itk::SmartPointer<const Self>							ConstPointer;

  itkStaticConstMacro(InputImageDimension, unsigned int, TInputImage::ImageDimension);
  itkStaticConstMacro(OutputImageDimension, unsigned int, TOutputImage::ImageDimension);

  itkNewMacro(Self);
  itkTypeMacro(Image2DToCubeSliceFilter, itk::ImageToImageFilter);

  typedef TInputImage						InputImageType;
  typedef typename InputImageType::Pointer	InputImagePointer;
  typedef typename InputImageType::RegionType InputRegionType;
  typedef typename InputImageType::PixelType  InputImagePixelType;
  typedef typename InputImageType::SizeType InputSizeType;
  typedef typename InputImageType::IndexType InputIndexType;
  typedef typename InputImageType::SpacingType InputSpacingType;
  typedef typename InputImageType::PointType InputPointType;


  typedef TOutputImage						OutputImageType;
  typedef typename OutputImageType::Pointer	OutputImagePointer;
  typedef typename OutputImageType::RegionType OutputRegionType;
  typedef typename OutputImageType::PixelType  OutputImagePixelType;
  typedef typename OutputImageType::SizeType OutputSizeType;
  typedef typename OutputImageType::IndexType OutputIndexType;
  typedef typename OutputImageType::SpacingType OutputSpacingType;
  typedef typename OutputImageType::PointType OutputPointType;

  /** Typedef to describe the output and input image region types. */
  typedef typename TOutputImage::RegionType OutputImageRegionType;
  typedef typename TInputImage::RegionType  InputImageRegionType;


//  typedef enum{ZSLICE, YSLICE, XSLICE} TargetSliceDimension;

//  void SetTargetSliceDimension(TargetSliceDimension tsd)
//  	  {this->mTargetDimension = tsd;}
//  TargetSliceDimension GetTargetSliceDimension(void)
//  	  {return this->mTargetDimension;}

  itkSetMacro(ZLevel, unsigned int)
  itkGetMacro(ZLevel, unsigned int)

//  void SetZLevel(unsigned int level)
//  {
//	  if (level > 0)
//	  {
//		  this->m_ZLevel = level;
//		  if (this->m_FirstZLevel == -1)
//			  this->m_FirstZLevel = level;
//		  this->Modified();
//	  }
//  }

  itkSetMacro(ZSpacing, double);
  itkGetMacro(ZSpacing, double);

  itkSetMacro(ZOrigin, double);
  itkGetMacro(ZOrigin, double);

  void GenerateOutputInformation(void);
//  void GenerateInputRequestedRegion(void);
  void EnlargeOutputRequestedRegion(itk::DataObject* output);

protected:
  Image2DToCubeSliceFilter();
  ~Image2DToCubeSliceFilter();

  void
  ThreadedGenerateData(
		  const OutputImageRegionType& outputRegionForThread,
          itk::ThreadIdType threadId );

  unsigned int m_ZLevel;
  double m_ZSpacing;
  double m_ZOrigin;

//  unsigned int m_FirstZLevel;

//  TargetSliceDimension mTargetDimension;

}; // end of class definition

} // end of namespace otb

#ifndef ITK_MANUAL_INSTANTIATION
#include "otbImage2DToCubeSliceFilter.txx"
#endif

#endif // ifndef header
