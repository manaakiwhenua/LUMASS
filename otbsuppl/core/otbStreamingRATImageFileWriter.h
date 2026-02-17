/******************************************************************************
* Created by Alexander Herzig
* Copyright 2025-2026 New Zealand Institute for Bioeconomy Science Limited
*
* This file is part of 'LUMASS', which is free software: you can redistribute
* it and/or modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation, either version 3 of the License,
* or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
********************************************************************************/

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
#ifndef __otbStreamingRATImageFileWriter_h
#define __otbStreamingRATImageFileWriter_h

#ifdef BUILD_RASSUPPORT
    #include "RasdamanConnector.hh"
#endif

#include <mpi.h>

#include "otbAttributeTable.h"
#include "otbImageIOBase.h"
#include "itkImageToImageFilter.h"
#include "otbStreamingManager.h"

#ifdef _WIN32
  #include "nmotbsupplcorewriter_export.h"
#else 
  #include "nmotbsupplcoreio_export.h"
  #define NMOTBSUPPLCOREWRITER_EXPORT NMOTBSUPPLCOREIO_EXPORT
  #define NMOTBSupplCoreWriter_EXPORTS NMOTBSupplCoreIO_EXPORTS
#endif

namespace otb
{

/** \class StreamingRATImageFileWriter
 * \brief Writes image data to a single file with streaming process.
 *
 * StreamingRATImageFileWriter writes its input data to a single output file.
 * StreamingRATImageFileWriter interfaces with an ImageIO class to write out the
 * data whith streaming process.
 *
 * StreamingRATImageFileWriter will divide the output into several pieces
 * (controlled by SetNumberOfDivisionsStrippedStreaming, SetNumberOfLinesStrippedStreaming,
 * SetAutomaticStrippedStreaming, SetTileDimensionTiledStreaming or SetAutomaticTiledStreaming),
 * and call the upstream pipeline for each piece, tiling the individual outputs into one large
 * output. This reduces the memory footprint for the application since
 * each filter does not have to process the entire dataset at once.
 *
 * StreamingRATImageFileWriter will write directly the streaming buffer in the image file, so
 * that the output image never needs to be completely allocated
 *
 * \sa ImageFileWriter
 * \sa ImageSeriesReader
 * \sa ImageIOBase
 */
template <class TInputImage>
class NMOTBSUPPLCOREWRITER_EXPORT StreamingRATImageFileWriter : public itk::ImageToImageFilter<TInputImage, TInputImage>
{
public:
  /** Standard class typedefs. */
  typedef StreamingRATImageFileWriter                          Self;
  typedef itk::ImageToImageFilter<TInputImage, TInputImage> Superclass;
  typedef itk::SmartPointer<Self>                           Pointer;
  typedef itk::SmartPointer<const Self>                     ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(StreamingRATImageFileWriter, itk::ImageToImageFilter);

  /** Some typedefs for the input and output. */
  typedef TInputImage                            InputImageType;
  typedef typename InputImageType::Pointer       InputImagePointer;
  typedef typename InputImageType::RegionType    InputImageRegionType;
  typedef typename InputImageType::PixelType     InputImagePixelType;
  typedef TInputImage                            OutputImageType;
  typedef typename OutputImageType::Pointer      OutputImagePointer;
  typedef typename OutputImageType::RegionType   OutputImageRegionType;
  typedef typename OutputImageType::PixelType    OutputImagePixelType;
  typedef typename Superclass::DataObjectPointer DataObjectPointer;

  /** Dimension of input image. */
  itkStaticConstMacro(InputImageDimension, unsigned int,
                      InputImageType::ImageDimension);

  /** Streaming manager base class pointer */
  typedef StreamingManager<InputImageType>       StreamingManagerType;
  typedef typename StreamingManagerType::Pointer StreamingManagerPointerType;

  /**  Return the StreamingManager object responsible for dividing
   *   the region to write */
  StreamingManagerType* GetStreamingManager(void)
    {
    return m_StreamingManager;
    }

  /**  Set a user-specified implementation of StreamingManager
   *   used to divide the largest possible region in several divisions */
  void SetStreamingManager(StreamingManagerType* streamingManager)
    {
    m_StreamingManager = streamingManager;
    }

  /**  Set the streaming mode to 'stripped' and configure the number of strips
   *   which will be used to stream the image */
  void SetNumberOfDivisionsStrippedStreaming(unsigned int nbDivisions);

  /**  Set the streaming mode to 'tiled' and configure the number of tiles
   *   which will be used to stream the image */
  void SetNumberOfDivisionsTiledStreaming(unsigned int nbDivisions);

