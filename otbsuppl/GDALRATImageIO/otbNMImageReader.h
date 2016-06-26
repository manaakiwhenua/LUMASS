 /****************************************************************************** 
 * Created by Alexander Herzig 
 * Copyright 2015 Landcare Research New Zealand Ltd
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

    NOTE: this is an adjusted version of the ImageFileReader class
          to allow for reading and writing images GDAL-/rasdaman-based images
          supporting raster attribute tables
				
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
#ifndef __otbNMImageReader_h
#define __otbNMImageReader_h

#include "otbImageFileReader.h"
#include "otbAttributeTable.h"
#include "otbCurlHelper.h"
#include "otbCurlHelperInterface.h"

#ifdef BUILD_RASSUPPORT
    #include "RasdamanConnector.hh"
#endif

#include "otbgdalratimageio_export.h"

namespace otb
{

/** \class NMImageReader
 * \brief Reads GDAL- or rasdaman-based images, supporting
 *        raster attribute tables, user specified largest possible region
 *        (in conjunction with update mode) for both formats
 *        and image overview for GDAL-based images.
 *        It uses either the GDALRATImageIO or the RasdamanImageIO for
 *        the boilerplate I/O operations.
 *
 * \sa ImageFileReader
 * \sa ImageIOBase
 * \sa GDALRATImageIO
 * \sa RasdamanImageIO
 *
 * \ingroup IOFilters
 *
 */
template <class TOutputImage>
class OTBGDALRATIMAGEIO_EXPORT NMImageReader : public otb::ImageFileReader<TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef NMImageReader                     Self;
  typedef otb::ImageFileReader<TOutputImage>  Superclass;
  typedef itk::SmartPointer<Self>             Pointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(NMImageReader, Superclass);

  /** The pixel type of the output image. */
  typedef typename TOutputImage::InternalPixelType OutputImagePixelType;

  /** The size of the output image. */
  typedef typename TOutputImage::SizeType  SizeType;

  /** The region of the output image. */
  typedef typename TOutputImage::RegionType  ImageRegionType;

  /** Prepare image allocation at the first call of the pipeline processing */
  virtual void GenerateOutputInformation(void);

  /** Get/Set the overview to be retreived from the file */
  itkGetMacro(OverviewIdx, int)
  void SetOverviewIdx(int ovvidx);

  /** Allow for retro-fitting of overviews, in case not present */
  void BuildOverviews(const std::string& method);

  /** Allow the user to set a custom largest possible region
      to help with customised overview region loading */

  itkSetMacro(UseUserLargestPossibleRegion, bool)
  itkGetMacro(UseUserLargestPossibleRegion, bool)
  itkBooleanMacro(UseUserLargestPossibleRegion)

  itkSetMacro(UserLargestPossibleRegion, ImageRegionType)
  itkGetMacro(UserLargestPossibleRegion, ImageRegionType)

  /** Enable/disable RAT support for GDAL-based images
   *  (note: this swich has not effect rasdaman-based images)
   */
  itkSetMacro(RATSupport, bool)
  itkGetMacro(RATSupport, bool)
  itkBooleanMacro(RATSupport)

  /** Specifies whether images with 3 or more bands should
   *  be interpreated as RGB images RGBPixelType, or
   *  whether they're to be interpreted as VectorImageType
   */
  itkSetMacro(RGBMode, bool)
  itkGetMacro(RGBMode, bool)
  itkBooleanMacro(RGBMode)

//  virtual void SetFileName(std::string filename)
//    {this->SetFileName(filename.c_str());}
//  virtual void SetFileName(const char* filename);

//  virtual const char* GetFileName() const
//    {return this->m_FileName.c_str();}


  itkGetMacro(RATType, otb::AttributeTable::TableType);
  itkSetMacro(RATType, otb::AttributeTable::TableType);


  /** get the RAT from the reader*/
  AttributeTable::Pointer GetAttributeTable(int band);

#ifdef BUILD_RASSUPPORT
  /** Set/get the rasdaman connector object */
  void SetRasdamanConnector(RasdamanConnector* rasconn);

  RasdamanConnector* GetRasdamanConnector() {return this->mRasconn;}
#endif


  /** Does the real work. */
  virtual void GenerateData();

  /** Give the reader a chance to indicate that it will produce more
   * output than it was requested to produce. NMImageReader cannot
   * currently read a portion of an image (since the ImageIO objects
   * cannot read a portion of an image), so the NMImageReader must
   * enlarge the RequestedRegion to the size of the image on disk. */
  virtual void EnlargeOutputRequestedRegion(itk::DataObject *output);

protected:
  NMImageReader();
  virtual ~NMImageReader();
  void PrintSelf(std::ostream& os, itk::Indent indent) const;

  unsigned int m_DatasetNumber;

  AttributeTable::Pointer m_RAT;

  int m_OverviewIdx;
  bool m_UseUserLargestPossibleRegion;
  ImageRegionType m_UserLargestPossibleRegion;

  unsigned int m_AdditionalNumber;

  //std::string m_FileName;

  bool mbRasMode;
  bool m_RGBMode;
  bool m_RATSupport;
  otb::AttributeTable::TableType m_RATType;

private:

#ifdef BUILD_RASSUPPORT
  	  RasdamanConnector* mRasconn;
#endif
  /** Test whether the given filename exist and it is readable,
      this is intended to be called before attempting to use
      ImageIO classes for actually reading the file. If the file
      doesn't exist or it is not readable, and exception with an
      approriate message will be thrown. */
  void TestFileExistanceAndReadability();

  /** Generate the filename (for GDALImageI for example). If filename is a directory, look if is a
    * CEOS product (file "DAT...") In this case, the GdalFileName contain the open image file.
    */
  bool GetGdalReadImageFileName( const std::string & filename, std::string & GdalFileName );

  NMImageReader(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented


  std::string m_ExceptionMessage;
  CurlHelperInterface::Pointer m_Curl;

};


} //namespace otb

#ifndef OTB_MANUAL_INSTANTIATION
#include "otbNMImageReader.txx"
#endif

#endif // __otbNMImageReader_h
