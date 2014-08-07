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
#ifndef __otbGDALRATImageFileReader_h
#define __otbGDALRATImageFileReader_h

#include "otbImageFileReader.h"
#include "otbAttributeTable.h"

namespace otb
{

/** \class GDALRATImageFileReader
 * \brief Resource to read GDAL images and associated raster attribute tables from a file.
 *
 * \sa ImageSeriesReader
 * \sa ImageIOBase
 *
 * \ingroup IOFilters
 *
 */
template <class TOutputImage>
class ITK_EXPORT GDALRATImageFileReader : public otb::ImageFileReader<TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef GDALRATImageFileReader                    Self;
  typedef ImageFileReader<TOutputImage> Superclass;
  typedef itk::SmartPointer<Self>            Pointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(GDALRATImageFileReader, Superclass);

  /** The pixel type of the output image. */
  typedef typename TOutputImage::InternalPixelType OutputImagePixelType;

  /** The size of the output image. */
  typedef typename TOutputImage::SizeType SizeType;

  /** The region of the output image. */
  typedef typename TOutputImage::RegionType ImageRegionType;

  /** Prepare image allocation at the first call of the pipeline processing */
  virtual void GenerateOutputInformation(void);

  itkGetMacro(OverviewIdx, int);
  void SetOverviewIdx(int ovvidx);

//
//  /** Does the real work. */
//  virtual void GenerateData();

  /** Give the reader a chance to indicate that it will produce more
   * output than it was requested to produce. GDALRATImageFileReader cannot
   * currently read a portion of an image (since the ImageIO objects
   * cannot read a portion of an image), so the GDALRATImageFileReader must
   * enlarge the RequestedRegion to the size of the image on disk. */
//  virtual void EnlargeOutputRequestedRegion(itk::DataObject *output);

  /** get the attribute table of the nth band **/
  virtual AttributeTable::Pointer GetAttributeTable(int iBand);

protected:
  GDALRATImageFileReader();
  virtual ~GDALRATImageFileReader();
  void PrintSelf(std::ostream& os, itk::Indent indent) const;

  unsigned int m_DatasetNumber;

  // pointer to the raster attribute table of the first band
  AttributeTable::Pointer m_RAT;

  int m_OverviewIdx;

private:
  /** Test whether the given filename exist and it is readable,
      this is intended to be called before attempting to use
      ImageIO classes for actually reading the file. If the file
      doesn't exist or it is not readable, and exception with an
      approriate message will be thrown. */
  	  void TestFileExistanceAndReadability();
//
//  //ImageKeywordlist GenerateKeywordList(const std::string& filename);
//
//  /** Generate the filename (for GDALImageI for example). If filename is a directory, look if is a
//    * CEOS product (file "DAT...") In this case, the GdalFileName contain the open image file.
//    */
  bool GetGdalReadImageFileName(const std::string& filename, std::string& GdalFileName);

  GDALRATImageFileReader(const Self &); //purposely not implemented
  void operator =(const Self&); //purposely not implemented

  std::string m_ExceptionMessage;

  CurlHelperInterface::Pointer m_Curl;

};

} //namespace otb

#ifndef OTB_MANUAL_INSTANTIATION
#include "otbGDALRATImageFileReader.txx"
#endif

#endif // __otbGDALRATImageFileReader_h