  /**  Set the streaming mode to 'stripped' and configure the number of strips
   *   which will be used to stream the image with respect to a number of line
   *   per strip */
  void SetNumberOfLinesStrippedStreaming(unsigned int nbLinesPerStrip);

  /**  Set the streaming mode to 'stripped' and configure the number of MB
   *   available. The actual number of divisions is computed automatically
   *   by estimating the memory consumption of the pipeline.
   *   Setting the availableRAM parameter to 0 means that the available RAM
   *   is set from the CMake configuration option.
   *   The bias parameter is a multiplier applied on the estimated memory size
   *   of the pipeline and can be used to fine tune the potential gap between
   *   estimated memory and actual memory used, which can happen because of
   *   composite filters for example */
  void SetAutomaticStrippedStreaming(unsigned int availableRAM = 0, double bias = 1.0);

  /**  Set the streaming mode to 'tiled' and configure the dimension of the tiles
   *   in pixels for each dimension (square tiles will be generated) */
  void SetTileDimensionTiledStreaming(unsigned int tileDimension);

  /**  Set the streaming mode to 'tiled' and configure the number of MB
   *   available. The actual number of divisions is computed automatically
   *   by estimating the memory consumption of the pipeline.
   *   Tiles will be square.
   *   Setting the availableRAM parameter to 0 means that the available RAM
   *   is set from the CMake configuration option
   *   The bias parameter is a multiplier applied on the estimated memory size
   *   of the pipeline and can be used to fine tune the potential gap between
   *   estimated memory and actual memory used, which can happen because of
   *   composite filters for example */
  void SetAutomaticTiledStreaming(unsigned int availableRAM = 0, double bias = 1.0);

  /**  Set buffer memory size (in bytes) use to calculate the number of stream divisions */
  itkLegacyMacro( void SetBufferMemorySize(unsigned long) );

  /**  Set the buffer number of lines use to calculate the number of stream divisions */
  itkLegacyMacro( void SetBufferNumberOfLinesDivisions(unsigned long) );

  /**  The number of stream divisions is calculate by using
   * OTB_STREAM_IMAGE_SIZE_TO_ACTIVATE_STREAMING and
   * OTB_STREAM_MAX_SIZE_BUFFER_FOR_STREAMING cmake variables.
   */
  itkLegacyMacro( void SetAutomaticNumberOfStreamDivisions(void) );

  /** Set the tiling automatic mode for streaming division */
  itkLegacyMacro( void SetTilingStreamDivisions(void) );
  /** Choose number of divisions in tiling streaming division */
  itkLegacyMacro( void SetTilingStreamDivisions(unsigned long) );

  /** Return the string to indicate the method use to calculate number of stream divisions. */
  itkLegacyMacro( std::string GetMethodUseToCalculateNumberOfStreamDivisions(void) );

  /** Set the number of pieces to divide the input.  The upstream pipeline
   * will be executed this many times. */
  itkLegacyMacro( void SetNumberOfStreamDivisions(unsigned long) );

  /** Get the number of pieces to divide the input. The upstream pipeline
   * will be executed this many times. */
  itkLegacyMacro( unsigned long GetNumberOfStreamDivisions(void) );

  /** Override UpdateOutputData() from ProcessObject to divide upstream
   * updates into pieces. This filter does not have a GenerateData()
   * or ThreadedGenerateData() method.  Instead, all the work is done
   * in UpdateOutputData() since it must update a little, execute a little,
   * update some more, execute some more, etc. */
  virtual void UpdateOutputData(itk::DataObject * itkNotUsed(output));

  /** ImageFileWriter Methods */

  /** Specify the name of the output file to write. */
  //itkGetStringMacro(FileName);

  /**
   * Set the filename and destroy the current driver.
   * \param filename the name of the file.
   */
  void SetFileNames(std::vector<std::string> filenames)
  {
    m_FileNames.clear();
    m_FileNames = filenames;
    m_ImageIOs.clear();

    this->Modified();
  }

  void SetFileName(std::string filename)
  {
      if (m_FileNames.size() > 0)
      {
          m_FileNames[0] = filename;
      }
      else
      {
          m_FileNames.push_back(filename);
      }
  }

  /** Set the resampling type for building pyramids
      (note: currently ignored for rasdaman) */
  itkSetStringMacro(ResamplingType)
  itkGetStringMacro(ResamplingType)

  /** Set the streaming type */
  itkSetStringMacro(StreamingMethod)
  itkGetStringMacro(StreamingMethod)

