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
 * extensively adapted by Alexander Herzig, Landcare Research New Zealand Ltd.
 *
 */

#ifndef __otbNMJSKernelFilter_h
#define __otbNMJSKernelFilter_h


// First make sure that the configuration is available.
// This line can be removed once the optimized versions
// gets integrated into the main directories.

//#include "nmlog.h"

#include "itkImageToImageFilter.h"
#include "itkImage.h"
#include "itkNumericTraits.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkNMConstShapedNeighborhoodIterator.h"
#include "itkNeighborhood.h"

//#include "otbMultiParser.h"

#include <QString>
#include <QJSEngine>
#include <QJSValue>
#include <QSharedPointer>

#include "otbAttributeTable.h"
#include "otbSQLiteTable.h"

#include "otbsupplfilters_export.h"

namespace otb
{
/*! \class NMJSKernelFilter
 *  \brief A JavaScript-scriptable (optionally shaped neighborhood) filter
 *
 * This filter allows the user to execute a custom script
 * written in JavaScript to calculate the output pixel value.
 * If required, the user may access individual pixel values of
 * a user-defined CIRCULAR or RECTANGULAR neighbourhood around
 * the centre pixel to be calculated.
 *
 *	THE NEIGHBOURHOOD
 *
 *      The filter supports a different radius for each
 *      image axis and the kernel window length (len) in pixel
 *      for each axis is
 *
 *      len = d * axis_radius + 1
 *        3 = 2 * axis_radius + 1        // for 2-dimensional image (kernel)
 *
 *      where d is the image axis (or dimension);
 *      for a kernel with radius 1 it yields a 3x3
 *      neighbourhood; pixel values can be accessed
 *      by its index position in the neighbourood
 *      which is numbered sequentially from left
 *      to right and top to bottom starting with 0
 *
 *      0  1  2
 *      3  4  5
 *      6  7  8
 *
 *	ACCESSING PIXEL VALUES
 *
 *	    Image values are referenced by the image identifier,
 *      which is the UserID of the particular input component
 *      (e.g. an ImageReader's UserID); if a radius of > 0 for any
 *      of the image diemensions is specified, the image values are
 *      referenced by their identifier AND their particular index
 *      value according to the above schedule (in case of a 3x3 kernel).
 *
 *	    In the presence of a neighbourhood, image values are retrieved
 *      using the kwinVal function, e.g. the centre pixel value (i.e. index 4)
 *      of image 'img' can be retrieved with
 *
 *      \code
 *      v = kwinVal(img, 4, thid, addr);
 *      \endcode
 *
 *	   note: the user only ever has to specify the
 *            first and second parameter of the kwinVal
 *            function, i.e. the image identifier and the pixel index!
 *            The 'thid' and 'addr' parameters always have to be specified
 *            as shown above and are required for technical reasons.
 *
 *	   For the convenience of the user, the index of the centre pixel is provided
 *      as a pre-defined constant 'centrePixIdx', and the centre pixel value could
 *      be retrieved by
 *
 *         \code
 *	   v = kwinVal(img, centrePixIdx, thid, addr);
 *         \endcode
 *
 *      regardless of the actual size and shape of the neighbourhood;
 *
 *
 *	ACCESSING TABLE VALUES
 *
 *	   Table values are referenced by their 0-based column and row index
 *      in the table. For example the value t, in column 4 and row 3 of the table
 *      'mytab' can be retrieved with
 *
 *         \code
 *	   t = tabVal(mytab, 3, 2, addr);
 *         \endcode
 *
 *	   note: similar to the kwinVal function, the tabVal function requires one
 *            administrative parmameter ('addr'), which has to be specified every
 *            time the function is being used
 *
 *      In contrast to 'standalone' tables as above, an input image's RAT is referenced
 *      by the input component's UserID (e.g. 'img') extended by the suffix '_t', i.e. 'img_t'
 *
 *
 * 	REFERENCING THE OUTPUT VALUE
 *
 *	   The name of the output value is user-defined an must be specified in the
 *      property section of this filter; in the sample scripts below, it is usually
 *      represented by the variable 'out'.
 *
 *
 *	GENERAL KERNEL SCRIPT SYNTAX
 *
 *      A kernel script is made up of a set of variable assignments, i.e. 'var = a + b;'.
 *      The right hand side (i.e. right of the '=' sign) is represented by a muParser
 *      expression. See the following website for details:
 *
 *	<a href="http://beltoforion.de/article.php?a=muparser&p=features">muParser</a>
 *
 *      If the left hand side (e.g. 'var = ') is missing, as common for the test expression
 *      in a for-loop header, an implict variable is assigned to it.
 *
 *      The typing of variables is implicit; every variable is of type double! Variable assignments
 *      have to specified explicitly using the '=' sign. A side-effect assignment like
 *      with the C-style '++' or '--' operator is not supported; also not support are the
 *      operators '+=', '-=', '/=', '*=' (anything else?).
 *
 *        \code
 *        size=10;
 *        out=0;
 *        b=5.7;
 *        for (i=1; i < size; i = i+1)
 *        {
 *            b = b * i;
 *        }
 *        out=b;
 *        \endcode
 *
 *      A for loop may not be nested in a muParser expression,
 *      as for example
 *
 *        \code
 *        var = a < 0 ? for (int myvar=0; myvar < numPix; myvar = myvar+1){out=out+}
 *                    : 0;
 *        \endcode
 *
 *      However, for loops itself may be nested, e.g.
 *
 *        \code
 *        for (i=0; i < number; i=i+1)
 *        {
 *            for (g=4; g >=0; g = g-1)
 *            {
 *                out = i*g;
 *            }
 *        }
 *        \endcode
 *
 *      and muParser expressions may be used in the loop header
 *      header or body (s. the floral resources script below).
 *
 *   RESERVED names (must not be used for user variables!):
 *
 *      numPix       : number of active pixel in the neighbourhood
 *      centrePixIdx : 1D neighbourhood index of the centre pixel
 *      addr         : object address, used in kwinVal, tabVal, neigDist
 *      thid         : thread id, used in kwinVal and tabVal
 *      kwinVal      : function to access neighbourhood values by 1D-index
 *      tabVal       : function to access table values by column and row index
 *      neigDist     : neighbour distance from centre pixel (in pixel)
 *
 *      otb::Math constants: e, log2e, log10e, ln2, ln10, pi, euler
 *      muParser standard functions: http://beltoforion.de/article.php?a=muparser&p=features
 *
 *      addtional functions:
 *
 *      rand(lower_limit, upper_limit)  : returns random value using std::rand
 *      fmod(numerator, denominator)    : returns remainder of float division using std::fmod
 *
 */
template <class TInputImage, class TOutputImage>
class OTBSUPPLFILTERS_EXPORT NMJSKernelFilter :
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
  typedef NMJSKernelFilter                          Self;
  typedef itk::ImageToImageFilter< InputImageType, OutputImageType> Superclass;
  typedef itk::SmartPointer<Self>                                   Pointer;
  typedef itk::SmartPointer<const Self>                             ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self)

  /** Run-time type information (and related methods). */
  itkTypeMacro(NMJSKernelFilter, itk::ImageToImageFilter)
  
  /** Image typedef support. */
  typedef typename InputImageType::PixelType               InputPixelType;
  typedef typename OutputImageType::PixelType              OutputPixelType;
  typedef typename itk::NumericTraits<InputPixelType>::RealType InputRealType;
  
  typedef typename InputImageType::RegionType  InputImageRegionType;
  typedef typename OutputImageType::RegionType OutputImageRegionType;
  typedef typename InputImageType::SizeType    InputSizeType;
  typedef typename InputImageType::IndexType   IndexType;

  typedef typename InputImageType::SpacingType  SpacingType;
  typedef typename InputImageType::PointType    OriginType;

  typedef typename itk::NMConstShapedNeighborhoodIterator<InputImageType> InputShapedIterator;
  typedef typename InputShapedIterator::OffsetType  OffsetType;
  typedef typename InputShapedIterator::NeighborIndexType NeighborIndexType;

  typedef typename itk::ImageRegionConstIterator<InputImageType> InputRegionIterator;
  typedef typename itk::ConstNeighborhoodIterator<InputImageType> InputNeighborhoodIterator;
  typedef typename itk::ImageRegionIterator<OutputImageType> OutputRegionIterator;

  /*! Set the radius of the neighborhood.
   *  RECTANGULAR neighbourhoods may have
   *  (of course) a different size for each
   *  dimension. However, if the KernelShape
   *  is CIRCULAR, we create a square neighbourhood
   *  based on the biggest size across dimensions
   *
   *  NOTE: the circular neighbourhood assumes
   *        square pixels
   */
  itkSetMacro(Radius, InputSizeType)
  void SetRadius(const int* radius);

  /*! Get the radius of the neighbourhood*/
  itkGetConstReferenceMacro(Radius, InputSizeType)
  
  /*! Set the kernel script */
  itkSetStringMacro(KernelScript)

  /*! Set the kernel script */
  itkSetStringMacro(WorkspacePath)

  /*! Set the initialisation script */
  itkSetStringMacro(InitScript)

  /*! Set the kernel shape <Square, Circle> */
  itkSetStringMacro(KernelShape)

  /*! Set the nodata value of the computation */
  itkSetMacro(Nodata, OutputPixelType)

  /*! Forward component UserIDs to the filter*/
  void SetInputNames(const std::vector<std::string>& inputNames);

  void setRAT(unsigned int idx, AttributeTable::Pointer table);

  void SetNumThreads(unsigned int nthreads);

  otb::AttributeTable::Pointer GetAuxOutput()
    {return static_cast<otb::AttributeTable*>(m_AuxTable);}


  /*! The filter needs a larger input requested region than
   * the output requested region.
   * \sa ImageToImageFilter::GenerateInputRequestedRegion() */
  virtual void GenerateInputRequestedRegion() throw();

  virtual itk::DataObject::Pointer MakeOutput(unsigned int idx);
  virtual void SetNthInput(itk::DataObject::DataObjectPointerArraySizeType num, itk::DataObject* input);

  void Reset();


