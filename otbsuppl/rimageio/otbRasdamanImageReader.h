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

	NOTE: this is an adjusted version of the otbRasdamanImageReader class 
	      to allow for reading and writing images stored in a rasdaman
				image data base
				
				the only really difference is, that this version doesn't check
				the validity of the 'filename', since this task is delegated 
				to the RasdamanImageIO class, which has to be used together with
				this reader class  

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
#ifndef __otbRasdamanImageReader_h
#define __otbRasdamanImageReader_h

#include "otbImageFileReader.h"
#include "otbAttributeTable.h"
#include "RasdamanConnector.hh"

namespace otb
{

/** \class RasdamanImageReader
 * \brief Resource to read an image from a rasdaman image data base
 *
 * \sa ImageSeriesReader
 * \sa ImageIOBase
 *
 * \ingroup IOFilters
 *
 */
template <class TOutputImage>
class ITK_EXPORT RasdamanImageReader : public otb::ImageFileReader<TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef RasdamanImageReader                     Self;
  typedef otb::ImageFileReader<TOutputImage>  Superclass;
  typedef itk::SmartPointer<Self>             Pointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(RasdamanImageReader, otb::ImageFileReader);

  /** The pixel type of the output image. */
  typedef typename TOutputImage::InternalPixelType OutputImagePixelType;

  /** The size of the output image. */
  typedef typename TOutputImage::SizeType  SizeType;

  /** The region of the output image. */
  typedef typename TOutputImage::RegionType  ImageRegionType;

  /** The pixel type of the output image. */
  //typedef typename TOutputImage::InternalPixelType OutputImagePixelType;

  /** Prepare image allocation at the first call of the pipeline processing */
  virtual void GenerateOutputInformation(void);

  /** Does the real work. */
  virtual void GenerateData();

  /** Give the reader a chance to indicate that it will produce more
   * output than it was requested to produce. RasdamanImageReader cannot
   * currently read a portion of an image (since the ImageIO objects
   * cannot read a portion of an image), so the RasdamanImageReader must
   * enlarge the RequestedRegion to the size of the image on disk. */
  virtual void EnlargeOutputRequestedRegion(itk::DataObject *output);

  /** get the RAT from the reader*/
  otb::AttributeTable::Pointer getRasterAttributeTable(int band);

  /** set the rasdaman connector object */
  void SetRasdamanConnector(RasdamanConnector* rascon);


protected:
  RasdamanImageReader();
  virtual ~RasdamanImageReader();
  void PrintSelf(std::ostream& os, itk::Indent indent) const;

  ////unsigned int m_DatasetNumber;

private:

  	  RasdamanConnector* mRasconn;

  /** Test whether the given filename exist and it is readable,
      this is intended to be called before attempting to use
      ImageIO classes for actually reading the file. If the file
      doesn't exist or it is not readable, and exception with an
      approriate message will be thrown. */
  //void TestFileExistanceAndReadability();

  /** Generate the filename (for GDALImageI for example). If filename is a directory, look if is a
    * CEOS product (file "DAT...") In this case, the GdalFileName contain the open image file.
    */
  //bool GetGdalReadImageFileName( const std::string & filename, std::string & GdalFileName );

  RasdamanImageReader(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  std::string m_ExceptionMessage;
  //CurlHelperInterface::Pointer m_Curl;

};


} //namespace otb

#ifndef OTB_MANUAL_INSTANTIATION
#include "otbRasdamanImageReader.txx"
#endif

#endif // __otbRasdamanImageReader_h
