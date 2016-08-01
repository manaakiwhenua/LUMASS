 /****************************************************************************** 
 * Created by Alexander Herzig 
 * Copyright 2013 Landcare Research New Zealand Ltd
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
 * SortFilter.h
 *
 *  Created on: 04/02/2013
 *      Author: alex
 */

#ifndef SortFILTER_H_
#define SortFILTER_H_

#define ctxSortFilter "SortFilter"

#include "nmlog.h"
#include <string>
#include "itkImageToImageFilter.h"
#include "itkImageRegionIterator.h"
#include "itkMultiThreader.h"

#include "otbsupplfilters_export.h"

/**  Sorts an image and any additionally specified 'depending' image accordingly.
 *
 *	This class sorts input image #0 either ascending or descending. Any additionally
 *	specified image is sorted according to the order of input image #0. Furthermore
 *	this filter produces an 'IndexImage' which denotes the original 1D-index (offset)
 *	position of input image #0. The IndexImage is available via \c GetIndexImage
 *  or via \c GetOutput(n) whereas n denotes the total number of input images.
 *	The filter is implemented as being partly multi-threaded, i.e. the filter
 *	splits the image in chunks according to the number of threads and pre-sorts those
 *	in parallel using quick sort. The pre-sorted chunks are then merged to a
 *	single image using just one thread.
 */

namespace otb {

template <class TInputImage, class TOutputImage = TInputImage>
class OTBSUPPLFILTERS_EXPORT SortFilter
			: public itk::ImageToImageFilter<TInputImage, TOutputImage>
{
public:

  typedef SortFilter							Self;
  typedef itk::ImageToImageFilter<TInputImage, TOutputImage>		Superclass;
  typedef itk::SmartPointer<Self>								Pointer;
  typedef itk::SmartPointer<const Self>							ConstPointer;

  itkNewMacro(Self)
  itkTypeMacro(SortFilter, itk::ImageToImageFilter)

  typedef TInputImage						InputImageType;
  typedef typename InputImageType::Pointer	InputImagePointer;
  typedef typename InputImageType::RegionType InputImageRegionType;
  typedef typename InputImageType::PixelType  InputImagePixelType;
  typedef typename itk::ImageRegionIteratorWithIndex<InputImageType> InputIteratorType;

  typedef TOutputImage						OutputImageType;
  typedef typename OutputImageType::Pointer	OutputImagePointer;
  typedef typename OutputImageType::RegionType OutputImageRegionType;
  typedef typename OutputImageType::PixelType  OutputImagePixelType;


  typedef typename otb::Image<float, OutputImageType::ImageDimension> IndexImageType;
  typedef typename IndexImageType::Pointer                           IndexImagePointer;
  typedef typename IndexImageType::RegionType						 IndexImageRegionType;
  typedef typename IndexImageType::PixelType                         IndexImagePixelType;
  typedef typename itk::ImageRegionIterator<IndexImageType> IndexIteratorType;


  itkSetMacro(SortAscending, bool)
  itkGetMacro(SortAscending, bool)
  itkBooleanMacro(SortAscending)

  IndexImageType* GetIndexImage(void)
  {
//	  return dynamic_cast<IndexImageType*>(
//			  const_cast<itk::DataObject*>(
//					  itk::ProcessObject::GetOutput(
//							  this->GetNumberOfInputs())));

      return dynamic_cast<IndexImageType*>(
                  const_cast<itk::DataObject*>(
                      itk::ProcessObject::GetOutput("IndexImage")));
  }


protected:
	SortFilter();
	virtual ~SortFilter();
	void PrintSelf(std::ostream& os, itk::Indent indent) const;

	struct ThreadStruct
	{
		Pointer Filter;
	};

	static ITK_THREAD_RETURN_TYPE CalledFromThreader(void* arg);
	virtual void AllocateOutputs(void);
	long SplitArray(long threadId, long numSplits, long* offset);

	void GenerateData(void);

	void ProcessThreaded(long threadId, long offset, long length);
	void BeforeThreadedGenerateData(void);
	void AfterThreadedGenerateData(void);

	inline void SortRegionDescending(std::vector<InputImagePixelType*> inArrs,
			IndexImagePixelType* idxBuf, long left, long right);
	inline void SortRegionAscending(std::vector<InputImagePixelType*> inArrs,
			IndexImagePixelType* idxBuf, long left, long right);
private:

	bool m_SortAscending;
	std::vector<long> mArrayOffsets;
	std::vector<long> mChunkSize;

};

} // end namespace

//#include "otbSortFilter_ExplicitInst.h"

#ifndef ITK_MANUAL_INSTANTIATION
#include "otbSortFilter.txx"
#endif

#endif /* SortFILTER_H_ */


