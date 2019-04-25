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
/*
 * DEMSlopeAspectFilter.h
 *
 *  Created on: 13/01/2011
 *      Author: alex
 */

#ifndef DEMSlopeAspectFilter_H_
#define DEMSlopeAspectFilter_H_

#include "itkImageToImageFilter.h"
#include "itkNeighborhoodAllocator.h"
#include "itkNeighborhoodIterator.h"
#include "itkNeighborhood.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"

#include "nmlog.h"
// ToDo: check, if really required
//#include "itkConceptChecking.h"

#include "otbsupplfilters_export.h"

namespace otb {

template <class TInputImage, class TOutputImage=TInputImage >
class OTBSUPPLFILTERS_EXPORT DEMSlopeAspectFilter
			: public itk::ImageToImageFilter<TInputImage, TOutputImage>
{
public:

  typedef DEMSlopeAspectFilter							Self;
  typedef itk::ImageToImageFilter<TInputImage, TOutputImage>		Superclass;
  typedef itk::SmartPointer<Self>								Pointer;
  typedef itk::SmartPointer<const Self>							ConstPointer;

  itkNewMacro(Self);
  itkTypeMacro(DEMSlopeAspectFilter, itk::ImageToImageFilter);

  typedef TInputImage                           InputImageType;
  typedef typename InputImageType::ConstPointer InputImageConstPointer;
  typedef typename InputImageType::RegionType   InputImageRegionType;
  typedef typename InputImageType::PixelType    InputImagePixelType;
  typedef typename InputImageType::IndexType    InputImageIndexType;
  typedef typename InputImageType::SpacingType  InputImageSpacingType;

  using InputImageSizeType = typename InputImageType::SizeType;

  typedef TOutputImage                          OutputImageType;
  typedef typename OutputImageType::Pointer     OutputImagePointer;
  typedef typename OutputImageType::RegionType  OutputImageRegionType;
  typedef typename OutputImageType::PixelType   OutputImagePixelType;
  typedef typename OutputImageType::IndexType   OutputImageIndexType;


  using InputRegionConstIterType = itk::ImageRegionConstIterator<InputImageType>;
  using KernelIterType = itk::ConstNeighborhoodIterator<InputImageType>;
  using RegionIterType = itk::ImageRegionIterator<OutputImageType>;

  typedef enum {TERRAIN_SLOPE, TERRAIN_LS, TERRAIN_WETNESS, TERRAIN_SEDTRANS} TerrainAttribute;
  typedef enum {GRADIENT_DEGREE, GRADIENT_PERCENT, GRADIENT_ASPECT, GRADIENT_DIMLESS} AttributeUnit;
  typedef enum {ALGO_HORN, ALGO_ZEVEN} TerrainAlgorithm;

  typedef itk::NeighborhoodAllocator<InputImagePixelType> NeighborhoodAllocType;
  typedef itk::Neighborhood<InputImagePixelType, InputImageType::ImageDimension, NeighborhoodAllocType> NeighborhoodType;


  /*! supported slope algorithms:
   * Horn           // after Horn 1981
   * Zevenbergen    // after Zevenbergen & Thorne 1987
   */
  itkSetStringMacro( TerrainAlgorithm )

  /*! supported terrain attributes
   * Slope          // slope angle
   * LS             // (R)USLE LS factor (Desmet & Govers)
   * Wetness        // topographic wetness index
   * SedTransport   // Moore & Burch 1986 (cited in Mitasova et al. 1996)
   */
  itkSetStringMacro( TerrainAttribute )

  /*! supported units for slope
   * Dim.less   // tan(angle)
   * Degree     // 0-90
   * Percent    // 0-100
   * Aspect     // 0-360 (north=0 deg., clockwise)
   */
  itkSetStringMacro( AttributeUnit )

  void SetNodata(const double& nodata);
  void SetInputNames(const std::vector<std::string>& inputNames);

  virtual void SetNthInput(itk::DataObject::DataObjectPointerArraySizeType num, itk::DataObject* input);


  InputImageType* GetDEMImage();
  InputImageType* GetFlowAccImage();

protected:
    DEMSlopeAspectFilter();
    virtual ~DEMSlopeAspectFilter();
    //	void PrintSelf(std::ostream& os, itk::Indent indent) const;

    void GenerateInputRequestedRegion() throw ();
    void AfterThreadedGenerateData();
    void BeforeThreadedGenerateData();
    void ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
                           itk::ThreadIdType threadId);

    void Slope(const NeighborhoodType& nh, double* val);
	void dZdX(const NeighborhoodType& nh, double* val);
	void dZdY(const NeighborhoodType& nh, double* val);
    void LS(const NeighborhoodType& nDem, const InputImagePixelType& flowacc,
            double* val);
    void Wetness(const NeighborhoodType& nDem, const InputImagePixelType& flowacc,
                 double* val);
    void SedTrans(const NeighborhoodType& nDem, const InputImagePixelType& flowacc,
                  double* val);


private:
	double m_xdist;
	double m_ydist;
    double m_Nodata;

    long m_Pixcounter;

    std::string m_TerrainAlgorithm;
    std::string m_TerrainAttribute;
    std::string m_AttributeUnit;

    TerrainAlgorithm m_eTerrainAlgorithm;
    TerrainAttribute m_eTerrainAttribute;
    AttributeUnit    m_eAttributeUnit;


    std::vector<std::string> m_IMGNames;
    std::vector<std::string> m_DataNames;

    static const double RtoD;
    static const double DegToRad;
    static const double Pi;

};

} // end namespace

//#include "otbDEMSlopeAspectFilter_ExplicitInst.h"

#ifndef ITK_MANUAL_INSTANTIATION
#include "otbDEMSlopeAspectFilter.txx"
#endif

#endif /* DEMSlopeAspectFilter_H_ */


