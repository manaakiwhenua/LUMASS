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
 * FlowAccumulationFilter.h
 *
 *  Created on: 13/01/2011
 *      Author: alex
 */

#ifndef FLOWACCUMULATIONFILTER_H_
#define FLOWACCUMULATIONFILTER_H_

#include "itkImageToImageFilter.h"
#include "itkNeighborhoodAllocator.h"
#include "itkNeighborhoodIterator.h"
#include "itkNeighborhood.h"
#include "nmlog.h"
#include "otbsupplfilters_export.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkImageRegionIteratorWithIndex.h"
// ToDo: check, if really required
//#include "itkConceptChecking.h"

namespace otb {

template <class TInputImage, class TOutputImage>
class OTBSUPPLFILTERS_EXPORT FlowAccumulationFilter
            : public itk::ImageToImageFilter<TInputImage, TOutputImage>
{
public:

  typedef FlowAccumulationFilter							Self;
  typedef itk::ImageToImageFilter<TInputImage, TOutputImage>		Superclass;
  typedef itk::SmartPointer<Self>								Pointer;
  typedef itk::SmartPointer<const Self>							ConstPointer;

  itkNewMacro(Self);
  itkTypeMacro(FlowAccumulationFilter, itk::ImageToImageFilter);

  typedef TInputImage                           InputImageType;
  typedef typename InputImageType::Pointer      InputImagePointer;
  typedef typename InputImageType::RegionType   InputImageRegionType;
  typedef typename InputImageType::PixelType    InputImagePixelType;
  typedef typename InputImageType::IndexType    InputImageIndexType;
  typedef typename InputImageType::SpacingType  InputImageSpacingType;

  typedef TOutputImage                          OutputImageType;
  typedef typename OutputImageType::Pointer     OutputImagePointer;
  typedef typename OutputImageType::RegionType  OutputImageRegionType;
  typedef typename OutputImageType::PixelType   OutputImagePixelType;
  typedef typename OutputImageType::IndexType   OutputImageIndexType;

  typedef typename itk::Image<char, InputImageType::ImageDimension>         ProcImageType;
  typedef typename ProcImageType::Pointer                                   ProcImageTypePointer;
  typedef typename ProcImageType::PixelType                                 ProcImagePixelType;
  typedef typename ProcImageType::IndexType                                 ProcImageIndexType;
  typedef typename ProcImageType::RegionType                                ProcImageRegionType;


  struct HeightList
  {
    long col;
    long row;
    InputImagePixelType z;
  };

  /*! Sets the flow accumulation algorithm. Options are
   *  Dinf          - Tarboton 1997
   *  MFD            - Quin et al. 1991
   *  MFDw(FlowWeight) - Holmgren 1994
   */
  itkSetStringMacro(FlowAccAlgorithm)

  /*! Sets the nodata value of input / output data */
  itkSetMacro(nodata, InputImagePixelType )

  /*! Sets the flow weight used in conjunction with the
   *  Holmgren algorithm
   */
  itkSetMacro(FlowExponent, int)

  /*! If bflowlength is true, the downward flow path length
   *  in map units is calculated for each of the downhill
   *  directions the particular algorithm would have
   *  calculated a flow fraction for. Note: the length
   *  is aportioned but always the full length in either
   *  cardinal or diagonal direction from cell centre to
   *  cell centre.
   *
   */
  void SetFlowLength(bool bflowlength)
  {
      m_bFlowLength = bflowlength;
  }

  void SetFlowLengthUpstream(bool bflowlenup)
  {
      m_bFlowLengthUp = bflowlenup;
  }


protected:
    FlowAccumulationFilter();
    virtual ~FlowAccumulationFilter();

    virtual void EnlargeOutputRequestedRegion(itk::DataObject *output);
    virtual void GenerateData(void);

    /*!
     * \brief Flow accumulation according to Tarboton 1997
     */
    void TFlowAcc(HeightList* phlSort,
                  InputImagePixelType* ibuf,
                  InputImagePixelType* wbuf,
                  OutputImagePixelType* obuf,
                  const double xps, const double yps,
                  const long ncols, const long nrows, long& pixelcounter);

    /*!
     * \brief Flow accumulation according to Quinn et al. 1991
     */
    void QFlowAcc(HeightList *phlSort,
                  InputImagePixelType *ibuf,
                  InputImagePixelType* wbuf,
                  OutputImagePixelType *obuf,
                  const double xps, const double yps,
                  const long ncols, const long nrows, long &pixelcounter);

    /*!
     * \brief Flow accumulation according to Holmgren 1994
     * \param h exponent used for calculating flow proportion fi
     *          for cell i: fi = tan(beta)_i^h / sum_i(tan(beta)_i^h)
     *
     */
    void HFlowAcc(HeightList *phlSort,
                  InputImagePixelType *ibuf,
                  InputImagePixelType* wbuf,
                  OutputImagePixelType *obuf,
                  const double xps, const double yps,
                  const long ncols, const long nrows, long &pixelcounter);

    /*!
     * \brief sortHeightList sorts the input DEM according to descending height
     *        while keeping track of the original position of each pixel; the
     *        sortHeightListAsc variant of this method sorts the height list
     *        according to ascending height values.
     */
    void sortHeightList(HeightList* SortHeight, long links, long rechts, long& counter);
    void sortHeightListAsc(HeightList *SortHeight, long links, long rechts, long &counter);

    void getNeighbourIndex(const int& neigpos, const int& colx, const int& rowx, int& nidx);
    void getNeighbourDistance(const int& neigpos, double& ndist);
    int convert2DTo1DIdx(const int& col, const int& row);

private:
    double m_xdist;
    double m_ydist;
    double m_ddist;
    long m_numpixel;
    int m_FlowExponent;
    bool m_bFlowLength;
    bool m_bFlowLengthUp;

    long m_NumCols;
    long m_NumRows;

    InputImagePixelType m_nodata;

    std::string m_FlowAccAlgorithm;

};

template <class TInputImage, class TOutputImage>
inline int FlowAccumulationFilter<TInputImage, TOutputImage>
::convert2DTo1DIdx(const int &col, const int &row)
{
    return (row * m_NumCols + col);
}

template <class TInputImage, class TOutputImage>
inline void FlowAccumulationFilter<TInputImage, TOutputImage>
::getNeighbourDistance(const int& neigpos, double& ndist)
{
    ndist = 0;
    switch(neigpos)
    {
    case 0 : //Submatrix-Zelle mit der Index-Nummer Null!
        ndist = m_ddist;
        break;
    case 1 :
        ndist = m_ydist;
        break;
    case 2 :
        ndist = m_ddist;
        break;
    case 3 :
        ndist = m_xdist;
        break;
    case 4 :
        ndist = m_ddist;
        break;
    case 5 :
        ndist = m_ydist;
        break;
    case 6 :
        ndist = m_ddist;
        break;
    case 7 :
        ndist = m_xdist;
        break;
    default :
        break;
    }
}

template <class TInputImage, class TOutputImage>
inline void FlowAccumulationFilter<TInputImage, TOutputImage>
::getNeighbourIndex(const int& neigpos, const int& colx, const int& rowx, int& nidx)
{
    nidx = 0;
    switch(neigpos)
    {
    case 0 : //Submatrix-Zelle mit der Index-Nummer Null!
        nidx = (rowx-1) * m_NumCols + (colx-1);
        break;
    case 1 :
        nidx = (rowx-1) * m_NumCols + colx;
        break;
    case 2 :
        nidx = (rowx-1) * m_NumCols + (colx+1);
        break;
    case 3 :
        nidx = rowx * m_NumCols + (colx+1);
        break;
    case 4 :
        nidx = (rowx+1) * m_NumCols + (colx+1);
        break;
    case 5 :
        nidx = (rowx+1) * m_NumCols + colx;
        break;
    case 6 :
        nidx = (rowx+1) * m_NumCols + (colx-1);
        break;
    case 7 :
        nidx = rowx * m_NumCols + (colx-1);
        break;
    default :
        break;
    }
}


template <class TInputImage, class TOutputImage>
inline void FlowAccumulationFilter<TInputImage, TOutputImage>
::sortHeightListAsc(HeightList *SortHeight, long links, long rechts, long &counter)
{
    long li, re;
    long col, row, mcol, mrow;
    InputImagePixelType h, mh;
    long mitte;

    li = links;
    re = rechts;
    mitte = (links + rechts) / (int) 2;   //bewusste IntegerDivision --> ganzzahliger Index mitte

    mh = SortHeight[mitte].z;
    mcol = SortHeight[mitte].col;
    mrow = SortHeight[mitte].row;

    do
    {
        while (SortHeight[li].z < mh)
        {
            ++li;
        }
        while (SortHeight[re].z > mh)
        {
            --re;
        }
        if (li <= re)
        {
            h	= SortHeight[li].z;
            col = SortHeight[li].col;
            row = SortHeight[li].row;

            SortHeight[li].z	= SortHeight[re].z;
            SortHeight[li].col	= SortHeight[re].col;
            SortHeight[li].row	= SortHeight[re].row;

            SortHeight[re].z	= h;
            SortHeight[re].col	= col;
            SortHeight[re].row	= row;

            ++li;
            --re;
            ++counter;
        }

    } while (li <= re);
    this->UpdateProgress((float)counter/(float)m_numpixel);
    if (links < re)  this->sortHeightListAsc(SortHeight, links, re, counter);
    if (rechts > li) this->sortHeightListAsc(SortHeight, li, rechts, counter);
}


template <class TInputImage, class TOutputImage>
inline void FlowAccumulationFilter<TInputImage, TOutputImage>
::sortHeightList(HeightList *SortHeight, long links, long rechts, long &counter)
{
    long li, re;
    long col, row, mcol, mrow;
    InputImagePixelType h, mh;
    long mitte;

    li = links;
    re = rechts;
    mitte = (links + rechts) / (int) 2;   //bewusste IntegerDivision --> ganzzahliger Index mitte

    mh = SortHeight[mitte].z;
    mcol = SortHeight[mitte].col;
    mrow = SortHeight[mitte].row;

    do
    {
        while (SortHeight[li].z > mh)
        {
            li++;
        }
        while (SortHeight[re].z < mh)
        {
            re--;
        }
        if (li <= re)
        {
            h	= SortHeight[li].z;
            col = SortHeight[li].col;
            row = SortHeight[li].row;

            SortHeight[li].z	= SortHeight[re].z;
            SortHeight[li].col	= SortHeight[re].col;
            SortHeight[li].row	= SortHeight[re].row;

            SortHeight[re].z	= h;
            SortHeight[re].col	= col;
            SortHeight[re].row	= row;

            li++;
            re--;
            ++counter;
        }

    } while (li <= re);
    this->UpdateProgress((float)counter/(float)m_numpixel);
    if (links < re)  this->sortHeightList(SortHeight, links, re, counter);
    if (rechts > li) this->sortHeightList(SortHeight, li, rechts, counter);
}


} // end namespace

//#include "otbFlowAccumulationFilter_ExplicitInst.h"

#ifndef ITK_MANUAL_INSTANTIATION
#include "otbFlowAccumulationFilter.txx"
#endif

#endif /* FLOWACCUMULATIONFILTER_H_ */