  /** Set the max streaming size */
  itkSetMacro(StreamingSize, int)
  itkGetMacro(StreamingSize, int)


  /** Specify the region to write. If left NULL, then the whole image
   * is written. */
  void SetIORegion(const itk::ImageIORegion& region);
  itkGetConstReferenceMacro(IORegion, itk::ImageIORegion)

  /** Set the compression On or Off */
  itkSetMacro(UseCompression, bool)
  itkGetConstReferenceMacro(UseCompression, bool)
  itkBooleanMacro(UseCompression)
  itkSetMacro(CompressionLevel, int)
  itkGetMacro(CompressionLevel, int)

  /** By default the MetaDataDictionary is taken from the input image and
   *  passed to the ImageIO. In some cases, however, a user may prefer to
   *  introduce her/his own MetaDataDictionary. This is often the case of
   *  the ImageSeriesWriter. This flag defined whether the MetaDataDictionary
   *  to use will be the one from the input image or the one already set in
   *  the ImageIO object. */
  itkSetMacro(UseInputMetaDataDictionary, bool)
  itkGetConstReferenceMacro(UseInputMetaDataDictionary, bool)
  itkBooleanMacro(UseInputMetaDataDictionary)

  void SetImageIO(otb::ImageIOBase* imgIO);
  otb::ImageIOBase* GetImageIO(void);
  const otb::ImageIOBase* GetImageIO(void) const;

  /**
   * Enable/disable writing of a .geom file with the ossim keyword list along with the written image
   */
  itkSetMacro(WriteGeomFile, bool)
  itkGetMacro(WriteGeomFile, bool)
  itkBooleanMacro(WriteGeomFile)

  /** Sets the input raster attribute table */
  void SetInputRAT(AttributeTable* rat, unsigned int idx=0);


  /** Indicate whether the writer should be used in 'update' mode for
   *  allowing externally driven streaming as well as updating
   *  parts of existing images
   */
  void SetUpdateMode(bool bUpdate);
  itkGetMacro(UpdateMode, bool)
  itkBooleanMacro(UpdateMode)


  itkSetMacro(WriteImage, bool)
  itkGetMacro(WriteImage, bool)
  itkBooleanMacro(WriteImage)

  itkSetMacro(ParallelIO, bool)
  itkGetMacro(ParallelIO, bool)

  itkSetMacro(MpiComm, MPI_Comm)
  itkGetMacro(MpiComm, MPI_Comm)

  /** introduce a rasdaman mode to run this writer explicitly with
   *  with an RasdamanImageIO; this seems to be a dirty hack and violates
   *  somehow the generica nature of this writer, but otherwise rasdaman
   *  collections names (which don't come with a file extension)
   *  are always doomed to be invlaid file names and an exception is thrown
   */
#ifdef BUILD_RASSUPPORT
  void SetRasdamanConnector(RasdamanConnector* rascon);
  RasdamanConnector* GetRasdamanConnector() {return mRasconn;}
#endif

  /** Specifiy a user specified largest possible region to be written as
   *  part of the image information; this is primarily meant for initialising
   *  a partly empty image, which is going to be updated subsequently
   *  by repetitive calls of this writer class (i.e. allows for external
   *  'streaming logic' which becomes necessary when certain algorithms
   *  have to work repetitively on large images)
   */
  void SetForcedLargestPossibleRegion(const itk::ImageIORegion& forcedReg);
  itk::ImageIORegion GetForcedLargestPossibleRegion() {return m_ForcedLargestPossibleRegion;}

  /** specify the update region valid for the current call of this writer
   *  class; NOTE: this is supposed to be used in conjunction of the
   *  SetForcedLargestPossilbeRegion method to initiate externally streamed
   *  writing
   */
  void SetUpdateRegion(const itk::ImageIORegion& updateRegion);

  void BuildOverviews();

protected:
  StreamingRATImageFileWriter();
  virtual ~StreamingRATImageFileWriter();
  void PrintSelf(std::ostream& os, itk::Indent indent) const;

  std::string printRegion(const itk::ImageIORegion& ioreg);
  std::string printRegion(const OutputImageRegionType& reg);


  void VerifyInputInformation();
  void GenerateOutputInformation();

  /** Does the real work. */
  virtual void GenerateData(void);


private:
  StreamingRATImageFileWriter(const StreamingRATImageFileWriter &); //purposely not implemented
  void operator =(const StreamingRATImageFileWriter&); //purposely not implemented

