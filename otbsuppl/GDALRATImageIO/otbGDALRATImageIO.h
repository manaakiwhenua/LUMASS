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
#include "otbImageIOBase.h"

#include "nmlog.h"
#include "otbAttributeTable.h"
#include "otbRAMTable.h"
#include "otbSQLiteTable.h"

/* GDAL Libraries */
#include "gdal.h"
#include "gdal_priv.h"
#include "cpl_string.h"
#include "cpl_conv.h"
#include "ogr_spatialref.h"
#include "ogr_srs_api.h"
#include "gdal_rat.h"
#include "otbgdalratimageio_export.h"

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
class OTBGDALRATIMAGEIO_EXPORT GDALRATImageIO : public otb::ImageIOBase
{
public:

  typedef unsigned char InputPixelType;

  /** Standard class typedefs. */
  typedef GDALRATImageIO             Self;
  typedef otb::ImageIOBase        Superclass;
  typedef itk::SmartPointer<Self> Pointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(GDALRATImageIO, otb::ImageIOBase);

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

  /** Set/Get the band map to be read/written by this IO */
  void SetBandMap(std::vector<int> map)
    {m_BandMap = map;}
  std::vector<int> GetBandMap() {return m_BandMap;}

  /** Set/Get whether pixels are represented as RGB(A) pixels */
  itkSetMacro(RGBMode, bool)
  itkGetMacro(RGBMode, bool)

  /** Get total number of components (bands) of source data set */
  int GetTotalNumberOfBands(void) {return m_NbBands;}

  /** Set/Get whether files should be opened in update mode
   *  rather than being overridden */
  itkSetMacro(ImageUpdateMode, bool);
  itkGetMacro(ImageUpdateMode, bool);

  itkGetMacro(NbOverviews, int);

  itkGetMacro(OverviewIdx, int);
  void SetOverviewIdx(int idx);

  itkGetMacro(RATType, otb::AttributeTable::TableType);
  itkSetMacro(RATType, otb::AttributeTable::TableType);

  std::vector<unsigned int> GetOverviewSize(int ovv);

  /** retreive the upper left corner of the image */
  std::vector<double> GetUpperLeftCorner(void)
    {return m_UpperLeftCorner;}


  /** surrounding region specs */
  void SetForcedLPR(const itk::ImageIORegion& forcedLPR);

  /** close the gdal data set incase it is still open */
  void CloseDataset(void);


  /** define some stubs here for abstract methods inherited from base clase */
  virtual unsigned int GetOverviewsCount() {return (unsigned int)0;} 
  virtual std::vector<std::string> GetOverviewsInfo() {return std::vector<std::string>();}
  virtual void SetOutputImagePixelType( bool isComplexInternalPixelType, 
	  bool isVectorImage) {}

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
  //bool GetSubDatasetInfo(std::vector<std::string>& names, std::vector<std::string>& desc);

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


  /** sets a pointer for the nth band RAT */
  void setRasterAttributeTable(otb::AttributeTable* rat, int band);

  /** Writes an attribute table to the nth band of the specified
   *  GDALDataset
   */
  void WriteRAT(AttributeTable::Pointer intab, unsigned int iBand=1);

  /** Builds overviews for all available bands in the file */
  void BuildOverviews(const std::string& resamplingType);

  unsigned int* getLPR(void)
  {return this->m_LPRDimensions;}

protected:
  /** Constructor.*/
  GDALRATImageIO();
  /** Destructor.*/
  virtual ~GDALRATImageIO();

  /** update overview-related information */
  void updateOverviewInfo();

  void PrintSelf(std::ostream& os, itk::Indent indent) const;
  /** Read all information on the image*/
  void InternalReadImageInformation();
  /** Write all information on the image*/
  void InternalWriteImageInformation(const void* buffer);
  /** Read RAT into the desired underlying implementation of AttributeTable */
  SQLiteTable::Pointer InternalReadSQLiteRAT(unsigned int iBand);
  RAMTable::Pointer InternalReadRAMRAT(unsigned int iBand);

  /** Write specified RAT type into the image */
  void InternalWriteRAMRAT(AttributeTable::Pointer intab, unsigned int iBand);
  void InternalWriteSQLiteRAT(AttributeTable::Pointer intab, unsigned int iBand);

  /** stores the coordinates of the upper left corner of the image */
  std::vector<double> m_UpperLeftCorner;

  /** Number of bands of the image*/
  int m_NbBands;

  /** map of band numbers to be read/written */
  std::vector<int> m_BandMap;

  /** pixels in RGB mode? */
  bool m_RGBMode;

  /** number of overviews */
  int m_NbOverviews;

  /** dimensions of each overview */
  std::vector<std::vector<unsigned int > > m_OvvSize;

  /** current overview (as index) */
  int m_OverviewIdx;

  /** dimensions of the largest possible region */
  unsigned int m_LPRDimensions[2];

  /** spacing of the largest possible region */
  double m_LPRSpacing[2];


  /** Determines the level of compression for written files.
   *  Range 0-9; 0 = none, 9 = maximum , default = 4 */
  int         m_CompressionLevel;

  bool        m_IsIndexed;

  /** Dataset index to extract (starting at 0)*/
  unsigned int m_DatasetNumber;

  /** indicates whether RAT support is switched on or not */
  bool m_RATSupport;

  /** preferred output RAT type */
  otb::AttributeTable::TableType m_RATType;


  /** keeps pointers to the RAT of individual bands */
  std::vector<otb::AttributeTable::Pointer> m_vecRAT;

  /** are existing images being opened in update or (overwrite) mode?*/
  bool m_ImageUpdateMode;

  /** force a largest possible region */
  itk::ImageIORegion m_ForcedLPR;
  bool m_UseForcedLPR;

  bool m_ImgInfoHasBeenRead;


private:
  GDALRATImageIO(const Self &); //purposely not implemented
  void operator =(const Self&); //purposely not implemented

  /** Determine real file name to write the image */
  std::string GetGdalWriteImageFileName(const std::string& gdalDriverShortName, const std::string& filename) const;

  std::string FilenameToGdalDriverShortName(const std::string& name) const;

  /** if we're editing solely the RAT, we perform this check */
  bool TableStructureChanged(AttributeTable::Pointer tab, unsigned int iBand);

  GDALDataset* CreateCopy();

  /** GDAL parameters. */
  //typedef itk::SmartPointer<GDALDatasetWrapper> GDALDatasetWrapperPointer;
  //GDALDatasetWrapperPointer m_Dataset;
  GDALDataset* m_Dataset;

  GDALDataType m_GDALComponentType;
  /** Nombre d'octets par pixel */
  int m_BytePerPixel;

  bool GDALInfoReportCorner(const char * corner_name, double x, double y,
                            double& dfGeoX, double& dfGeoY) const;

  bool m_CreatedNotWritten;
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
