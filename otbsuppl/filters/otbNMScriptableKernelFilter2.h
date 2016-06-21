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
 * This class is inspired by the itk::MeanImageFilter. It has been copied and
 * adjusted by Alexander Herzig, Landcare Research New Zealand Ltd.
 *
 */

#ifndef __otbNMScriptableKernelFilter2_h
#define __otbNMScriptableKernelFilter2_h


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

//#include "mpParser.h"
#include "otbMultiParser.h"

#include "otbsupplfilters_export.h"

namespace otb
{
/*! \class NMScriptableKernelFilter2
 *  \brief A scriptable (optionally shaped neighborhood) filter
 *
 *  This filter allows the user to run a custom script
 *  on a pixel or a (optionally circular) kernel of user specified
 *  size. The user-defined script represents a sequence of
 *  muParser-based expressions (completed by a semi-colon ';'),
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
 *  A for loop may not be nested in a muParser expression,
 *  as for example
 *
 *  (a<0) ? for ... : 0;
 *
 */
template <class TInputImage, class TOutputImage>
class ITK_EXPORT NMScriptableKernelFilter2 :
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
  typedef NMScriptableKernelFilter2                          Self;
  typedef itk::ImageToImageFilter< InputImageType, OutputImageType> Superclass;
  typedef itk::SmartPointer<Self>                                   Pointer;
  typedef itk::SmartPointer<const Self>                             ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self)

  /** Run-time type information (and related methods). */
  itkTypeMacro(NMScriptableKernelFilter2, itk::ImageToImageFilter)
  
  /** Image typedef support. */
  typedef typename InputImageType::PixelType               InputPixelType;
  typedef typename OutputImageType::PixelType              OutputPixelType;
  typedef typename itk::NumericTraits<InputPixelType>::RealType InputRealType;
  
  typedef typename InputImageType::RegionType  InputImageRegionType;
  typedef typename OutputImageType::RegionType OutputImageRegionType;
  typedef typename InputImageType::SizeType    InputSizeType;
  typedef typename InputImageType::IndexType   IndexType;
  typedef typename InputImageType::OffsetType  OffsetType;

  typedef typename otb::MultiParser            ParserType;
  typedef typename otb::MultiParser::Pointer   ParserPointerType;
  typedef typename otb::MultiParser::ValueType ParserValue;
  typedef typename otb::MultiParser::CharType CharType;

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
  virtual void GenerateInputRequestedRegion() throw(itk::InvalidRequestedRegionError);

  void Reset();

  static const double kwinVal(const char* imgName, double kwinIdx, double threadId, double thisAddr)
  {
      return m_mapNameImgValue[static_cast<double>(thisAddr)][threadId][imgName][kwinIdx];
  }

  static const double tabVal(const char* tabName, double colIdx, double rowIdx, double thisAddr)
  {
      return m_mapNameTable[static_cast<double>(thisAddr)][tabName][colIdx][rowIdx];
  }

#ifdef ITK_USE_CONCEPT_CHECKING
  /** Begin concept checking */
  itkConceptMacro(InputHasNumericTraitsCheck,
                  (itk::Concept::HasNumericTraits<InputPixelType>));
  /** End concept checking */
#endif

protected:
  NMScriptableKernelFilter2();
  virtual ~NMScriptableKernelFilter2();
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
      const int numForExp = m_vecBlockLen[i]-3;
      const ParserPointerType& testParser = m_vecParsers[threadId][++i];
      ParserValue& testValue = m_mapNameAuxValue[threadId][m_mapParserName[testParser.GetPointer()]];
      testValue = testParser->Eval();

      ParserPointerType& counterParser = m_vecParsers[threadId][++i];
      ParserValue& counterValue = m_mapNameAuxValue[threadId][m_mapParserName[counterParser.GetPointer()]];

      while (testValue != 0)
      {
          for (int exp=1; exp <= numForExp; ++exp)
          {
              const ParserPointerType& forParser = m_vecParsers[threadId][i+exp];
              ParserValue& forValue = m_mapNameAuxValue[threadId][m_mapParserName[forParser.GetPointer()]];
              forValue = forParser->Eval();

              if (m_vecBlockLen[i+exp] > 1)
              {
                  Loop(i+exp, threadId);
                  exp += m_vecBlockLen[i+exp]-1;
              }
          }

          counterValue = counterParser->Eval();
          testValue = testParser->Eval();
      }
  }

private:
  NMScriptableKernelFilter2(const Self&); //purposely not implemented
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

  double m_This;
  std::vector<double> m_thid;

  // admin objects for the scriptable kernel filter

  // have to define these separately for each thread ...
  // the parsers themselves
  std::vector<std::vector<ParserPointerType> > m_vecParsers;
  // the map kernel values
  static std::map<double, std::vector<std::map<std::string, std::vector<ParserValue> > > > m_mapNameImgValue;
  // the output value per thread
  std::vector<ParserValue> m_vOutputValue;
  // the over- and underflows per thread
  std::vector<long long> m_NumOverflows;
  std::vector<long long> m_NumUnderflows;

  // these are aux scalar values which are defined in the KernelScript itself, e.g.
  // loop counter and test variables
  std::vector<std::map<std::string, ParserValue> > m_mapNameAuxValue;

  // can share those across threads ...
  // read-only input tables
  static std::map<double, std::map<std::string, std::vector<std::vector<ParserValue> > > > m_mapNameTable;
  // the link between each parser and the name of the variable representing its output value
  std::map<MultiParser*, std::string> m_mapParserName;
  // the link between input images and their user defined names
  std::map<std::string, InputImageType*> m_mapNameImg;
  // the length of each script block; note a block is either a single statement/expression,
  // or a for loop including the test and counter variables
  std::vector<int> m_vecBlockLen;


};
  
} // end namespace itk

// definition of static member vars of above class
template<class TInputImage, class TOutput>
std::map<double, std::map<std::string, std::vector<std::vector<typename otb::NMScriptableKernelFilter2<TInputImage, TOutput>::ParserValue> > > >
otb::NMScriptableKernelFilter2<TInputImage, TOutput>::m_mapNameTable;

template<class TInputImage, class TOutput>
std::map<double, std::vector<std::map<std::string, std::vector<typename otb::NMScriptableKernelFilter2<TInputImage, TOutput>::ParserValue> > > >
otb::NMScriptableKernelFilter2<TInputImage, TOutput>::m_mapNameImgValue;


#include "otbNMScriptableKernelFilter2_ExplicitInst.h"

#ifndef ITK_MANUAL_INSTANTIATION
#include "otbNMScriptableKernelFilter2.txx"
#endif


#endif
