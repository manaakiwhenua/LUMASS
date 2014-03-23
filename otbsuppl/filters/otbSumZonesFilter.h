/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2010-2014 Landcare Research New Zealand Ltd
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

#ifndef OTBSUMZONESFILTER_H_
#define OTBSUMZONESFILTER_H_

#include "otbAttributeTable.h"
#include "itkImageToImageFilter.h"
#include "otbImage.h"

namespace otb
{
/** \class SumZonesFilter
 *  \brief Summarises zones
 *
 *  This filter requires at least two inputs:
 *  	1. a zone image defining the zones for which to compute the
 *  	   summary (type = TOutputImage);
 *  	2. an image containing the values to be summarised (type = TInputImage)
 *  	3. optionally an otb::AttributeTable, which contains as many rows
 *  	   as zones in the zone image - ideally, it's the attribute table of
 *  	   that particular layer
 *
 */

template< class TInputImage, class TOutputImage >
class ITK_EXPORT SumZonesFilter
	: public itk::ImageToImageFilter< TInputImage, TOutputImage >
{
public:
	  /** Extract dimension from input and output image. */
	  itkStaticConstMacro(InputImageDimension, unsigned int,
	                      TInputImage::ImageDimension);
	  itkStaticConstMacro(OutputImageDimension, unsigned int,
	                      TOutputImage::ImageDimension);

	  /** Convenient typedefs for simplifying declarations. */
	  typedef TInputImage  InputImageType;
	  typedef TOutputImage OutputImageType;

	  typedef typename InputImageType::Pointer	InputImagePointerType;
	  typedef typename OutputImageType::Pointer OutputImagePointerType;

	  /** Standard class typedefs. */
	  typedef SumZonesFilter                                            Self;
	  typedef itk::ImageToImageFilter< InputImageType, OutputImageType> Superclass;
	  typedef itk::SmartPointer<Self>                                   Pointer;
	  typedef itk::SmartPointer<const Self>                             ConstPointer;

	  /** Method for creation through the object factory. */
	  itkNewMacro(Self);

	  /** Run-time type information (and related methods). */
	  itkTypeMacro(SumZonesFilter, itk::ImageToImageFilter);

	  /** Image typedef support. */
	  typedef typename InputImageType::PixelType   InputImagePixelType;
	  typedef typename OutputImageType::PixelType  OutputImagePixelType;

	  typedef typename InputImageType::RegionType  InputImageRegionType;
	  typedef typename OutputImageType::RegionType OutputImageRegionType;

	  typedef typename InputImageType::SizeType    InputImageSizeType;
	  typedef typename OutputImageType::SizeType   OutputImageSizeType;

	  typedef typename InputImageType::SpacingType  InputImageSpacingType;
	  typedef typename OutputImageType::SpacingType OutputImageSpacingType;

	  /** Set the input images */
	  void SetZoneImage(const InputImageType* image);
	  void SetValueImage(const OutputImageType* image);

	  /** Specify the table to store the zone values */
	  void SetZoneTable(AttributeTable::Pointer);

	  AttributeTable::Pointer GetZoneTable(void) {return mZoneTable;}

protected:
	  SumZonesFilter();
	  virtual ~SumZonesFilter();
	  virtual void PrintSelf(std::ostream& os, itk::Indent indent) const;

	  void BeforeThreadedGenerateData();
	  void ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread, int threadId );
	  void AfterThreadedGenerateData();


private:
	  SumZonesFilter(const Self&); //purposely not implemented
	  void operator=(const Self&); //purposely not implemented

	  AttributeTable::Pointer mZoneTable;
	  // this image defines the zones for which to do the summary
	  InputImagePointerType mZoneImage;
	  // this image contains the values to be summarised for the individual zones
	  OutputImagePointerType mValueImage;
	  // this image is produced on request and contains ONE selected statistic
	  // for the given zones
	  OutputImagePointerType mOutputImage;

	  std::unordered_map< InputPixelType, std::vector< double> >* mThreadValueStore;

};

} // end namespace otb

#ifndef OTB_MANUAL_INSTANTIATION
#include "otbSumZonesFilter.txx"
#endif

#endif /* OTBSUMZONESFILTER_H_ */
