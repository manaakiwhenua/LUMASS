 /****************************************************************************** 
 * Created by Alexander Herzig 
 * Copyright 2016 Landcare Research New Zealand Ltd
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

/*
 * This class is based on the itk::MeanImageFilter. It has been copied and
 * adjusted by Alexander Herzig, Landcare Research New Zealand Ltd.
 *
 */

#ifndef __otbNMScriptableKernelFilter_h
#define __otbNMScriptableKernelFilter_h


// First make sure that the configuration is available.
// This line can be removed once the optimized versions
// gets integrated into the main directories.

#include "nmlog.h"
#include "itkImageToImageFilter.h"
#include "itkImage.h"
#include "itkNumericTraits.h"
#include "mpParser.h"

#include "otbsupplfilters_export.h"

namespace otb
{
/*  \brief scriptable kernel filter
 *
 *
 *
 */
template <class TInputImage, class TOutputImage>
class ITK_EXPORT NMScriptableKernelFilter :
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

  /** Standard class typedefs. */
  typedef NMScriptableKernelFilter                          Self;
  typedef itk::ImageToImageFilter< InputImageType, OutputImageType> Superclass;
  typedef itk::SmartPointer<Self>                                   Pointer;
  typedef itk::SmartPointer<const Self>                             ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self)

  /** Run-time type information (and related methods). */
  itkTypeMacro(NMScriptableKernelFilter, itk::ImageToImageFilter)
  
  /** Image typedef support. */
  typedef typename InputImageType::PixelType               InputPixelType;
  typedef typename OutputImageType::PixelType              OutputPixelType;
  typedef typename itk::NumericTraits<InputPixelType>::RealType InputRealType;
  
  typedef typename InputImageType::RegionType  InputImageRegionType;
  typedef typename OutputImageType::RegionType OutputImageRegionType;
  typedef typename InputImageType::SizeType    InputSizeType;

  /** Set the radius of the neighborhood . */
  itkSetMacro(Radius, InputSizeType)

  /** Get the radius of the neighbourhood*/
  itkGetConstReferenceMacro(Radius, InputSizeType)
  
  /** Set the kernel script */
  itkSetStringMacro(KernelScript)

  /** Set the kernel shape <Square, Circle> */
  itkSetStringMacro(KernelShape)

  /** Set the nodata value of the computation */
  itkSetMacro(Nodata, OutputPixelType)

  /** Forward component UserIDs to the filter*/
  void SetInputNames(const std::vector<std::string>& inputNames);


  /** The neighbourhood counting filter needs a larger input requested region than
   * the output requested region.
   * \sa ImageToImageFilter::GenerateInputRequestedRegion() */
  virtual void GenerateInputRequestedRegion() throw(itk::InvalidRequestedRegionError);

#ifdef ITK_USE_CONCEPT_CHECKING
  /** Begin concept checking */
  itkConceptMacro(InputHasNumericTraitsCheck,
                  (itk::Concept::HasNumericTraits<InputPixelType>));
  /** End concept checking */
#endif

protected:
  NMScriptableKernelFilter();
  virtual ~NMScriptableKernelFilter() {}
  void PrintSelf(std::ostream& os, itk::Indent indent) const;

  /** This filter can be implemented as a multithreaded filter.
   * Therefore, this implementation provides a ThreadedGenerateData()
   * routine which is called for each processing thread. The output
   * image data is allocated automatically by the superclass prior to
   * calling ThreadedGenerateData().  ThreadedGenerateData can only
   * write to the portion of the output image specified by the
   * parameter "outputRegionForThread"
   *d
   * \sa ImageToImageFilter::ThreadedGenerateData(),
   *     ImageToImageFilter::GenerateData() */
  void ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
                            itk::ThreadIdType threadId );

  void BeforeThreadedGenerateData();
  void Reset();
  void CacheInputData();
  void ParseScript();
  void ParseCommand();

private:
  NMScriptableKernelFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  long long m_NumPixels;
  long long m_PixelCounter;

  InputSizeType m_Radius;
  std::string m_KernelScript;

  std::string m_KernelShape;
  std::vector<std::string> m_DataNames;

  OutputPixelType m_Nodata;

  // admin objects for the scriptable kernel filter
  std::vector<std::vector<mup::ParserX*> > m_vecParsers;
  std::vector<std::map<mup::ParserX*, std::string> > m_mapParserName;
  std::vector<std::map<std::string, mup::Value> m_mapNameImgValue;
  std::map<std::string, mup::Value> m_mapNameAuxValue;
  std::vector<std::vector<int> > m_vecBlockLen;

};
  
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "otbNMScriptableKernelFilter.txx"
#endif

//#endif

#endif
