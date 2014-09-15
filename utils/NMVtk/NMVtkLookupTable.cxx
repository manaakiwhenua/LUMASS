/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLookupTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "NMVtkLookupTable.h"

#include "vtkObjectFactory.h"
#include "vtkBitArray.h"
#include "vtkMathConfigure.h"
#include "vtkMath.h"


bool _mbUseLowerUpperClr = false;
double _mLower;
double _mUpper;
unsigned char _mUpperClr[4];
unsigned char _mLowerClr[4];


template<class T>
void NMVtkLookupTableMapData(NMVtkLookupTable *self, T *input,
    unsigned char *output, int length,
    int inIncr, int outFormat);

template<class T>
void NMVtkLookupTableIndexedMapData(
    NMVtkLookupTable* self, T* input, unsigned char* output, int length,
    int inIncr, int outFormat );

template<class T>
unsigned char *NMVtkLinearLookup(NMVtkLookupTable* self,
  T v, unsigned char *table, double maxIndex, double shift, double scale,
  unsigned char *vtkNotUsed(nanColor));

inline double NMVtkApplyLogScale(double v, const double range[2],
                               const double logRange[2]);

//unsigned char*
//NMVtkLinearLookupMain(double v,
//                unsigned char* table,
//                double maxIndex,
//                double shift, double scale);


vtkStandardNewMacro(NMVtkLookupTable);

NMVtkLookupTable::NMVtkLookupTable(int sze, int ext)
    : vtkLookupTable(sze, ext), mbUseLowerUpperClr(false)
{
}

NMVtkLookupTable::~NMVtkLookupTable(void)
{
}

void
NMVtkLookupTable::setLowerUpperClrOn(void)
{
    mbUseLowerUpperClr = true;
    _mbUseLowerUpperClr = true;
}

void
NMVtkLookupTable::setLowerUpperClrOff(void)
{
    mbUseLowerUpperClr = false;
    _mbUseLowerUpperClr = false;
}

void
NMVtkLookupTable::setLowerClr(double val, unsigned char clr[])
{
    mLower = val;
    _mLower = val;
    for (int c=0; c < 4; c++)
    {
        mLowerClr[c] = clr[c];
        _mLowerClr[c] = clr[c];
    }
}


void
NMVtkLookupTable::setUpperClr(double val, unsigned char clr[])
{
    mUpper = val;
    _mUpper = val;
    for (int c=0; c < 4; c++)
    {
        mUpperClr[c] = clr[c];
        _mUpperClr[c] = clr[c];
    }
}

static void NMVtkLookupTableLogRange(const double range[2], double logRange[2])
{
  double rmin = range[0];
  double rmax = range[1];

  // does the range include zero?
  if ((rmin <= 0 && rmax >= 0) ||
      (rmin >= 0 && rmax <= 0))
    {
    // clamp the smaller value to 1e-6 times the larger
    if (fabs(rmax) >= fabs(rmin))
      {
      rmin = rmax*1e-6;
      }
    else
      {
      rmax = rmin*1e-6;
      }

    // ensure values are not zero
    if (rmax == 0)
      {
      rmax = (rmin < 0 ? -VTK_DBL_MIN : VTK_DBL_MIN);
      }
    if (rmin == 0)
      {
      rmin = (rmax < 0 ? -VTK_DBL_MIN : VTK_DBL_MIN);
      }
    }

  if (rmax < 0) // rmin and rmax have same sign now
    {
    logRange[0] = -log10(-rmin);
    logRange[1] = -log10(-rmax);
    }
  else
    {
    logRange[0] = log10(rmin);
    logRange[1] = log10(rmax);
    }
}

unsigned char*
NMVtkLookupTable::MapValue(double v)
{
    if (mbUseLowerUpperClr)
    {
        if (v < mLower)
        {
            return mLowerClr;
        }
        else if (v > mUpper)
        {
            return mUpperClr;
        }
        else
        {
            int idx = this->GetIndex(v);
            return idx >= 0 ? (this->Table->GetPointer(0) + 4*idx) : this->GetNanColorAsUnsignedChars();
        }
    }
    else
    {
        int idx = this->GetIndex(v);
        return idx >= 0 ? (this->Table->GetPointer(0) + 4*idx) : this->GetNanColorAsUnsignedChars();
    }
}