#ifdef ITK_USE_CONCEPT_CHECKING
  /** Begin concept checking */
  itkConceptMacro(InputHasNumericTraitsCheck,
                  (itk::Concept::HasNumericTraits<InputPixelType>));
  /** End concept checking */
#endif

protected:
  NMJSKernelFilter();
  virtual ~NMJSKernelFilter();
  void PrintSelf(std::ostream& os, itk::Indent indent) const;

  /** This filter is implemented as a multithreaded filter.
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
  void RunInitScript();

private:
  NMJSKernelFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  unsigned int m_NumThreads;
  long long m_NumPixels;
  long long m_PixelCounter;
  std::vector<long long> m_vthPixelCounter;
  int m_NumNeighbourPixel;

  InputSizeType m_Radius;
  std::vector<NeighborIndexType> m_ActiveKernelIndices;
  int m_CentrePixelIndex;
  int m_ActiveNeighborhoodSize;

  std::string m_InitScript;
  std::string m_KernelScript;
  std::string m_KernelShape;
  std::string m_WorkspacePath;

  otb::SQLiteTable::Pointer m_AuxTable;

  std::vector<otb::AttributeTable::Pointer> m_vRAT;
  std::vector<std::string> m_RATNames;
  std::vector<std::string> m_IMGNames;
  std::vector<std::string> m_DataNames;
  OutputPixelType m_Nodata;

  QJSValue m_InitResultObject;

  SpacingType   m_Spacing;
  OriginType    m_Origin;

  // maps InitResult properties and their min,max,sum values
  // across threads
  std::vector<double> m_minVal;
  std::vector<double> m_maxVal;
  std::vector<double> m_sumVal;

  std::map<std::string, int> m_mapStatNameIdx;


  // ===================================================
  //        thread dependant object/value stores
  // ===================================================

  // have to define these separately for each thread ...
  // the parsers themselves
  QSharedPointer<QJSEngine> m_InitEngine;
  std::vector<QSharedPointer<QJSEngine> > m_vJSEngine;
  std::vector<QJSValue> m_vScript;
  std::vector<QJSValue> m_vKernelStore;
  std::vector<QJSValue> m_vKernelInfo;

  std::vector<std::map<std::string, QJSValue> > m_mapNameImgKernel;



  // the x, y, and z coordinate of centre pixel
//  std::vector<double> m_mapXCoord;
//  std::vector<double> m_mapYCoord;
//  std::vector<double> m_mapZCoord;


  // ===================================================
  //        thread independant object/value stores
  // ===================================================

  // the link between input images and their user defined names
  std::map<std::string, InputImageType*> m_mapNameImg;

  // the over- and underflows per thread
  std::vector<long long> m_NumOverflows;
  std::vector<long long> m_NumUnderflows;

  // ===================================================
  //        static value stores
  // ===================================================
  // these stores make class instance dependant values
  // available via the static muParser callback functions
  // ... bit a of dirty hack ... I guess

  // can share these across threads ...
  // read-only neighbour pixel distances to centre pixel
  std::vector<double> m_vNeighbourDistance;

  // read-only input tables
  std::map<std::string, std::vector<std::vector<double> > > m_mapNameTable;

  // the map kernel values are thread dependant
  std::vector<std::map<std::string, InputShapedIterator > > m_mapNameImgNeigValues;
  std::vector<std::map<std::string, double> > m_mapNameImgValue;

};
  
} // end namespace itk

//#include "otbNMJSKernelFilter_ExplicitInst.h"

#ifndef ITK_MANUAL_INSTANTIATION
#include "otbNMJSKernelFilter.txx"
#endif


#endif
