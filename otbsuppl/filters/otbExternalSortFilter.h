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
/*
 * ExternalSortFilter.h
 *
 *  Created on: 14/01/2015
 *      Author: alex
 */

#ifndef ExternalSortFilter_H_
#define ExternalSortFilter_H_

#define ctxExternalSortFilter "ExternalSortFilter"

#include "nmlog.h"
#include <string>
#include <fstream>
#include <vector>
#include "itkImageToImageFilter.h"
#include "itkImageRegionIterator.h"
#include "otbNMImageReader.h"
#include "otbStreamingRATImageFileWriter.h"
#include "otbSortFilter.h"
#include "itkNMImageRegionSplitterMaxSize.h"

#include "otbsupplfilters_export.h"

#ifdef BUILD_RASSUPPORT
    #include "RasdamanConnector.hh"
#endif

/*  Sorts an image and any additionally specified 'depending' image accordingly.
 *
 *	This class uses otbSortFilter to sort a large (>RAM) input image #0 either
 *  ascending or descending. Any additionally specified images are sorted according
 *  to the order of input image #0. Furthermore this filter produces and 'IndexImage'
 *  which denotes the original 1D-index (offset) position of input image #0.
 *  The IndexImage is available via GetIndexImage.
 *	This filter processes the input images in RAM concumable chunks, sorts the chunks
 *  using otbSortFilter, and writes the chunks to disk. Finally, it merges the chunks
 *  on disk to one image and writes it out.
 *
 *  inputs: fileName_image0, fileName_image1, ..., fileName_imageN
 *
 */

namespace otb {

template <class TInputImage, class TOutputImage = TInputImage>
class ITK_EXPORT ExternalSortFilter
			: public itk::ImageToImageFilter<TInputImage, TOutputImage>
{
public:

  typedef ExternalSortFilter							Self;
  typedef itk::ImageToImageFilter<TInputImage, TOutputImage>		Superclass;
  typedef itk::SmartPointer<Self>								Pointer;
  typedef itk::SmartPointer<const Self>							ConstPointer;

  itkNewMacro(Self);
  itkTypeMacro(ExternalSortFilter, itk::ImageToImageFilter);

  typedef TInputImage						InputImageType;
  typedef typename InputImageType::Pointer	InputImagePointer;
  typedef typename InputImageType::RegionType InputImageRegionType;
  typedef typename InputImageType::PixelType  InputImagePixelType;
  typedef typename itk::ImageRegionIteratorWithIndex<InputImageType> InputIteratorType;

  typedef TOutputImage						OutputImageType;
  typedef typename OutputImageType::Pointer	OutputImagePointer;
  typedef typename OutputImageType::RegionType OutputImageRegionType;
  typedef typename OutputImageType::PixelType  OutputImagePixelType;

  typedef typename otb::SortFilter<TInputImage, TOutputImage>        SortFilterType;
  typedef typename SortFilterType::IndexImageType                    IndexImageType;

  //typedef typename otb::Image<long, OutputImageType::ImageDimension> IndexImageType;
  typedef typename IndexImageType::Pointer                           IndexImagePointer;
  typedef typename IndexImageType::RegionType						 IndexImageRegionType;
  typedef typename IndexImageType::PixelType                         IndexImagePixelType;
  typedef typename itk::ImageRegionIterator<IndexImageType> IndexIteratorType;

  typedef typename otb::NMImageReader<InputImageType>                ImageReaderType;
  typedef typename ImageReaderType::Pointer                          ImageReaderPointerType;

  typedef typename otb::StreamingRATImageFileWriter<OutputImageType> ImageWriterType;
  typedef typename ImageWriterType::Pointer                          ImageWriterPointerType;
  typedef typename otb::StreamingRATImageFileWriter<IndexImageType>  IndexImageWriterType;
  typedef typename IndexImageWriterType::Pointer                     IndexImageWriterPointerType;

  typedef typename otb::NMImageReader<OutputImageType> ChunkReaderType;
  typedef typename ChunkReaderType::Pointer            ChunkReaderPointerType;
  typedef typename otb::NMImageReader<IndexImageType> IndexReaderType;
  typedef typename IndexReaderType::Pointer           IndexReaderPointerType;



  /** Whether or not sorting should be done in ascending order */
  itkSetMacro(SortAscending, bool)
  itkGetMacro(SortAscending, bool)
  itkBooleanMacro(SortAscending)

  /** Set max size in MiB of image chunks used for processing */
  itkSetMacro(MaxChunkSize, int)
  itkGetMacro(MaxChunkSize, int)

#ifdef BUILD_RASSUPPORT
  /** Set the rasdaman connector object */
  void SetRasdamanConnector(RasdamanConnector* rascon);
#endif

  void SetNthFileName(int idx, std::string filename);

protected:
    ExternalSortFilter();
    virtual ~ExternalSortFilter();
	void PrintSelf(std::ostream& os, itk::Indent indent) const;

	void GenerateData(void);

    void GenerateOutputFileNames(std::vector<std::string>& outputFN);

    std::vector<std::string> PreSortChunks(
                                std::vector<std::string>& outNames,
                                std::vector<ImageReaderPointerType>& readers,
                                std::vector<ImageWriterPointerType>& writers,
                                IndexImageWriterPointerType& idxWriter,
                                InputImageRegionType& lpr,
                                itk::ImageIORegion& wior,
                                int imagechunk
                                );

    bool FineSortChunks(std::vector<std::string>& chunkNames,
                        std::vector<std::string>& outNames,
                        std::vector<ImageWriterPointerType>& writers,
                        IndexImageWriterPointerType& idxWriter,
                        InputImageRegionType& lpr,
                        itk::ImageIORegion& wior,
                        int imagechunk
                        );



    void forwardChunkSplit(
            const int& splitNum,
            const int& maxSplitSplitSize,
            itk::NMImageRegionSplitterMaxSize::Pointer& ssp,
            std::vector< IndexReaderPointerType >&                idxChkReaders,
            std::vector< IndexImagePixelType* >&                  idxChkBuffers,
            std::vector< std::vector< ChunkReaderPointerType > >& imgChkReaders,
            std::vector< std::vector< OutputImagePixelType* > >&  imgChkBuffers,
            std::vector< InputImageRegionType >&   imgChkLPR,
            std::vector< int >&                    imgChkOffset,
            std::vector< int >&                    imgChkLength,
            std::vector< int >&                    imgChkSplitNum
            );

    void forwardOutputSplit(
            int& outputSplit,
            const int& numSplits,
            const int& maxSplitSize,
            itk::NMImageRegionSplitterMaxSize::Pointer& splitter,
            std::vector< ImageWriterPointerType >& writers,
            IndexImageWriterPointerType&           idxWriter,
            IndexImagePixelType* &                  idxOutBuffer,
            std::vector< OutputImagePixelType* >&  outBuffers,
            int&                    outputOffset,
            int&                    outputLength
            );


#ifdef BUILD_RASSUPPORT
    RasdamanConnector* m_Rasconn;
#endif

    bool m_SortAscending;
    std::vector<std::string> m_FileNames;
    std::vector<std::string> m_TmpFileNames;

    std::vector<long> m_RowOffsets;

    int m_MaxChunkSize;


private:

};

} // end namespace

#ifndef ITK_MANUAL_INSTANTIATION
#include "otbExternalSortFilter.txx"
#endif

#endif /* ExternalSortFilter_H_ */


