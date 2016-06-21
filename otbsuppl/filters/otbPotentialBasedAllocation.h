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

#ifndef __otbPotentialBasedAllocation_h
#define __otbPotentialBasedAllocation_h

#include <vector>

#include "nmlog.h"
//#include "itkImageToImageFilter.h"
#include "itkInPlaceImageFilter.h"
#include "itkImage.h"
#include "itkNumericTraits.h"

#include "otbsupplfilters_export.h"

namespace otb
{
template <class TInputImage, class TOutputImage>
class OTBSUPPLFILTERS_EXPORT PotentialBasedAllocation :
    public itk::InPlaceImageFilter< TInputImage, TOutputImage >
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
  typedef typename InputImageType::Pointer InputImagePointer;
  typedef typename OutputImageType::Pointer OutputImagePointer;

  /** Standard class typedefs. */
  typedef PotentialBasedAllocation                          Self;
  typedef itk::ImageToImageFilter< InputImageType, OutputImageType> Superclass;
  typedef itk::SmartPointer<Self>                                   Pointer;
  typedef itk::SmartPointer<const Self>                             ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(PotentialBasedAllocation, itk::ImageToImageFilter);
  
  /** Image typedef support. */
  typedef typename InputImageType::PixelType                    InputPixelType;
  typedef typename OutputImageType::PixelType                   OutputPixelType;
  typedef typename itk::NumericTraits<InputPixelType>::RealType InputRealType;
  
  typedef typename InputImageType::RegionType                   InputImageRegionType;
  typedef typename OutputImageType::RegionType                  OutputImageRegionType;
  typedef typename InputImageType::SizeType                     InputSizeType;

  /** Set / Get category identifiers; used for mapping categories
   *  and  for denoting which of the input
   *  maps had the max potential for anyone given pixel */
  void SetCategories(std::vector<OutputPixelType> cats)
  	  {this->m_Categories = cats;}
  std::vector<OutputPixelType> GetCategories(void)
	  {return this->m_Categories;}

  void SetThresholds(std::vector<InputPixelType> potentialThresholds)
  	  {this->m_Thresholds = potentialThresholds;}
  std::vector<InputPixelType> GetThresholds(void)
	  {return this->m_Thresholds;}


  
#ifdef ITK_USE_CONCEPT_CHECKING
  /** Begin concept checking */
  itkConceptMacro(InputHasNumericTraitsCheck,
                  (itk::Concept::HasNumericTraits<InputPixelType>));
  /** End concept checking */
#endif

protected:
  PotentialBasedAllocation();
  virtual ~PotentialBasedAllocation() {}
  void PrintSelf(std::ostream& os, itk::Indent indent) const;

  /** This filter can be implemented as a multithreaded filter.
   * Therefore, this implementation provides a ThreadedGenerateData()
   * routine which is called for each processing thread. The output
   * image data is allocated automatically by the superclass prior to
   * calling ThreadedGenerateData().  ThreadedGenerateData can only
   * write to the portion of the output image specified by the
   * parameter "outputRegionForThread"
   *
   * \sa ImageToImageFilter::ThreadedGenerateData(),
   *     ImageToImageFilter::GenerateData() */
  void ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
                            itk::ThreadIdType threadId );

  /** Prepare processing, make some consistency checks */
  void BeforeThreadedGenerateData(void);


private:
  PotentialBasedAllocation(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  std::vector<OutputPixelType> m_Categories;
  std::vector<InputPixelType> m_Thresholds;

};
  
} // end namespace itk

#include "otbPotentialBasedAllocation_ExplicitInst.h"

#ifndef ITK_MANUAL_INSTANTIATION
#include "otbPotentialBasedAllocation.txx"
#endif

//#endif

#endif
