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
#ifndef __otbGDALRATImageIO_h
#define __otbGDALRATImageIO_h


/* C++ Libraries */
#include <string>

/* ITK Libraries */
#include "itkImageIOBase.h"

#include "nmlog.h"
#include "otbAttributeTable.h"

/* GDAL Libraries */
#include "gdal.h"
#include "gdal_priv.h"
#include "cpl_string.h"
#include "cpl_conv.h"
#include "ogr_spatialref.h"
#include "ogr_srs_api.h"
#include "gdal_rat.h"

namespace otb
{
class GDALDatasetWrapper;
class GDALDataTypeWrapper;

/** \class GDALRATImageIO
 *
 * \brief ImageIO object for reading (not writing) GDAL images
 *
 * The streaming read is implemented.
 *
 * \ingroup IOFilters
 *
 */
class ITK_EXPORT GDALRATImageIO : public itk::ImageIOBase
{
public:

  typedef unsigned char InputPixelType;

  /** Standard class typedefs. */
  typedef GDALRATImageIO             Self;
  typedef itk::ImageIOBase        Superclass;
  typedef itk::SmartPointer<Self> Pointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(GDALRATImageIO, itk::ImageIOBase);

  /** Set/Get the level of compression for the output images.
   *  0-9; 0 = none, 9 = maximum. */
  itkSetMacro(CompressionLevel, int);
  itkGetMacro(CompressionLevel, int);

  /** Set/Get whether the pixel type (otb side) is complex */
  itkSetMacro(IsComplex, bool);
  itkGetMacro(IsComplex, bool);

  /** Set/Get whether the pixel type (otb side) is Vector or Scalar */
  itkSetMacro(IsVectorImage, bool);
  itkGetMacro(IsVectorImage, bool);

  /** Set/Get the dataset index to extract (starting at 0)*/
  itkSetMacro(DatasetNumber, unsigned int);
  itkGetMacro(DatasetNumber, unsigned int);

  /** Set/Get the IO mode (i.e. RAT support on or off) */
  itkSetMacro(RATSupport, bool);
  itkGetMacro(RATSupport, bool);

  /** Set/Get whether files should be opened in update mode
   *  rather than being overridden */
  itkSetMacro(ImageUpdateMode, bool);
  itkGetMacro(ImageUpdateMode, bool);

  /** surrounding region specs */
  void SetForcedLPR(const itk::ImageIORegion& forcedLPR);
 // void SetUpdateRegion(const itk::ImageIORegion& updateRegion);

  /** close the gdal data set incase it is still open */
  void CloseDataset(void);

  /*-------- This part of the interface deals with reading data. ------ */

  /** Determine the file type. Returns true if this ImageIO can read the
   * file specified. */
  virtual bool CanReadFile(const char*);

  /** Determine the file type. Returns true if the ImageIO can stream read the specified file */
  virtual bool CanStreamRead()
  {
    return true;
  }

  /** Set the spacing and dimention information for the set filename. */
  virtual void ReadImageInformation();

  /** Reads the data from disk into the memory buffer provided. */
  virtual void Read(void* buffer);

  /** Reads 3D data from multiple files assuming one slice per file. */
  virtual void ReadVolume(void* buffer);

  /** Get Info about all subDataset in hdf file */
  bool GetSubDatasetInfo(std::vector<std::string>& names, std::vector<std::string>& desc);

  /** Get if the pixel type in the file is complex or not (GDAL side)*/
  bool GDALPixelTypeIsComplex();

  /*-------- This part of the interfaces deals with writing data. ----- */

  /** Determine the file type. Returns true if this ImageIO can read the
   * file specified. */
  virtual bool CanWriteFile(const char*);

  /** Determine the file type. Returns true if the ImageIO can stream write the specified file */
  virtual bool CanStreamWrite();

  /** Writes the spacing and dimentions of the image.
   * Assumes SetFileName has been called with a valid file name. */
  virtual void WriteImageInformation();

  /** Writes the data to disk from the memory buffer provided. Make sure
   * that the IORegion has been set properly. */
  virtual void Write(const void* buffer);

  /** Reads the Attribute Table of the nth band*/
  virtual AttributeTable::Pointer ReadRAT(unsigned int iBand);

protected:
  /** Constructor.*/
  GDALRATImageIO();
  /** Destructor.*/
  virtual ~GDALRATImageIO();

  void PrintSelf(std::ostream& os, itk::Indent indent) const;
  /** Read all information on the image*/
  void InternalReadImageInformation();
  /** Write all information on the image*/
  void InternalWriteImageInformation(const void* buffer);
  /** Number of bands of the image*/
  int m_NbBands;
  /** Buffer*/
  //float **pafimas;

  /** Determines the level of compression for written files.
   *  Range 0-9; 0 = none, 9 = maximum , default = 4 */
  int         m_CompressionLevel;

  bool        m_IsIndexed;

  /** Dataset index to extract (starting at 0)*/
  unsigned int m_DatasetNumber;

  /** indicates whether RAT support is switched on or not */
  bool m_RATSupport;

  /** are existing images being opened in update or (overwrite) mode?*/
  bool m_ImageUpdateMode;

  /** force a largest possible region */
  //itk::ImageIORegion m_UpdateRegion;
  itk::ImageIORegion m_ForcedLPR;
  bool m_UseForcedLPR;
  //bool m_UseUpdateRegion;


private:
  GDALRATImageIO(const Self &); //purposely not implemented
  void operator =(const Self&); //purposely not implemented

  /** Determine real file name to write the image */
  std::string GetGdalWriteImageFileName(const std::string& gdalDriverShortName, const std::string& filename) const;

  std::string FilenameToGdalDriverShortName(const std::string& name) const;


  /** GDAL parameters. */
  typedef itk::SmartPointer<GDALDatasetWrapper> GDALDatasetWrapperPointer;
  GDALDatasetWrapperPointer m_Dataset;

  GDALDataTypeWrapper*    m_PxType;
  /** Nombre d'octets par pixel */
  int m_BytePerPixel;

  bool GDALInfoReportCorner(const char * corner_name, double x, double y,
                            double& dfGeoX, double& dfGeoY) const;

  bool m_FlagWriteImageInformation;
  bool m_CanStreamWrite;

  /** Whether the pixel type (otb side, not gdal side) is complex
   * this information has to be provided by the reader */
  bool m_IsComplex;

  /** Whether the pixel type (otb side, not gdal side) is Vector
   * this information has to be provided by the reader */
  bool m_IsVectorImage;

  static const std::string ctx;

};

} // end namespace otb

#endif // __otbGDALRATImageIO_h
