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

//#include "nmlog.h"

#include "itkImageToImageFilter.h"
#include "itkImage.h"
#include "itkNumericTraits.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkConstShapedNeighborhoodIterator.h"

#include "mpParser.h"

#include "nmotbsupplfilters_export.h"


namespace otb
{
/*! \class NMScriptableKernelFilter
 *  \brief A scriptable (optionally shaped neighborhood) filter
 *
 *  This filter allows the user to run a custom script
 *  on a pixel or a (optionally circular) kernel of user specified
 *  size. The user-defined script represents a sequence of
 *  muParserX-based expressions (completed by a semi-colon ';'),
 *  with the added functionality of c-style for loops, e.g.
 *
 *  size=10;
 *  a=0;
 *  b=0;
 *  for (i=0; i < size; i = i+1)
 *  {
 *      a=i;
 *  }
 *  b=a;
 *
 *  A for loop may not be nested in a muParserX expression,
 *  as for example
 *
 *  (a<0) ? for ... : 0;
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
  typedef typename InputImageType::IndexType   IndexType;
  typedef typename InputImageType::OffsetType  OffsetType;

  typedef typename itk::ConstShapedNeighborhoodIterator<InputImageType> InputShapedIterator;
  typedef typename itk::ImageRegionConstIterator<InputImageType> InputRegionIterator;
  typedef typename itk::ConstNeighborhoodIterator<InputImageType> InputNeighborhoodIterator;
  typedef typename itk::ImageRegionIterator<OutputImageType> OutputRegionIterator;

  /** Set the radius of the neighborhood . */
  itkSetMacro(Radius, InputSizeType)
  void SetRadius(const int* radius);

  /** Get the radius of the neighbourhood*/
  itkGetConstReferenceMacro(Radius, InputSizeType)
  
  /** Set the kernel script */
  itkSetStringMacro(KernelScript)

  /** Set the kernel shape <Square, Circle> */
  itkSetStringMacro(KernelShape)

  /** Set the variable name associated with
   *  the output image pixel(s) */
  itkSetStringMacro(OutputVarName)

  /** Set the nodata value of the computation */
  //itkSetMacro(Nodata, OutputPixelType)
  void SetNodata(const double& nodata);

  /** Forward component UserIDs to the filter*/
  void SetInputNames(const std::vector<std::string>& inputNames);

  void SetFilterInput(const unsigned int& idx, itk::DataObject* dataObj);


  /** The neighbourhood counting filter needs a larger input requested region than
   * the output requested region.
   * \sa ImageToImageFilter::GenerateInputRequestedRegion() */
  virtual void GenerateInputRequestedRegion() throw();

  void Reset();

#ifdef ITK_USE_CONCEPT_CHECKING
  /** Begin concept checking */
  itkConceptMacro(InputHasNumericTraitsCheck,
                  (itk::Concept::HasNumericTraits<InputPixelType>));
  /** End concept checking */
#endif

protected:
  NMScriptableKernelFilter();
  virtual ~NMScriptableKernelFilter();
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
  void AfterThreadedGenerateData();
  void CacheInputData();
  void ParseScript();
  void ParseCommand(const std::string& expr);

  inline void Loop(int i, const int& threadId)
  {
      const int numForExp = m_vecBlockLen.at(i)-3;
      mup::ParserX* testParser = m_vecParsers.at(threadId).at(++i);
      mup::Value& testValue = m_mapNameAuxValue.at(threadId).find(m_mapParserName.find(testParser)->second)->second;
      testValue = testParser->Eval();
      const mup::ParserX* counterParser = m_vecParsers.at(threadId).at(++i);

      while (testValue.GetFloat())
      {
          for (int exp=1; exp <= numForExp; ++exp)
          {
              const mup::ParserX* forExp = m_vecParsers.at(threadId).at(i+exp);
              forExp->Eval();

              if (m_vecBlockLen.at(i+exp) > 1)
              {
                  Loop(i+exp, threadId);
                  exp += m_vecBlockLen.at(i+exp)-1;
              }
          }

          counterParser->Eval();
          // since we don't impose the test expression of a for loop
          // to include a result assignment,  we do it manually here,
          // just in case ...
          testValue = testParser->Eval();
      }
  }

private:
  NMScriptableKernelFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  long long m_NumPixels;
  long long m_PixelCounter;
  std::vector<long long> m_vthPixelCounter;
  int m_NumNeighbourPixel;

  InputSizeType m_Radius;
  std::vector<OffsetType> m_ActiveKernelOffsets;

  std::string m_KernelScript;
  std::string m_KernelShape;
  std::string m_OutputVarName;

  std::vector<std::string> m_DataNames;

  OutputPixelType m_Nodata;
  std::vector<mup::Value> m_vOutputValue;
  std::vector<long long> m_NumOverflows;
  std::vector<long long> m_NumUnderflows;

  // admin objects for the scriptable kernel filter

  // need a separate set of parsers and img variables
  // for each individual thread
  std::vector<std::vector<mup::ParserX*> > m_vecParsers;
  std::vector<std::map<std::string, mup::Value> > m_mapNameImgValue;
  std::vector<std::map<std::string, mup::Value> > m_mapNameAuxValue;

  // can share those across threads
  std::map<mup::ParserX*, std::string> m_mapParserName;
  std::map<std::string, InputImageType*> m_mapNameImg;
  std::vector<int> m_vecBlockLen;


};
  
} // end namespace itk

//#include "otbNMScriptableKernelFilter_ExplicitInst.h"

#ifndef ITK_MANUAL_INSTANTIATION
#include "otbNMScriptableKernelFilter.txx"
#endif

//#endif

#endif
