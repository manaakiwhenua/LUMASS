/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2010-2015 Landcare Research New Zealand Ltd
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

/*  ACKNOWLEDGEMENT
 *
 *  This implementation is based on a conceptual algorithm for
 *  computing unique combinations developed by my brillant colleague
 *
 *  Robbie Price (Manaaki Whenua - Landcare Research New Zealand Ltd)
 *
 *
 */

#ifndef OTBCOMBINETWOFILTER_H_
#define OTBCOMBINETWOFILTER_H_

#include <map>
#include <set>
#include <string>

#include "otbSQLiteTable.h"
#include "itkImageToImageFilter.h"
#include "otbImage.h"

#include "nmotbsupplfilters_export.h"

namespace otb
{
/** \class CombineTwoFilter
 *
 */

template< class TInputImage, class TOutputImage = TInputImage >
class NMOTBSUPPLFILTERS_EXPORT CombineTwoFilter
	: public itk::ImageToImageFilter< TInputImage, TOutputImage >
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

	  typedef typename InputImageType::Pointer	InputImagePointerType;
	  typedef typename OutputImageType::Pointer OutputImagePointerType;

	  /** Standard class typedefs. */
          typedef CombineTwoFilter                                            Self;
	  typedef itk::ImageToImageFilter< InputImageType, OutputImageType> Superclass;
	  typedef itk::SmartPointer<Self>                                   Pointer;
	  typedef itk::SmartPointer<const Self>                             ConstPointer;

	  /** Method for creation through the object factory. */
	  itkNewMacro(Self);

	  /** Run-time type information (and related methods). */
          itkTypeMacro(CombineTwoFilter, itk::ImageToImageFilter);

	  /** Image typedef support. */
	  typedef typename InputImageType::PixelType   InputPixelType;
	  typedef typename OutputImageType::PixelType  OutputPixelType;

	  typedef typename InputImageType::RegionType  InputImageRegionType;
	  typedef typename OutputImageType::RegionType OutputImageRegionType;

	  typedef typename InputImageType::SizeType    InputImageSizeType;
	  typedef typename OutputImageType::SizeType   OutputImageSizeType;

	  typedef typename InputImageType::SpacingType  InputImageSpacingType;
	  typedef typename OutputImageType::SpacingType OutputImageSpacingType;

          typedef long long ComboIndexType;

          /*! maps the hyperspace index to the 0-based image index
           *  Map key: hyperspace index
           *  Map value: index of unique combination (0-based)
           */
          typedef typename std::map< ComboIndexType,  ComboIndexType > ComboMapType;
          typedef typename ComboMapType::iterator ComboMapTypeIterator;

          typedef typename std::set< ComboIndexType > ComboTrackerType;
          typedef typename ComboTrackerType::iterator ComboTrackerTypeIterator;

          AttributeTable::Pointer getRAT(unsigned int idx);
          void setRAT(unsigned int idx, AttributeTable::Pointer);

          void SetInputNodata(const std::vector<long long>& inNodata);
          void SetImageNames(const std::vector<std::string>& imgNames);

          ComboIndexType GetNumUniqueCombinations(void) {return m_NumUniqueCombinations-1;}

          virtual void ResetPipeline();

          itkGetMacro(Workspace, std::string);
          itkSetMacro(Workspace, std::string);

          itkGetMacro(OutputTableFileName, std::string);
          void SetOutputTableFileName(const std::string& outtablename)
            {m_OutputTableFileName = outtablename;}

protected:
          CombineTwoFilter();
          virtual ~CombineTwoFilter();
	  virtual void PrintSelf(std::ostream& os, itk::Indent indent) const;

          void BeforeThreadedGenerateData();
          void ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread, itk::ThreadIdType threadId );
          void AfterThreadedGenerateData();
          //void GenerateData();


private:
          CombineTwoFilter(const Self&); //purposely not implemented
	  void operator=(const Self&); //purposely not implemented

          std::string m_Workspace;
          std::string m_OutputTableFileName;
          SQLiteTable::Pointer m_ComboTable;
          bool m_dropTmpDBs;

          bool m_StreamingProc;
          std::vector<InputPixelType> m_InputNodata;
          std::vector<ComboIndexType> m_vHyperSpaceDomains;
          std::vector<ComboIndexType> m_vHyperStrides;
          std::set<ComboIndexType>    m_sComboTracker;
          std::vector<std::string>    m_vColnames;
          std::vector<std::string>    m_ImgNames;

          //std::set<ZoneKeyType> mZones;
          //typename std::vector<ComboMapType> m_ThreadComboStore;
          ComboMapType m_ComboMap;
          std::vector<ComboTrackerType> m_vThreadComboTracker;
          ComboIndexType m_NumUniqueCombinations;
          OutputPixelType m_NodataCount;
          ComboIndexType m_TotalPixCount;
          std::vector<ComboIndexType> m_vThreadPixCount;
          std::vector<AttributeTable::Pointer> m_vRAT;

          static const std::string ctx;


};

} // end namespace otb


template< class TInputImage, class TOutputImage>
const std::string otb::CombineTwoFilter<TInputImage, TOutputImage>::ctx = "otb::CombineTwoFilter";

//#include "otbCombineTwoFilter_ExplicitInst.h"

#ifndef OTB_MANUAL_INSTANTIATION
#include "otbCombineTwoFilter.txx"
#endif

#endif /* OTBCOMBINETWOFILTER_H_ */