  void ObserveSourceFilterProgress(itk::Object* object, const itk::EventObject & event )
  {
    if (typeid(event) != typeid(itk::ProgressEvent))
      {
      return;
      }

    itk::ProcessObject* processObject = dynamic_cast<itk::ProcessObject*>(object);
    if (processObject)
      {
      m_DivisionProgress = processObject->GetProgress();
      }

    this->UpdateFilterProgress();
  }

  void UpdateFilterProgress()
  {
    this->UpdateProgress( (m_DivisionProgress + m_CurrentDivision) / m_NumberOfDivisions );
  }

  unsigned int m_NumberOfInputs;
  unsigned int m_CurrentWriteImage;
  unsigned int m_NumberOfDivisions;
  unsigned int m_CurrentDivision;
  float m_DivisionProgress;

  /** ImageFileWriter Parameters */
  std::vector<std::string> m_FileNames;
  std::string m_ResamplingType;
  std::string m_StreamingMethod;  // TILED | STRIPPED
  int m_StreamingSize;          // MB streaming pieces
  int m_CompressionLevel;        // 0 (none) - 9 (max)
  std::vector<otb::ImageIOBase::Pointer> m_ImageIOs;

  bool m_UserSpecifiedImageIO; //track whether the ImageIO is user specified

  itk::ImageIORegion m_IORegion;
  bool               m_UserSpecifiedIORegion; // track whether the region is user specified
  bool m_FactorySpecifiedImageIO; //track whether the factory mechanism set the ImageIO
  bool m_UseCompression;
  bool m_UseInputMetaDataDictionary; // whether to use the
                                     // MetaDataDictionary from the
                                     // input or not.

  bool m_WriteGeomFile;              // Write a geom file to store the kwl
  bool m_ParallelIO;

  MPI_Comm m_MpiComm;

  StreamingManagerPointerType m_StreamingManager;


  itk::ImageIORegion m_UpdateRegion;
  itk::ImageIORegion m_ForcedLargestPossibleRegion;
  bool m_UseForcedLPR;
  bool m_UseUpdateRegion;
  bool m_UpdateMode;
  bool m_WriteImage;

#ifdef BUILD_RASSUPPORT
  RasdamanConnector* mRasconn;
#endif

  std::vector<AttributeTable::Pointer> m_InputRATs;
  bool m_RATHaveBeenWritten;

};

} // end namespace otb

#ifdef NMOTBSupplCoreWriter_EXPORTS
  #include "otbStreamingRATImageFileWriter.txx"
#endif

#ifdef _WIN32
#include "itkRGBPixel.h"
#include "otbImage.h"
#include "otbVectorImage.h"

extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<unsigned int, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<int, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<unsigned char, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<char, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<unsigned short, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<short, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<float, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<double, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<unsigned long, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<long, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<unsigned long long, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<long long, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<unsigned int, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<int, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<unsigned char, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<char, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<unsigned short, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<short, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<float, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<double, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<unsigned long, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<long, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<unsigned long long, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<long long, 1>>;

extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<unsigned int, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<int, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<unsigned char, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<char, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<unsigned short, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<short, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<float, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<double, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<unsigned long, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<long, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<unsigned long long, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<long long, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<unsigned int, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<int, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<unsigned char, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<char, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<unsigned short, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<short, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<float, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<double, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<unsigned long, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<long, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<unsigned long long, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<long long, 2>>;

extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<unsigned int, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<int, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<unsigned char, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<char, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<unsigned short, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<short, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<float, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<double, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<unsigned long, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<long, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<unsigned long long, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<long long, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<unsigned int, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<int, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<unsigned char, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<char, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<unsigned short, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<short, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<float, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<double, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<unsigned long, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<long, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<unsigned long long, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::VectorImage<long long, 3>>;

extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<unsigned int>, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<int>, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<unsigned char>, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<char>, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<unsigned short>, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<short>, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<float>, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<double>, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<unsigned long>, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<long>, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<unsigned long long>, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<long long>, 1>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<unsigned int>, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<int>, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<unsigned char>, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<char>, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<unsigned short>, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<short>, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<float>, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<double>, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<unsigned long>, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<long>, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<unsigned long long>, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<long long>, 2>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<unsigned int>, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<int>, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<unsigned char>, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<char>, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<unsigned short>, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<short>, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<float>, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<double>, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<unsigned long>, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<long>, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<unsigned long long>, 3>>;
extern template class NMOTBSUPPLCOREWRITER_EXPORT otb::StreamingRATImageFileWriter<otb::Image<itk::RGBPixel<long long>, 3>>;
#endif // _WIN32


#endif