void
NMVtkLookupTable::GetColor(double v, double rgb[3])
{
  unsigned char *rgb8 = this->MapValue(v);

  rgb[0] = rgb8[0]/255.0;
  rgb[1] = rgb8[1]/255.0;
  rgb[2] = rgb8[2]/255.0;
}

void
NMVtkLookupTable::MapScalarsThroughTable2(void *input,
                                             unsigned char *output,
                                             int inputDataType,
                                             int numberOfValues,
                                             int inputIncrement,
                                             int outputFormat)
{
  if ( this->IndexedLookup )
    {
    switch (inputDataType)
      {
      case VTK_BIT:
        {
        vtkIdType i, id;
        vtkBitArray *bitArray = vtkBitArray::New();
        bitArray->SetVoidArray(input,numberOfValues,1);
        vtkUnsignedCharArray *newInput = vtkUnsignedCharArray::New();
        newInput->SetNumberOfValues(numberOfValues);
        for (id=i=0; i<numberOfValues; i++, id+=inputIncrement)
          {
          newInput->SetValue(i, bitArray->GetValue(id));
          }
        NMVtkLookupTableIndexedMapData(this,
                                     static_cast<unsigned char*>(newInput->GetPointer(0)),
                                     output,numberOfValues,
                                     inputIncrement,outputFormat);
        newInput->Delete();
        bitArray->Delete();
        }
        break;

      vtkTemplateMacro(
        NMVtkLookupTableIndexedMapData(this,static_cast<VTK_TT*>(input),output,
                                     numberOfValues,inputIncrement,outputFormat)
        );
      default:
        vtkErrorMacro(<< "MapImageThroughTable: Unknown input ScalarType");
        return;
      }
    }
  else
    {
    switch (inputDataType)
      {
      case VTK_BIT:
        {
        vtkIdType i, id;
        vtkBitArray *bitArray = vtkBitArray::New();
        bitArray->SetVoidArray(input,numberOfValues,1);
        vtkUnsignedCharArray *newInput = vtkUnsignedCharArray::New();
        newInput->SetNumberOfValues(numberOfValues);
        for (id=i=0; i<numberOfValues; i++, id+=inputIncrement)
          {
          newInput->SetValue(i, bitArray->GetValue(id));
          }
        NMVtkLookupTableMapData(this,
                              static_cast<unsigned char*>(newInput->GetPointer(0)),
                              output,numberOfValues,
                              inputIncrement,outputFormat);
        newInput->Delete();
        bitArray->Delete();
        }
        break;

      vtkTemplateMacro(
        NMVtkLookupTableMapData(this,static_cast<VTK_TT*>(input),output,
                              numberOfValues,inputIncrement,outputFormat)
        );
      default:
        vtkErrorMacro(<< "MapImageThroughTable: Unknown input ScalarType");
        return;
      }
    }
}

