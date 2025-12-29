#pragma once


#ifndef NMItkDataObjectWrapperImportTemplates_H
#define NMItkDataObjectWrapperImportTemplates_H

#ifdef _WIN32
    #define DOW_IMPORT_TEMPLATES __declspec(dllimport)
#else
    #define DOW_IMPORT_TEMPLATES
#endif

//#include "itkImageBase.h"
#include "itkRGBPixel.h"
#include "otbImage.h"
#include "otbVectorImage.h"
#include "itkImageToImageFilter.h"

extern template class DOW_IMPORT_TEMPLATES itk::ImageBase<1>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageBase<2>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageBase<3>;

extern template class DOW_IMPORT_TEMPLATES otb::Image<unsigned long, 1 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<long, 1 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<unsigned long long, 1 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<long long, 1 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<unsigned int, 1 > ;
extern template class DOW_IMPORT_TEMPLATES otb::Image<int, 1 > ;
extern template class DOW_IMPORT_TEMPLATES otb::Image<unsigned char, 1 > ;
extern template class DOW_IMPORT_TEMPLATES otb::Image<char, 1 > ;
extern template class DOW_IMPORT_TEMPLATES otb::Image<unsigned short, 1 > ;
extern template class DOW_IMPORT_TEMPLATES otb::Image<short, 1 > ;
extern template class DOW_IMPORT_TEMPLATES otb::Image<float, 1 > ;
extern template class DOW_IMPORT_TEMPLATES otb::Image<double, 1 > ;

extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<unsigned long, 1 >;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<long, 1 >;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<unsigned long long, 1 >;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<long long, 1 >;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<unsigned int, 1 > ;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<int, 1 > ;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<unsigned char, 1 > ;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<char, 1 > ;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<unsigned short, 1 > ;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<short, 1 > ;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<float, 1 > ;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<double, 1 > ;


extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<unsigned long, 1 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<long, 1 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<unsigned long long, 1 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<long long, 1 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<unsigned int, 1 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<int, 1 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<unsigned char, 1 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<char, 1 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<unsigned short, 1 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<short, 1 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<float, 1 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<double, 1 >>;

extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<unsigned long, 1 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<long, 1 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<unsigned long long, 1 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<long long, 1 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<unsigned int, 1 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<int, 1 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<unsigned char, 1 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<char, 1 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<unsigned short, 1 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<short, 1 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<float, 1 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<double, 1 >>;


extern template class DOW_IMPORT_TEMPLATES otb::Image<unsigned long, 2 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<long, 2 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<unsigned long long, 2 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<long long, 2 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<unsigned int, 2 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<int, 2 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<unsigned char, 2 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<char, 2 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<unsigned short, 2 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<short, 2 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<float, 2 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<double, 2 >;

extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<unsigned long, 2 >;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<long, 2 >;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<unsigned long long, 2 >;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<long long, 2 >;

extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<unsigned long, 2 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<long, 2 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<unsigned long long, 2 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<long long, 2 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<unsigned int, 2 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<int, 2 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<unsigned char, 2 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<char, 2 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<unsigned short, 2 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<short, 2 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<float, 2 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<double, 2 >>;

extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<unsigned long, 2 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<long, 2 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<unsigned long long, 2 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<long long, 2 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<unsigned int, 2 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<int, 2 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<unsigned char, 2 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<char, 2 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<unsigned short, 2 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<short, 2 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<float, 2 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<double, 2 >>;


extern template class DOW_IMPORT_TEMPLATES otb::Image<unsigned long, 3 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<long, 3 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<unsigned long long, 3 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<long long, 3 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<unsigned int, 3 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<int, 3 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<unsigned char, 3 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<char, 3 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<unsigned short, 3 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<short, 3 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<float, 3 >;
extern template class DOW_IMPORT_TEMPLATES otb::Image<double, 3 >;

extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<unsigned long, 3 >;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<long, 3 >;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<unsigned long long, 3 >;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<long long, 3 >;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<unsigned int, 3 >;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<int, 3 >;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<unsigned char, 3 >;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<char, 3 >;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<unsigned short, 3 >;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<short, 3 >;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<float, 3 >;
extern template class DOW_IMPORT_TEMPLATES otb::VectorImage<double, 3 >;


extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<unsigned long, 3 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<long, 3 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<unsigned long long, 3 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<long long, 3 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<unsigned int, 3 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<int, 3 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<unsigned char, 3 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<char, 3 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<unsigned short, 3 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<short, 3 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<float, 3 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<double, 3 >>;

extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<unsigned long, 3 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<long, 3 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<unsigned long long, 3 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<long long, 3 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<unsigned int, 3 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<int, 3 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<unsigned char, 3 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<char, 3 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<unsigned short, 3 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<short, 3 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<float, 3 >>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::VectorImage<double, 3 >>;


extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<unsigned int>, 1>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<int>, 1>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<unsigned char>, 1>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<char>, 1>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<unsigned short>, 1>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<short>, 1>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<float>, 1>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<double>, 1>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<unsigned long>, 1>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<long>, 1>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<unsigned long long>, 1>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<long long>, 1>;

extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<unsigned int>, 2>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<int>, 2>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<unsigned char>, 2>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<char>, 2>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<unsigned short>, 2>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<short>, 2>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<float>, 2>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<double>, 2>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<unsigned long>, 2>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<long>, 2>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<unsigned long long>, 2>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<long long>, 2>;


extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<unsigned int>, 3>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<int>, 3>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<unsigned char>, 3>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<char>, 3>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<unsigned short>, 3>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<short>, 3>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<float>, 3>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<double>, 3>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<unsigned long>, 3>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<long>, 3>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<unsigned long long>, 3>;
extern template class DOW_IMPORT_TEMPLATES otb::Image<itk::RGBPixel<long long>, 3>;

extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<unsigned int>, 1>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<int>, 1>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<unsigned char>, 1>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<char>, 1>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<unsigned short>, 1>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<short>, 1>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<float>, 1>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<double>, 1>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<unsigned long>, 1>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<long>, 1>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<unsigned long long>, 1>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<long long>, 1>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<unsigned int>, 2>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<int>, 2>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<unsigned char>, 2>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<char>, 2>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<unsigned short>, 2>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<short>, 2>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<float>, 2>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<double>, 2>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<unsigned long>, 2>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<long>, 2>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<unsigned long long>, 2>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<long long>, 2>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<unsigned int>, 3>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<int>, 3>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<unsigned char>, 3>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<char>, 3>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<unsigned short>, 3>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<short>, 3>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<float>, 3>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<double>, 3>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<unsigned long>, 3>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<long>, 3>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<unsigned long long>, 3>>;
extern template class DOW_IMPORT_TEMPLATES itk::ImageSource<otb::Image<itk::RGBPixel<long long>, 3>>;



extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<unsigned long long, 1 >, otb::Image<unsigned long long, 1 >   >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<long long, 1 >, otb::Image<long long, 1 >            >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<unsigned long, 1 >, otb::Image<unsigned long, 1 >  >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<long, 1 >, otb::Image<long, 1 >           >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<unsigned int, 1 >, otb::Image<unsigned int, 1 >   >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<int, 1 >, otb::Image<int, 1 >            >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<unsigned char, 1 >, otb::Image<unsigned char, 1 >  >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<char, 1 >, otb::Image<char, 1 >           >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<unsigned short, 1 >, otb::Image<unsigned short, 1 > >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<short, 1 >, otb::Image<short, 1 >          >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<float, 1 >, otb::Image<float, 1 >          >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<double, 1 >, otb::Image<double, 1 >         >;

extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<unsigned long long, 2 >, otb::Image<unsigned long long, 2 >   >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<long long, 2 >, otb::Image<long long, 2 >            >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<unsigned long, 2 >, otb::Image<unsigned long, 2 >  >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<long, 2 >, otb::Image<long, 2 >           >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<unsigned int, 2 >  , otb::Image<unsigned int, 2 >   >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<int, 2 >, otb::Image<int, 2 >            >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<unsigned char, 2 >, otb::Image<unsigned char, 2 >  >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<char, 2 >, otb::Image<char, 2 >           >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<unsigned short, 2 >, otb::Image<unsigned short, 2 > >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<short, 2 >, otb::Image<short, 2 >          >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<float, 2 >, otb::Image<float, 2 >          >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<double, 2 >, otb::Image<double, 2 >         >;

extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<unsigned long long, 3 >, otb::Image<unsigned long long, 3 >   >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<long long, 3 >, otb::Image<long long, 3 >            >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<unsigned long, 3 >, otb::Image<unsigned long, 3 >  >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<long, 3 >, otb::Image<long, 3 >           >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<unsigned int, 3 >, otb::Image<unsigned int, 3 >   >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<int, 3 >, otb::Image<int, 3 >            >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<unsigned char, 3 >, otb::Image<unsigned char, 3 >  >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<char, 3 >, otb::Image<char, 3 >           >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<unsigned short, 3 >, otb::Image<unsigned short, 3 > >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<short, 3 >, otb::Image<short, 3 >          >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<float, 3 >, otb::Image<float, 3 >          >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::Image<double, 3 >, otb::Image<double, 3 >         >;

extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<unsigned long long, 1 >, otb::VectorImage<unsigned long long, 1 >   >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<long long, 1 >, otb::VectorImage<long long, 1 >            >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<unsigned long, 1 >, otb::VectorImage<unsigned long, 1 >  >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<long, 1 >, otb::VectorImage<long, 1 >           >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<unsigned int, 1 >, otb::VectorImage<unsigned int, 1 >   >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<int, 1 >, otb::VectorImage<int, 1 >            >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<unsigned char, 1 >, otb::VectorImage<unsigned char, 1 >  >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<char, 1 >, otb::VectorImage<char, 1 >           >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<unsigned short, 1 >, otb::VectorImage<unsigned short, 1 > >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<short, 1 >, otb::VectorImage<short, 1 >          >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<float, 1 >, otb::VectorImage<float, 1 >          >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<double, 1 >, otb::VectorImage<double, 1 >         >;

extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<unsigned long long, 2 >, otb::VectorImage<unsigned long long, 2 >   >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<long long, 2 >, otb::VectorImage<long long, 2 >            >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<unsigned long, 2 >, otb::VectorImage<unsigned long, 2 >  >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<long, 2 >, otb::VectorImage<long, 2 >           >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<unsigned int, 2 >, otb::VectorImage<unsigned int, 2 >   >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<int, 2 >, otb::VectorImage<int, 2 >            >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<unsigned char, 2 >, otb::VectorImage<unsigned char, 2 >  >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<char, 2 >, otb::VectorImage<char, 2 >           >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<unsigned short, 2 >, otb::VectorImage<unsigned short, 2 > >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<short, 2 >, otb::VectorImage<short, 2 >          >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<float, 2 >, otb::VectorImage<float, 2 >          >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<double, 2 >, otb::VectorImage<double, 2 >         >;

extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<unsigned long long, 3 >, otb::VectorImage<unsigned long long, 3 >   >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<long long, 3 >, otb::VectorImage<long long, 3 >            >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<unsigned long, 3 >, otb::VectorImage<unsigned long, 3 >  >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<long, 3 >, otb::VectorImage<long, 3 >           >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<unsigned int, 3 >, otb::VectorImage<unsigned int, 3 >   >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<int, 3 >, otb::VectorImage<int, 3 >            >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<unsigned char, 3 >, otb::VectorImage<unsigned char, 3 >  >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<char, 3 >, otb::VectorImage<char, 3 >           >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<unsigned short, 3 >, otb::VectorImage<unsigned short, 3 > >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<short, 3 >, otb::VectorImage<short, 3 >          >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<float, 3 >, otb::VectorImage<float, 3 >          >;
extern template class DOW_IMPORT_TEMPLATES itk::ImageToImageFilter<otb::VectorImage<double, 3 >, otb::VectorImage<double, 3 >         >;


#endif  /* NMItkDataObjectWrapperImportTemplates_H */
