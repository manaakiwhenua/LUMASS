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
/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkMeanImageFilter.h,v $
  Language:  C++
  Date:      $Date: 2008-10-16 18:05:25 $
  Version:   $Revision: 1.7 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#ifndef __otbProcessLUPotentials_h
#define __otbProcessLUPotentials_h

#include <vector>

#include "nmlog.h"
//#include "otbPersistentImageFilter.h"
#include "itkImageToImageFilter.h"
#include "itkImage.h"
//#include "itkImageToImageFilter.h"
//#include "itkImage.h"
#include "itkNumericTraits.h"

#include "otbsupplfilters_export.h"

namespace otb
{
template <class TInputImage, class TOutputImage>
class OTBSUPPLFILTERS_EXPORT ProcessLUPotentials :
    public itk::ImageToImageFilter< TInputImage, TOutputImage >
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
  typedef ProcessLUPotentials                          Self;
  typedef itk::ImageToImageFilter< InputImageType, OutputImageType> Superclass;
  typedef itk::SmartPointer<Self>                                   Pointer;
  typedef itk::SmartPointer<const Self>                             ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(ProcessLUPotentials, itk::ImageToImageFilter);
  
  /** Image typedef support. */
  typedef typename InputImageType::PixelType                    InputPixelType;
  typedef typename OutputImageType::PixelType                   OutputPixelType;
  typedef typename itk::NumericTraits<InputPixelType>::RealType InputRealType;
  
  typedef typename InputImageType::RegionType                   InputImageRegionType;
  typedef typename OutputImageType::RegionType                  OutputImageRegionType;
  typedef typename InputImageType::SizeType                     InputSizeType;

  /** Set / Get category identifiers for denoting which of the input
   *  maps had the max potential for anyone given pixel */
  void SetCategories(std::vector<OutputPixelType> cats)
  	  {this->m_Categories = cats;}
  std::vector<OutputPixelType> GetCategories(void)
	  {return this->m_Categories;}

  
  virtual itk::DataObject::Pointer MakeOutput(unsigned int idx);

  /** Get the max potentials map (it's of InputImageType,
   *  so we have to grab it separately */
  InputImageType* GetMaxPotentialMap(void)
  	  {return dynamic_cast<InputImageType*>(
  			  const_cast<itk::DataObject*>(
  					  itk::ProcessObject::GetOutput(1)));}


#ifdef ITK_USE_CONCEPT_CHECKING
  /** Begin concept checking */
  itkConceptMacro(InputHasNumericTraitsCheck,
                  (itk::Concept::HasNumericTraits<InputPixelType>));
  /** End concept checking */
#endif

protected:
  ProcessLUPotentials();
  virtual ~ProcessLUPotentials() {}
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
  ProcessLUPotentials(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  std::vector<OutputPixelType> m_Categories;
  std::vector<InputImagePointer> m_Inputs;

  unsigned int m_MaskIdx;


};
  
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "otbProcessLUPotentials.txx"
#endif

//#endif

#endif