//----------------------------------------------------------------------------
template<class T>
void NMVtkLookupTableMapData(NMVtkLookupTable *self, T *input,
                           unsigned char *output, int length,
                           int inIncr, int outFormat)
{
  int i = length;
  double *range = self->GetTableRange();
  double maxIndex = self->GetNumberOfColors() - 1;
  double shift, scale;
  unsigned char *table = self->GetPointer(0);
  unsigned char *cptr;
  double alpha;

  unsigned char nanColor[4];
  const double *nanColord = self->GetNanColor();
  for (int c = 0; c < 4; c++)
    {
    double v = nanColord[c];
    if (v < 0.0) { v = 0.0; }
    else if (v > 1.0) { v = 1.0; }
    nanColor[c] = static_cast<unsigned char>(v*255.0 + 0.5);
    }

  if ( (alpha=self->GetAlpha()) >= 1.0 ) //no blending required
    {
    if (self->GetScale() == VTK_SCALE_LOG10)
      {
      double val;
      double logRange[2];
      NMVtkLookupTableLogRange(range, logRange);
      shift = -logRange[0];
      if (logRange[1] <= logRange[0])
        {
        scale = VTK_DOUBLE_MAX;
        }
      else
        {
        scale = (maxIndex + 1)/(logRange[1] - logRange[0]);
        }
      if (outFormat == VTK_RGBA)
        {
        while (--i >= 0)
          {
          val = NMVtkApplyLogScale(*input, range, logRange);
          cptr = NMVtkLinearLookup(self, val, table, maxIndex, shift, scale, nanColor);
          output[0] = cptr[0];
          output[1] = cptr[1];
          output[2] = cptr[2];
          output[3] = cptr[3];
          input += inIncr;
          output += 4;
          }
        }
      else if (outFormat == VTK_RGB)
        {
        while (--i >= 0)
          {
          val = NMVtkApplyLogScale(*input, range, logRange);
          cptr = NMVtkLinearLookup(self, val, table, maxIndex, shift, scale, nanColor);
          output[0] = cptr[0];
          output[1] = cptr[1];
          output[2] = cptr[2];
          input += inIncr;
          output += 3;
          }
        }
      else if (outFormat == VTK_LUMINANCE_ALPHA)
        {
        while (--i >= 0)
          {
          val = NMVtkApplyLogScale(*input, range, logRange);
          cptr = NMVtkLinearLookup(self, val, table, maxIndex, shift, scale, nanColor);
          output[0] = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                                 cptr[2]*0.11 + 0.5);
          output[1] = cptr[3];
          input += inIncr;
          output += 2;
          }
        }
      else // outFormat == VTK_LUMINANCE
        {
        while (--i >= 0)
          {
          val = NMVtkApplyLogScale(*input, range, logRange);
          cptr = NMVtkLinearLookup(self, val, table, maxIndex, shift, scale, nanColor);
          *output++ = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                                 cptr[2]*0.11 + 0.5);
          input += inIncr;
          }
        }
      }//if log scale

    else //not log scale
      {
      shift = -range[0];
      if (range[1] <= range[0])
        {
        scale = VTK_DOUBLE_MAX;
        }
      else
        {
        scale = (maxIndex + 1)/(range[1] - range[0]);
        }

      if (outFormat == VTK_RGBA)
        {
        while (--i >= 0)
          {
          cptr = NMVtkLinearLookup(self, *input, table, maxIndex, shift, scale,
                                 nanColor);
          output[0] = cptr[0];
          output[1] = cptr[1];
          output[2] = cptr[2];
          output[3] = cptr[3];
          input += inIncr;
          output += 4;
          }
        }
      else if (outFormat == VTK_RGB)
        {
        while (--i >= 0)
          {
          cptr = NMVtkLinearLookup(self, *input, table, maxIndex, shift, scale,
                                 nanColor);
          output[0] = cptr[0];
          output[1] = cptr[1];
          output[2] = cptr[2];
          input += inIncr;
          output += 3;
          }
        }
      else if (outFormat == VTK_LUMINANCE_ALPHA)
        {
        while (--i >= 0)
          {
          cptr = NMVtkLinearLookup(self, *input, table, maxIndex, shift, scale,
                                 nanColor);
          output[0] = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                                 cptr[2]*0.11 + 0.5);
          output[1] = cptr[3];
          input += inIncr;
          output += 2;
          }
        }
      else // outFormat == VTK_LUMINANCE
        {
        while (--i >= 0)
          {
          cptr = NMVtkLinearLookup(self, *input, table, maxIndex, shift, scale,
                                 nanColor);
          *output++ = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                                 cptr[2]*0.11 + 0.5);
          input += inIncr;
          }
        }
      }//if not log lookup
    }//if blending not needed

  else //blend with the specified alpha
    {
    if (self->GetScale() == VTK_SCALE_LOG10)
      {
      double val;
      double logRange[2];
      NMVtkLookupTableLogRange(range, logRange);
      shift = -logRange[0];
      if (logRange[1] <= logRange[0])
        {
        scale = VTK_DOUBLE_MAX;
        }
      else
        {
        scale = (maxIndex + 1)/(logRange[1] - logRange[0]);
        }
      if (outFormat == VTK_RGBA)
        {
        while (--i >= 0)
          {
          val = NMVtkApplyLogScale(*input, range, logRange);
          cptr = NMVtkLinearLookup(self, val, table, maxIndex, shift, scale, nanColor);
          output[0] = cptr[0];
          output[1] = cptr[1];
          output[2] = cptr[2];
          output[3] = static_cast<unsigned char>(cptr[3]*alpha + 0.5);
          input += inIncr;
          output += 4;
          }
        }
      else if (outFormat == VTK_RGB)
        {
        while (--i >= 0)
          {
          val = NMVtkApplyLogScale(*input, range, logRange);
          cptr = NMVtkLinearLookup(self, val, table, maxIndex, shift, scale, nanColor);
          output[0] = cptr[0];
          output[1] = cptr[1];
          output[2] = cptr[2];
          input += inIncr;
          output += 3;
          }
        }
      else if (outFormat == VTK_LUMINANCE_ALPHA)
        {
        while (--i >= 0)
          {
          val = NMVtkApplyLogScale(*input, range, logRange);
          cptr = NMVtkLinearLookup(self, val, table, maxIndex, shift, scale, nanColor);
          output[0] = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                                 cptr[2]*0.11 + 0.5);
          output[1] = static_cast<unsigned char>(alpha*cptr[3] + 0.5);
          input += inIncr;
          output += 2;
          }
        }
      else // outFormat == VTK_LUMINANCE
        {
        while (--i >= 0)
          {
          val = NMVtkApplyLogScale(*input, range, logRange);
          cptr = NMVtkLinearLookup(self, val, table, maxIndex, shift, scale, nanColor);
          *output++ = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                                 cptr[2]*0.11 + 0.5);
          input += inIncr;
          }
        }
      }//log scale with blending

    else //no log scale with blending
      {
      shift = -range[0];
      if (range[1] <= range[0])
        {
        scale = VTK_DOUBLE_MAX;
        }
      else
        {
        scale = (maxIndex + 1)/(range[1] - range[0]);
        }

      if (outFormat == VTK_RGBA)
        {
        while (--i >= 0)
          {
          cptr = NMVtkLinearLookup(self, *input, table, maxIndex, shift, scale,
                                 nanColor);
          output[0] = cptr[0];
          output[1] = cptr[1];
          output[2] = cptr[2];
          output[3] = static_cast<unsigned char>(cptr[3]*alpha + 0.5);
          input += inIncr;
          output += 4;
          }
        }
      else if (outFormat == VTK_RGB)
        {
        while (--i >= 0)
          {
          cptr = NMVtkLinearLookup(self, *input, table, maxIndex, shift, scale,
                                 nanColor);
          output[0] = cptr[0];
          output[1] = cptr[1];
          output[2] = cptr[2];
          input += inIncr;
          output += 3;
          }
        }
      else if (outFormat == VTK_LUMINANCE_ALPHA)
        {
        while (--i >= 0)
          {
          cptr = NMVtkLinearLookup(self, *input, table, maxIndex, shift, scale,
                                 nanColor);
          output[0] = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                                 cptr[2]*0.11 + 0.5);
          output[1] = static_cast<unsigned char>(cptr[3]*alpha + 0.5);
          input += inIncr;
          output += 2;
          }
        }
      else // outFormat == VTK_LUMINANCE
        {
        while (--i >= 0)
          {
          cptr = NMVtkLinearLookup(self, *input, table, maxIndex, shift, scale,
                                 nanColor);
          *output++ = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                                 cptr[2]*0.11 + 0.5);
          input += inIncr;
          }
        }
      }//no log scale
    }//alpha blending
}

