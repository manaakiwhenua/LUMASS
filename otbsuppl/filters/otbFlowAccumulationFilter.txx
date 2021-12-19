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
 * FlowAccumulationFilter.txx
 *
 *  Created on: 13/01/2011
 *      Author: alex
 */

#ifndef __otbFlowAccumulationFilter_txx
#define __otbFlowAccumulationFilter_txx

#include <queue>

#include "otbFlowAccumulationFilter.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkNeighborhoodIterator.h"
#include "vnl/vnl_math.h"

namespace otb {

template <class TInputImage, class TOutputImage>
FlowAccumulationFilter<TInputImage, TOutputImage>
::FlowAccumulationFilter()
    : m_xdist(1), m_ydist(1), m_nodata(0), m_FlowExponent(4),
      m_bFlowLength(false)
{
    this->SetNumberOfRequiredInputs(1);
    this->SetNumberOfRequiredOutputs(1);
}

template <class TInputImage, class TOutputImage>
FlowAccumulationFilter<TInputImage, TOutputImage>
::~FlowAccumulationFilter()
{
}


template< typename TInputImage, typename TOutputImage >
void FlowAccumulationFilter< TInputImage, TOutputImage >
::EnlargeOutputRequestedRegion(itk::DataObject *output)
{
    OutputImageType* outImg = dynamic_cast<OutputImageType*>(output);
    if (outImg != nullptr)
    {
        outImg->SetRequestedRegionToLargestPossibleRegion();
    }
}

template <class TInputImage, class TOutputImage>
void FlowAccumulationFilter<TInputImage, TOutputImage>
::QFlowAcc(HeightList *phlSort, InputImagePixelType *ibuf, InputImagePixelType *wbuf, OutputImagePixelType *obuf,
           const double xps, const double yps, const long ncols, const long nrows, long &pixelcounter)
{
    //HQFlowAcc calculates the flow accumulation based on a surface elevation grid
    //using the algorithm presented by QUINN et al. (1991)
    //this algorithm is a so called "multiple flow" algorithm, that forwards the flow
    //of the processing cell to each neighboring cell with lower elevation value

    double nenner;				//denominator of the flowacc formula
    double flaccx;				//flowacc value of the forwarding cell (i.e. processing cell)
    int i;				//index indicating the cells of the moving window (without the
                                //the processing cell (central cell)
    long col, row;		//loop variables indicating the current position in the grid
                                //by column (col) and row
    long colx;			//source (original) column of a "sorted" elevation value from the HeightList
    long rowx;			//source (original) row of a "sorted" elevation value from the HeightList
    double zx;					//elevation of the processing cell
    double zaehler[8];	//the numerator values for the moving window according to the used formula
    double hoehe[8];	//the elevation data of the moving window (see blow)

    //the indices used to indicate each cell of the moving window
    //x is the processing cell got from the sorted elevation data
    //		0 1 2
    //		7 x 3
    //		6 5 4

    //loop over the sorted elevation data, starting with the highest cell in the raster,
    //get the elevation data of the neighboring cells,
    //calculate the slope to each cell
    //forward a flowacc fraction depending on slope to each cell lower than the processing cell
    for (row = 0; row < nrows; row++)
    {
        for (col = 0; col < ncols; col++)
        {
            const int itidx = this->convert2DTo1DIdx(col, row);

            //assign values to helper variables
            zx = static_cast<double>(phlSort[itidx].z);
            if (zx == static_cast<double>(m_nodata))
            {
                    continue;
                    ++pixelcounter;
            }
            colx = phlSort[itidx].col;
            rowx = phlSort[itidx].row;

            const int zidx = this->convert2DTo1DIdx(colx, rowx);
            flaccx = static_cast<double>(obuf[zidx]); //ppFlowAcc[rowx][colx]);

            //consider only those cells with 8 valid neighbors in order to define the moving window
            if (  ((colx > 0) && (colx < (ncols-1)))  &&  ((rowx > 0)  && (rowx < (nrows-1))) )
            {
                //calculate the numerator and the denominator according to the formula
                // 1. calc slope for lower neighboring cells
                // 2. multiply slope with weight factor depending on direction (0.5 cardinal, 0.354 diagonal)
                //calculate the denominator (i.e. sum of numerators for all lower cells)
                nenner = 0;
                int nidx = -1;
                for (i=0; i < 8; i++)
                {
                    this->getNeighbourIndex(i, colx, rowx, nidx);
                    hoehe[i] = ibuf[nidx];

                    // .......................................................................
                    // CALC numerator and denominator for flow accumulation
                    if (!m_bFlowLength)
                    {
                        //consider only lower cells with valid elevation values
                        if (    hoehe[i] != static_cast<double>(m_nodata)
                            &&  (zx - hoehe[i]) > 0
                           )
                        {
                            //numerator
                            if ((i % 2) == 0)
                            {
                                zaehler[i] =  ( (zx - hoehe[i]) / (yps * sqrt(2)) ) * 0.354 * yps;
                            }
                            else
                            {
                                if (i == 7 || i == 3)
                                {
                                    zaehler[i] =  ( (zx - hoehe[i]) / xps ) * 0.5 * xps;
                                }
                                else
                                {
                                    zaehler[i] =  ( (zx - hoehe[i]) / yps ) * 0.5 * yps;
                                }
                            }

                            //denominator
                            nenner += zaehler[i];
                        }
                    }
                    // .......................................................................
                    // CALC flow length
                    else
                    {
                        if ((m_bFlowLengthUp ? hoehe[i] - zx : zx - hoehe[i]) > 0 && hoehe[i] != static_cast<double>(m_nodata))
                        {
                            const double weight = (wbuf == nullptr ? 1.0 : wbuf[nidx]);

                            double ndist = 1;

                            this->getNeighbourDistance(i, ndist);
                            const double pval = ndist * weight + flaccx;
                            if (obuf[nidx] < pval)
                            {
                                obuf[nidx] = pval;
                            }
                        }
                    }
                }

                // ...............................................................
                // calc flow accumulation fraction for each downslope cell
                if (!m_bFlowLength)
                {
                    //calculate the forwarded flowaccumulation fraction
                    for (i=0; i < 8; i++)
                    {
                        //consider only lower cells
                        if ((zx - hoehe[i]) > 0 && hoehe[i] != static_cast<double>(m_nodata))
                        {
                            this->getNeighbourIndex(i, colx, rowx, nidx);
                            const double weight = (wbuf == nullptr ? 1.0 : wbuf[nidx]);
                            obuf[nidx] += static_cast<OutputImagePixelType>(((zaehler[i]/nenner) * flaccx) * weight);
                        }
                    }
                }
            }
            ++pixelcounter;
        }
        this->UpdateProgress((float)pixelcounter/(float)(m_numpixel));
    }
}

template <class TInputImage, class TOutputImage>
void FlowAccumulationFilter<TInputImage, TOutputImage>
::HFlowAcc(HeightList *phlSort, InputImagePixelType *ibuf, InputImagePixelType *wbuf, OutputImagePixelType *obuf,
           const double xps, const double yps, const long ncols, const long nrows, long &pixelcounter)
{
    //calculates the flow accumulation based on a surface elevation grid
    //using the algorithm presented by Holmgren 1994
    //this algorithm is a so called "multiple flow" algorithm, that forwards the flow
    //of the processing cell to each neighboring cell with lower elevation value

    double nenner;				//denominator of the flowacc formula
    double flaccx;				//flowacc value of the forwarding cell (i.e. processing cell)
    int i;				//index indicating the cells of the moving window (without the
                                //the processing cell (central cell)
    long col, row;		//loop variables indicating the current position in the grid
                                //by column (col) and row
    long colx;			//source (original) column of a "sorted" elevation value from the HeightList
    long rowx;			//source (original) row of a "sorted" elevation value from the HeightList
    double zx;					//elevation of the processing cell
    double zaehler[8];	//the numerator values for the moving window according to the used formula
    double hoehe[8];	//the elevation data of the moving window (see blow)

    const double diags = sqrt(xps*xps+yps*yps);

    //the indices used to indicate each cell of the moving window
    //x is the processing cell got from the sorted elevation data
    //		0 1 2
    //		7 x 3
    //		6 5 4

    //loop over the sorted elevation data, starting with the highest cell in the raster,
    //get the elevation data of the neighboring cells,
    //calculate the slope to each cell
    //forward a flowacc fraction depending on slope to each cell lower than the processing cell
    for (row = 0; row < nrows; row++)
    {
        for (col = 0; col < ncols; col++)
        {
            const int itidx = this->convert2DTo1DIdx(col, row);

            //assign values to helper variables
            zx = static_cast<double>(phlSort[itidx].z);
            if (zx == static_cast<double>(m_nodata))
            {
                    continue;
                    ++pixelcounter;
            }
            colx = phlSort[itidx].col;
            rowx = phlSort[itidx].row;

            const int zidx = this->convert2DTo1DIdx(colx, rowx);
            flaccx = static_cast<double>(obuf[zidx]); //ppFlowAcc[rowx][colx]);

            //consider only those cells with 8 valid neighbors in order to define the moving window
            if (  ((colx > 0) && (colx < (ncols-1)))  &&  ((rowx > 0)  && (rowx < (nrows-1))) )
            {
                //calculate the numerator and the denominator according to the formula
                // 1. calc slope for lower neighboring cells
                // 2. calculate the denominator (i.e. sum of numerators for all lower cells)
                nenner = 0;
                int nidx = -1;
                for (i=0; i < 8; i++)
                {
                    this->getNeighbourIndex(i, colx, rowx, nidx);
                    hoehe[i] = ibuf[nidx];

                    // .......................................................................
                    // CALC numerator and denominator for flow accumulation
                    if (!m_bFlowLength)
                    {
                        //consider only lower cells with valid elevation values
                        if (    hoehe[i] != static_cast<double>(m_nodata)
                            &&  (zx - hoehe[i]) > 0 //(m_bFlowLengthUp ? hoehe[i] - zx > 0 : zx - hoehe[i]) > 0
                           )
                        {
                            //numerator
                            if ((i % 2) == 0)
                            {
                                zaehler[i] =  pow(((zx - hoehe[i]) / diags), m_FlowExponent);
                            }
                            else
                            {
                                if (i == 7 || i == 3)
                                {
                                    zaehler[i] =  pow(( (zx - hoehe[i]) / xps ), m_FlowExponent);
                                }
                                else
                                {
                                    zaehler[i] =  pow(( (zx - hoehe[i]) / yps ), m_FlowExponent);
                                }
                            }

                            //denominator
                            if (!std::isnan(zaehler[i]) && !std::isinf(zaehler[i]))
                            {
                                nenner += zaehler[i];
                            }
                            else
                            {
                                zaehler[i] = 0;
                            }
                        }
                        else
                        {
                            zaehler[i] = 0;
                        }
                    }
                    // .......................................................................
                    // CALC flow length
                    else
                    {
                        if ((m_bFlowLengthUp ? hoehe[i] - zx : zx - hoehe[i]) > 0 && hoehe[i] != static_cast<double>(m_nodata))
                        {
                            const double weight = (wbuf == nullptr ? 1.0 : wbuf[nidx]);

                            double ndist = 1;

                            this->getNeighbourDistance(i, ndist);
                            const double pval = ndist * weight + flaccx;
                            if (obuf[nidx] < pval)
                            {
                                obuf[nidx] = pval;
                            }
                        }
                    }
                }

                // ...............................................................
                // calc flow accumulation fraction for each downslope cell
                if (!m_bFlowLength)
                {
                    //calculate the forwarded flowaccumulation fraction
                    for (i=0; i < 8; i++)
                    {
                        //consider only lower cells
                        if (zaehler[i] > 0 && nenner > 0)//(zx - hoehe[i]) > 0 && hoehe[i] != static_cast<double>(m_nodata))
                        {
                            this->getNeighbourIndex(i, colx, rowx, nidx);
                            const double weight = (wbuf == nullptr ? 1.0 : wbuf[nidx]);
                            obuf[nidx] += static_cast<OutputImagePixelType>((zaehler[i]/nenner) * flaccx * weight);
                        }
                    }
                }
            }
            ++pixelcounter;
        }
        this->UpdateProgress((float)pixelcounter/(float)(m_numpixel));
    }
}


template <class TInputImage, class TOutputImage>
void FlowAccumulationFilter<TInputImage, TOutputImage>
::TFlowAcc(HeightList *phlSort, InputImagePixelType *ibuf,
           InputImagePixelType* wbuf, OutputImagePixelType *obuf,
           const double xps, const double yps, const long ncols, const long nrows, long &pixelcounter)
{
    //Variablen deklarieren
    const double Pi = 3.1415926535897932384626433832795;
    const double toDegree = 180/Pi;
    double rx, zx, fx;	//Neigungsrichtung
    int i;			//Schleifenvariablen
    double r[8], s[8], ac[8], af[8], z[8], e1[8], e2[8];
    double s1i, s2i;
    long colx, rowx;
    long col, row;
    double simax, simin;
    int sineg, sineq, simaxindex, siminindex;
    double alpha1, alpha2, p1, p2;
    long counter;
    double nodata = static_cast<double>(m_nodata);

    //Durch die sortierte Höhenmatrix iterieren,
    //eine 3*3 Submatrix bilden und dann die Berechnung für die 8 einzelenen Facetten
    //(Dreiecke) durchführen
    for (row=0; row < nrows; row++)
    {
        for (col=0; col < ncols; col++)
        {
            const int  itidx = this->convert2DTo1DIdx(col, row);

            //ein par Variablen initialisieren
            zx	 = static_cast<double>(phlSort[itidx].z);
            if (zx == static_cast<double>(m_nodata))
            {
                ++pixelcounter;
                continue;
            }
            colx = phlSort[itidx].col;
            rowx = phlSort[itidx].row;

            const int zidx = this->convert2DTo1DIdx(colx, rowx);
            fx   = static_cast<double>(obuf[zidx]);

            //Berechnung nur für "innere" Pixel durchführen (solche, zu denen eine 3*3 Submatrix
            //definiert werden kann) --> Randpixel werden ausgelassen.
            if (  ((colx > 0) && (colx < (ncols-1)))  &&  ((rowx > 0)  && (rowx < (nrows-1))) )
            {
                //Höhenwerte der 3*3 Submatrix abfragen
                z[0] = static_cast<double>(ibuf[(rowx-1) * ncols + (colx-1)]);
                if (z[0] == nodata) {obuf[(rowx-1) * ncols + (colx-1)] = static_cast<OutputImagePixelType>(nodata); continue;}
                z[1] = static_cast<double>(ibuf[(rowx-1) * ncols + colx]);
                if (z[1] == nodata) {obuf[(rowx-1) * ncols + colx] = static_cast<OutputImagePixelType>(nodata); continue;}
                z[2] = static_cast<double>(ibuf[(rowx-1) * ncols + (colx+1)]);
                if (z[2] == nodata) {obuf[(rowx-1) * ncols + (colx+1)] = static_cast<OutputImagePixelType>(nodata); continue;}

                z[7] = static_cast<double>(ibuf[rowx * ncols + (colx - 1)]);
                if (z[7] == nodata) {obuf[rowx * ncols + (colx - 1)] = static_cast<OutputImagePixelType>(nodata); continue;}
                z[3] = static_cast<double>(ibuf[rowx * ncols + (colx + 1)]);
                if (z[3] == nodata) {obuf[rowx * ncols + (colx + 1)] = static_cast<OutputImagePixelType>(nodata); continue;}

                z[6] = static_cast<double>(ibuf[(rowx+1) * ncols + (colx-1)]);
                if (z[6] == nodata) {obuf[(rowx+1) * ncols + (colx-1)] = static_cast<OutputImagePixelType>(nodata); continue;}
                z[5] = static_cast<double>(ibuf[(rowx+1) * ncols + colx]);
                if (z[5] == nodata) {obuf[(rowx+1) * ncols + colx] = static_cast<OutputImagePixelType>(nodata); continue;}
                z[4] = static_cast<double>(ibuf[(rowx+1) * ncols + (colx-1)]);
                if (z[4] == nodata) {obuf[(rowx+1) * ncols + (colx-1)] = static_cast<OutputImagePixelType>(nodata); continue;}

                //Arrays für versch. Facettenwerte füllen
                e1[0]=z[3];e1[1]=z[1];e1[2]=z[1];e1[3]=z[7];e1[4]=z[7];e1[5]=z[5];e1[6]=z[5];e1[7]=z[3];
                e2[0]=z[2];e2[1]=z[2];e2[2]=z[0];e2[3]=z[0];e2[4]=z[6];e2[5]=z[6];e2[6]=z[4];e2[7]=z[4];
                ac[0]=0;ac[1]=1;ac[2]=1;ac[3]=2;ac[4]=2;ac[5]=3;ac[6]=3;ac[7]=4;
                af[0]=1;af[1]=(-1);af[2]=1;af[3]=(-1);af[4]=1;af[5]=(-1);af[6]=1;af[7]=(-1);

                //Berechnung der SubFaktoren und -Vorfaktoren zur Berechnung des endgültigen
                //Wertes für die zentrale Zelle

                for (i=0; i < 8; i++)
                {
                    s1i = (zx - e1[i]) / xps; //cellsize;
                    s2i = (e1[i] - e2[i]) / yps; //cellsize;
                    r[i] = atan2(s2i,s1i);
                    if (r[i] < 0)
                    {
                        r[i] = 0;
                        s[i] = s1i;
                    }
                    else if ( r[i] > (atan2(yps,xps)) )
                    {
                        r[i] = atan2(yps,xps);
                        s[i] = (zx - e2[i]) / pow(((yps*yps)+(xps*xps)),0.5);
                    }
                    else
                        s[i] = pow(((s1i*s1i) + (s2i*s2i)),0.5);
                }

                //maximale Neigung der 8 Facetten ermitteln
                //und für diese Facette dann den flow direction angle rx berechnen
                simax = 0;
                simin = 0;
                sineg = 0;
                sineq = 0;
                simaxindex = 0;
                siminindex = 0;
                for (i=0; i < 8; i++)
                {
                    if (s[i] > simax)
                    {
                        simax = s[i];
                        simaxindex = i;
                    }

                    if (s[i] < simin)
                    {
                        simin = s[i];
                        siminindex = i;
                    }

                    //Wenn die neigung negativ ist, wird sineg um 1 erhöht
                    if (s[i] < 0)
                        sineg++;

                    //Wenn die Neigung für i gleich der Neigung für i-1 ist, dann
                    //wird sineq um 1 erhöht
                    if (i > 0)
                        if (s[i] == s[i-1])
                            sineq++;

                }


                //Bei diagonaler und cardinaler Richtung wird FlowAcc nur an eine Zelle weitergegeben
                if (m_bFlowLength && m_bFlowLengthUp)
                {
                    rx = (af[siminindex] * r[siminindex]) + (ac[siminindex] * (Pi / 2.));
                }
                else
                {
                    rx = (af[simaxindex] * r[simaxindex]) + (ac[simaxindex] * (Pi / 2.));
                }

                int n1idx=-1, n2idx=-1;
                double n1dist=0, n2dist=0;
                int cardia = 0; // 0: cardinal/diagonal, 1: car-dia, 2: dia-car

                if (itk::Math::FloatAlmostEqual(rx, 0.))					//Zelle 3
                    n1idx = 3; //obuf[rowx*ncols+(colx+1)] += (m_bFlowLength ? m_xdist + fx : fx) * (wbuf == nullptr ? 1 : wbuf[rowx*ncols+(colx+1)]);
                else if (itk::Math::FloatAlmostEqual(rx, (Pi/4.)))       //Zelle 2
                    n1idx = 2; //obuf[(rowx-1)*ncols+(colx+1)] +=(m_bFlowLength ? m_ddist + fx : fx) * (wbuf == nullptr ? 1 : wbuf[(rowx-1)*ncols+(colx+1)]);

                else if (itk::Math::FloatAlmostEqual(rx, (Pi/2.)))		//Zelle 1
                    n1idx = 1; //obuf[(rowx-1)*ncols+colx] += (m_bFlowLength ? m_ydist + fx : fx) * (wbuf == nullptr ? 1 : wbuf[(rowx-1)*ncols+colx]);

                else if (itk::Math::FloatAlmostEqual(rx, ((3*Pi)/4.)))	//Zelle 0
                    n1idx = 0; //obuf[(rowx-1)*ncols+(colx-1)] += (m_bFlowLength ? m_ddist + fx : fx) * (wbuf == nullptr ? 1 : wbuf[(rowx-1)*ncols+(colx-1)]);

                else if (itk::Math::FloatAlmostEqual(rx, Pi))			//Zelle 7
                    n1idx = 7; //obuf[rowx*ncols+(colx-1)] += (m_bFlowLength ? m_xdist + fx : fx) * (wbuf == nullptr ? 1 : wbuf[rowx*ncols+(colx-1)]);

                else if (itk::Math::FloatAlmostEqual(rx, ((5*Pi)/4.)))	//Zelle 6
                    n1idx = 6; //obuf[(rowx+1)*ncols+(colx-1)] += (m_bFlowLength ? m_ddist + fx : fx) * (wbuf == nullptr ? 1 : wbuf[(rowx+1)*ncols+(colx-1)]);

                else if (itk::Math::FloatAlmostEqual(rx, ((3*Pi)/2.)))	//Zelle 5
                    n1idx = 5; //obuf[(rowx+1) * ncols + colx] += (m_bFlowLength ? m_ydist + fx : fx) * (wbuf == nullptr ? 1 : wbuf[(rowx+1) * ncols + colx]);

                else if	(itk::Math::FloatAlmostEqual(rx, ((7*Pi)/4.)))	//Zelle 4
                    n1idx = 4; //obuf[(rowx+1)*ncols+(colx+1)] += (m_bFlowLength ? m_ddist + fx : fx) * (wbuf == nullptr ? 1 : wbuf[(rowx+1)*ncols+(colx+1)]);

                else						  //für nicht cardinale oder diagonale Richtungen
                {
                    if (m_bFlowLength && m_bFlowLengthUp)
                    {
                        alpha2 = r[siminindex];
                    }
                    else
                    {
                        alpha2 = r[simaxindex];
                    }

                    alpha1 = (Pi / 4.) - alpha2;
                    p1 = alpha1 / (double)(alpha1 + alpha2);
                    p2 = alpha2 / (double)(alpha1 + alpha2);

                    //allow only positive fractions
                    if (p1 < 0) p1 = 0;
                    if (p2 < 0) p2 = 0;

                    const int sidx = m_bFlowLength && m_bFlowLengthUp ? siminindex : simaxindex;

                    if (sidx == 0)
                    {
                        n1idx = 3; //obuf[rowx * ncols + (colx+1)] += (m_bFlowLength ? m_xdist + fx :  fxp1) * (wbuf == nullptr ? 1 : wbuf[rowx * ncols + (colx+1)]);		//Zelle 3
                        n2idx = 2; //obuf[(rowx-1) * ncols + (colx+1)] += (m_bFlowLength ? m_ddist + fx :  fxp2) * (wbuf == nullptr ? 1 : wbuf[(rowx-1) * ncols + (colx+1)]);	//Zelle 2
                        cardia = 1;
                    }
                    else if (sidx == 1)
                    {
                        n1idx = 2; //obuf[(rowx-1) * ncols + (colx+1)] += (m_bFlowLength ? m_ddist + fx :  fxp2) * (wbuf == nullptr ? 1 : wbuf[(rowx-1) * ncols + (colx+1)]);	//Zelle 2
                        n2idx = 1; //obuf[(rowx-1) * ncols + colx] += (m_bFlowLength ? m_ydist + fx :  fxp1) * (wbuf == nullptr ? 1 : wbuf[(rowx-1) * ncols + colx]);		//Zelle 1
                        cardia = 2;
                    }
                    else if (sidx == 2)
                    {
                        n1idx = 1; //obuf[(rowx-1) * ncols + colx] += (m_bFlowLength ? m_ydist + fx :  fxp1) * (wbuf == nullptr ? 1 : wbuf[(rowx-1) * ncols + colx]);		//Zelle 1
                        n2idx = 0; //obuf[(rowx-1) * ncols + (colx-1)] += (m_bFlowLength ? m_ddist + fx :  fxp2) * (wbuf == nullptr ? 1 : wbuf[(rowx-1) * ncols + (colx-1)]);	//Zelle 0
                        cardia = 1;
                    }
                    else if (sidx == 3)
                    {
                        n1idx = 0; //obuf[(rowx-1) * ncols + (colx-1)] += (m_bFlowLength ? m_ddist + fx :  fxp2) * (wbuf == nullptr ? 1 : wbuf[(rowx-1) * ncols + (colx-1)]);	//Zelle 0
                        n2idx = 7; //obuf[rowx * ncols + (colx-1)] += (m_bFlowLength ? m_xdist + fx :  fxp1) * (wbuf == nullptr ? 1 : wbuf[rowx * ncols + (colx-1)]);		//Zelle 7
                        cardia = 2;
                    }
                    else if (sidx == 4)
                    {
                        n1idx = 7; //obuf[rowx * ncols + (colx-1)] += (m_bFlowLength ? m_xdist + fx :  fxp1) * (wbuf == nullptr ? 1 : wbuf[rowx * ncols + (colx-1)]);		//Zelle 7
                        n2idx = 6; //obuf[(rowx + 1) * ncols + (colx-1)] += (m_bFlowLength ? m_ddist + fx :  fxp2) * (wbuf == nullptr ? 1 : wbuf[(rowx + 1) * ncols + (colx-1)]);	//Zelle 6
                        cardia = 1;
                    }
                    else if (sidx == 5)
                    {
                        n1idx = 6; //obuf[(rowx+1) * ncols + (colx-1)] += (m_bFlowLength ? m_ddist + fx :  fxp2) * (wbuf == nullptr ? 1 : wbuf[(rowx+1) * ncols + (colx-1)]);	//Zelle 6
                        n2idx = 5; //obuf[(rowx+1) * ncols + colx] += (m_bFlowLength ? m_ydist + fx :  fxp1) * (wbuf == nullptr ? 1 : wbuf[(rowx+1) * ncols + colx]);		//Zelle 5
                        cardia = 2;
                    }
                    else if (sidx == 6)
                    {
                        n1idx = 5; //obuf[(rowx+1) * ncols + colx] += (m_bFlowLength ? m_ydist + fx :  fxp1) * (wbuf == nullptr ? 1 : wbuf[(rowx+1) * ncols + colx]);		//Zelle 5
                        n2idx = 4; //obuf[(rowx+1) * ncols + (colx+1)] += (m_bFlowLength ? m_ddist + fx :  fxp2) * (wbuf == nullptr ? 1 : wbuf[(rowx+1) * ncols + (colx+1)]);	//Zelle 4
                        cardia = 1;
                    }
                    else if (sidx == 7)
                    {
                        n1idx = 4; //obuf[(rowx+1) * ncols + (colx+1)] += (m_bFlowLength ? m_ddist + fx :  fxp2) * (wbuf == nullptr ? 1 : wbuf[(rowx+1) * ncols + (colx+1)]);	//Zelle 4
                        n2idx = 3; //obuf[rowx * ncols + (colx+1)] += (m_bFlowLength ? m_xdist + fx :  fxp1) * (wbuf == nullptr ? 1 : wbuf[rowx * ncols + (colx+1)]);		//Zelle 3
                        cardia = 2;
                    }
                }

                const double fxp1 = fx * p1;
                const double fxp2 = fx * p2;

                int n11didx, n21didx;
                this->getNeighbourIndex(n1idx, colx, rowx, n11didx);
                double w1val = wbuf == nullptr ? 1.0 : static_cast<double>(wbuf[n11didx]);

                // ...................................................................
                // calc flow length

                if (m_bFlowLength)
                {
                    if (    (m_bFlowLengthUp ? z[n1idx] - zx : zx - z[n1idx]) > 0
                         && z[n1idx] != static_cast<double>(m_nodata)
                       )
                    {
                        this->getNeighbourDistance(n1idx, n1dist);
                        if (static_cast<double>(obuf[n11didx]) < (n1dist * w1val + fx))
                        {
                            obuf[n11didx] = static_cast<OutputImagePixelType>(n1dist * w1val + fx);
                        }
                    }

                    if (    n2idx != -1
                         && (m_bFlowLengthUp ? z[n2idx] - zx : zx - z[n2idx]) > 0
                         && z[n2idx] != static_cast<double>(m_nodata)
                       )
                    {
                        this->getNeighbourIndex(n2idx, colx, rowx, n21didx);
                        this->getNeighbourDistance(n2idx, n2dist);
                        double w2val = wbuf == nullptr ? 1.0 : static_cast<double>(wbuf[n21didx]);
                        if (static_cast<double>(obuf[n21didx]) < (n2dist * w2val + fx))
                        {
                            obuf[n21didx] = static_cast<OutputImagePixelType>(n2dist * w2val + fx);
                        }
                    }
                }

                // ...................................................................
                // calc flow accumulation

                else
                {
                    if (cardia == 0)
                    {
                        obuf[n11didx] += fx * w1val;
                    }
                    else
                    {
                        this->getNeighbourIndex(n2idx, colx, rowx, n21didx);
                        double w2val = wbuf == nullptr ? 1.0 : static_cast<double>(wbuf[n21didx]);

                        if (cardia == 1)
                        {
                            obuf[n11didx] += static_cast<OutputImagePixelType>(fxp1 * w1val);
                            obuf[n21didx] += static_cast<OutputImagePixelType>(fxp2 * w2val);
                        }
                        else //if (cardia == 2)
                        {
                            obuf[n11didx] += static_cast<OutputImagePixelType>(fxp2 * w1val);
                            obuf[n21didx] += static_cast<OutputImagePixelType>(fxp1 * w2val);
                        }
                    }
                }
            }
            ++pixelcounter;
        }
        //step the progressbar
        this->UpdateProgress((float)pixelcounter/(float)(m_numpixel));
    }
}

template <class TInputImage, class TOutputImage>
void FlowAccumulationFilter<TInputImage, TOutputImage>
::GenerateData(void)
{
    NMProcDebug(<< "Enter FlowAcc::GenerateData" << std::endl);

    // get a handle of the in- and output data
    InputImagePointer pInImg = nullptr;
    InputImagePointer pWeightImg = nullptr;

    // try fetching inputs by name first
    itk::ProcessObject::NameArray inputNames = this->GetInputNames();
    for (int n=0; n < inputNames.size(); ++n)
    {
        if (inputNames[n].find("dem") != std::string::npos)
        {
            pInImg = dynamic_cast<InputImageType*>(this->GetInputs()[n].GetPointer());
        }

        if (inputNames[n].find("weight") != std::string::npos)
        {
            pWeightImg = dynamic_cast<InputImageType*>(this->GetInputs()[n].GetPointer());
        }
    }

    // try fetching by index if names didn't work
    if (pInImg == nullptr)
    {
        pInImg = const_cast<InputImageType*>(this->GetInput(0));
    }

    if (this->GetNumberOfIndexedInputs() >= 2 && pWeightImg == nullptr)
    {
        pWeightImg = const_cast<InputImageType*>(this->GetInput(1));
    }

    // give up if there's no input dem
    if (pInImg == nullptr)
    {
        itkExceptionMacro(<< "No valid input DEM specified!");
    }


    OutputImagePointer pOutImg = this->GetOutput();
    pOutImg->SetBufferedRegion(pOutImg->GetRequestedRegion());
    pOutImg->Allocate();
    if (m_bFlowLength)
    {
        pOutImg->FillBuffer(0);
    }
    else
    {
        pOutImg->FillBuffer(1);
    }


    // get pixel size in x and y direction
    typename TInputImage::SpacingType spacing = pInImg->GetSpacing();
    m_xdist = spacing[0];
    m_ydist = abs(spacing[1]);
    m_ddist = sqrt(m_xdist * m_xdist + m_ydist * m_ydist);

    // get the number of pixels
    long numcols = pInImg->GetRequestedRegion().GetSize(0);
    long numrows = pInImg->GetRequestedRegion().GetSize(1);
    long numpix = numcols * numrows;
    m_numpixel = numpix * 2 + (numpix * std::log10(numpix));
    m_NumCols = numcols;
    m_NumRows = numrows;

    NMProcDebug(<< " cellsize is: " << spacing[0] << ", " << spacing[1] << std::endl);
    NMProcDebug(<< "  m_xdist: " << m_xdist << ", m_ydist: " << m_ydist << std::endl);
    NMProcDebug(<< "  numcols: " << numcols << ", numrows: " << numrows <<
            ", numpix: " << numpix << std::endl);

    // -----------------------------------------------------------------
    // read the input dem into height structre
    HeightList* hlDemSort = new HeightList[numpix];
    InputImagePixelType* ibuf = pInImg->GetBufferPointer();
    OutputImagePixelType* obuf = pOutImg->GetBufferPointer();
    InputImagePixelType* wbuf = nullptr;
    if (pWeightImg.IsNotNull())
    {
        wbuf = pWeightImg->GetBufferPointer();
    }

    long pixelcounter = 1;
    long maxpixcount = numpix * 3;
    for (long row=0; row < numrows; ++row)
    {
        for (long col=0; col < numcols; ++col)
        {
            const long idx = row * numcols + col;
            hlDemSort[idx].col = col;
            hlDemSort[idx].row = row;
            hlDemSort[idx].z   = ibuf[idx];
            ++pixelcounter;
        }
        this->UpdateProgress((float)pixelcounter/(float)(m_numpixel));
    }

    // -----------------------------------------------------------------
    // read the input dem into height structre
    // sort the height list
    //this->UpdateProgress(0.0);
    NMProcDebug( << "sorting ...");
    if (m_bFlowLength && m_bFlowLengthUp)
    {
        this->sortHeightListAsc(hlDemSort, 0, numpix-1, pixelcounter);
    }
    else
    {
        this->sortHeightList(hlDemSort, 0, numpix-1, pixelcounter);
    }
    this->UpdateProgress((float)2/3.0);
    m_numpixel = 3 * numpix;
    pixelcounter = 2 * numpix;

    // ---------------------------
    // compute flow acc
    NMProcDebug( << "flowacc ...");

    if (m_FlowAccAlgorithm.compare("Dinf") == 0)
    {
        this->TFlowAcc(hlDemSort, ibuf, wbuf, obuf, spacing[0], spacing[1],
                   numcols, numrows, pixelcounter);
    }
    else if (m_FlowAccAlgorithm.compare("MFD") == 0)
    {
        this->QFlowAcc(hlDemSort, ibuf, wbuf, obuf, spacing[0], spacing[1],
                   numcols, numrows, pixelcounter);
    }
    else if (m_FlowAccAlgorithm.compare("MFDw") == 0)
    {
        if (m_FlowExponent < 1)
        {
            m_FlowExponent = 1;
        }
        this->HFlowAcc(hlDemSort, ibuf, wbuf, obuf, spacing[0], spacing[1],
                numcols, numrows, pixelcounter);
    }

    NMProcDebug(<< "pixelcounter: " << pixelcounter);
    NMProcDebug(<< "m_numpixel" << m_numpixel);
    NMProcDebug(<< "pix ratio" << (pixelcounter / (float)m_numpixel));

    // -----------------------------------------------------------------
    // clean up
    delete hlDemSort;
    this->UpdateProgress(1.0);

    NMProcDebug(<< "Leave FlowAcc::GenerateData" << std::endl);
}


} // end namespace

#endif
