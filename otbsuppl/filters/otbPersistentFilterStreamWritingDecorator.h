/*=========================================================================

  Program:   ORFEO Toolbox
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


  Copyright (c) Centre National d'Etudes Spatiales. All rights reserved.
  See OTBCopyright.txt for details.


     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __otbPersistentFilterStreamWritingDecorator_h
#define __otbPersistentFilterStreamWritingDecorator_h

//#include "otbStreamingImageVirtualWriter.h"
#include "itkProcessObject.h"
#include "otbStreamingRATImageFileWriter.h"

namespace otb
{
/** \class PersistentFilterStreamWritingDecorator
 *  \brief This filter link a persistent filter with a StreamingRATImageWriter.
 *
 *  The StreamingVirtualWriter will break the input image into pieces and stream each
 *  piece through the persistent filter. That way, the persistent filter will computes
 *  its data on the whole image, but never loads the whole of it, and eventually processes
 *  each piece with multiple threads. Before the streaming of the whole image is triggered,
 *  the Reset() method of the persistent filter is called to clear the temporary data it might
 *  contain. After the streaming, the Synthetize() method is called to synthetize the
 *  temporary data. One can access the persistent filter via the GetFilter() method, and
 * StreamingVirtualWriter via the GetStreamer() method.
 *
 * \sa StreamingStatisticsImageFilter
 * \sa StreamingStatisticsVectorImageFilter
 */
template <class TFilter>
class ITK_EXPORT PersistentFilterStreamWritingDecorator
  : public itk::ProcessObject
{
public:
  /** Standard typedefs */
  typedef PersistentFilterStreamWritingDecorator Self;
  typedef itk::ProcessObject                 Superclass;
  typedef itk::SmartPointer<Self>            Pointer;
  typedef itk::SmartPointer<const Self>      ConstPointer;

  /** Type macro */
  itkNewMacro(Self);

  /** Creation through object factory macro */
  itkTypeMacro(PersistentFilterStreamWritingDecorator, ProcessObject);

  /** Template parameters typedefs */
  typedef TFilter                             FilterType;
  typedef typename FilterType::Pointer        FilterPointerType;
  typedef typename FilterType::InputImageType ImageType;

  typedef StreamingRATImageFileWriter<ImageType> StreamerType;
  typedef typename StreamerType::Pointer         StreamerPointerType;

  itkSetObjectMacro(Filter, FilterType);
  itkGetObjectMacro(Filter, FilterType);
  itkGetConstObjectMacro(Filter, FilterType);
  itkGetObjectMacro(Streamer, StreamerType);

  //itkSetMacro(FileName, const std::string &);
  //itkGetMacro(FileName, const std::string &);

  virtual void Update(void);

protected:
  /** Constructor */
  PersistentFilterStreamWritingDecorator();
  /** Destructor */
  virtual ~PersistentFilterStreamWritingDecorator() {}
  /**PrintSelf method */
  virtual void PrintSelf(std::ostream& os, itk::Indent indent) const;

  virtual void GenerateData(void);

  /// Object responsible for streaming
  StreamerPointerType m_Streamer;

  /// Object responsible for computation
  FilterPointerType m_Filter;

  //std::string m_FileName;


private:
  PersistentFilterStreamWritingDecorator(const Self &); //purposely not implemented
  void operator =(const Self&); //purposely not implemented

};
} // End namespace otb
#ifndef OTB_MANUAL_INSTANTIATION
#include "otbPersistentFilterStreamWritingDecorator.txx"
#endif

#endif