//----------------------------------------------------------------------------
// Apply log to value, with appropriate constraints.
inline double NMVtkApplyLogScale(double v, const double range[2],
                               const double logRange[2])
{
  // is the range set for negative numbers?
  if (range[0] < 0)
    {
    if (v < 0)
      {
      v = -log10(-v);
      }
    else if (range[0] > range[1])
      {
      v = logRange[0];
      }
    else
      {
      v = logRange[1];
      }
    }
  else
    {
    if (v > 0)
      {
      v = log10(v);
      }
    else if (range[0] <= range[1])
      {
      v = logRange[0];
      }
    else
      {
      v = logRange[1];
      }
    }
  return v;
}


//----------------------------------------------------------------------------
template<class T>
void NMVtkLookupTableIndexedMapData(
  NMVtkLookupTable* self, T* input, unsigned char* output, int length,
  int inIncr, int outFormat )
{
  int i = length;
  unsigned char* cptr;
  double alpha;

  unsigned char nanColor[4];
  const unsigned char* nanColorTmp = self->GetNanColorAsUnsignedChars();
  for (int c = 0; c < 4; c++)
    {
    nanColor[c] = nanColorTmp[c];
    }

  vtkVariant vin;
  if ( (alpha=self->GetAlpha()) >= 1.0 ) //no blending required
    {
    if (outFormat == VTK_RGBA)
      {
      while (--i >= 0)
        {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal( vin );
        cptr = idx < 0 ? nanColor : self->GetPointer( idx );

        output[0] = cptr[0];
        output[1] = cptr[1];
        output[2] = cptr[2];
        output[3] = cptr[3];
        input += inIncr;
        output += 4;
        }
      }
    else if (outFormat == VTK_RGB)
      {
      while (--i >= 0)
        {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal( vin );
        cptr = idx < 0 ? nanColor : self->GetPointer( idx );

        output[0] = cptr[0];
        output[1] = cptr[1];
        output[2] = cptr[2];
        input += inIncr;
        output += 3;
        }
      }
    else if (outFormat == VTK_LUMINANCE_ALPHA)
      {
      while (--i >= 0)
        {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal( vin );
        cptr = idx < 0 ? nanColor : self->GetPointer( idx );
        output[0] = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                               cptr[2]*0.11 + 0.5);
        output[1] = cptr[3];
        input += inIncr;
        output += 2;
        }
      }
    else // outFormat == VTK_LUMINANCE
      {
      while (--i >= 0)
        {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal( vin );
        cptr = idx < 0 ? nanColor : self->GetPointer( idx );
        *output++ = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                               cptr[2]*0.11 + 0.5);
        input += inIncr;
        }
      }
    } // if blending not needed

  else // blend with the specified alpha
    {
    if (outFormat == VTK_RGBA)
      {
      while (--i >= 0)
        {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal( vin );
        cptr = idx < 0 ? nanColor : self->GetPointer( idx );
        output[0] = cptr[0];
        output[1] = cptr[1];
        output[2] = cptr[2];
        output[3] = static_cast<unsigned char>(cptr[3]*alpha + 0.5);
        input += inIncr;
        output += 4;
        }
      }
    else if (outFormat == VTK_RGB)
      {
      while (--i >= 0)
        {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal( vin );
        cptr = idx < 0 ? nanColor : self->GetPointer( idx );
        output[0] = cptr[0];
        output[1] = cptr[1];
        output[2] = cptr[2];
        input += inIncr;
        output += 3;
        }
      }
    else if (outFormat == VTK_LUMINANCE_ALPHA)
      {
      while (--i >= 0)
        {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal( vin );
        cptr = idx < 0 ? nanColor : self->GetPointer( idx );
        output[0] = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                               cptr[2]*0.11 + 0.5);
        output[1] = static_cast<unsigned char>(cptr[3]*alpha + 0.5);
        input += inIncr;
        output += 2;
        }
      }
    else // outFormat == VTK_LUMINANCE
      {
      while (--i >= 0)
        {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal( vin );
        cptr = idx < 0 ? nanColor : self->GetPointer( idx );
        *output++ = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                               cptr[2]*0.11 + 0.5);
        input += inIncr;
        }
      }
    } // alpha blending
}

//==============================================================================
template<class T>
unsigned char *NMVtkLinearLookup(NMVtkLookupTable* self,
  T v, unsigned char *table, double maxIndex, double shift, double scale,
  unsigned char *vtkNotUsed(nanColor))
{
  return self->linearLookupMain(v, table, maxIndex, shift, scale);
}

unsigned char*
NMVtkLookupTable::linearLookupMain(double v,
                unsigned char* table,
                double maxIndex,
                double shift, double scale)
{
    if (mbUseLowerUpperClr)
    {
        if (v < mLower)
        {
            return mLowerClr;
        }
        else if (v > mUpper)
        {
            return mUpperClr;
        }
        else
        {
            double findx = (v + shift)*scale;

            // do not change this code: it compiles into min/max opcodes
            findx = (findx > 0 ? findx : 0);
            findx = (findx < maxIndex ? findx : maxIndex);

            return &table[4*static_cast<unsigned int>(findx)];
        }
    }
    else
    {
        double findx = (v + shift)*scale;

        // do not change this code: it compiles into min/max opcodes
        findx = (findx > 0 ? findx : 0);
        findx = (findx < maxIndex ? findx : maxIndex);

        return &table[4*static_cast<unsigned int>(findx)];
    }
}
