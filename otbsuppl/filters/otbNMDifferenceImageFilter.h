
#ifndef otbNMDifferenceImageFilter_h
#define otbNMDifferenceImageFilter_h

#include "otbDifferenceImageFilter.h"

namespace otb
{

template <class TInputImage, class TOutputImage>
class ITK_EXPORT NMDifferenceImageFilter : public otb::DifferenceImageFilter <TInputImage, TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef NMDifferenceImageFilter Self;
  typedef otb::DifferenceImageFilter<TInputImage, TOutputImage> Superclass;
  typedef itk::SmartPointer<Self>       Pointer;
  typedef itk::SmartPointer<const Self> ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(NMDifferenceImageFilter, DifferenceImageFilter);

  /** Some convenient typedefs. */
  //typedef TInputImage                                                  InputImageType;
  //typedef TOutputImage                                                 OutputImageType;
  //typedef typename OutputImageType::PixelType                          OutputPixelType;
  //typedef typename OutputImageType::RegionType                         OutputImageRegionType;
  //typedef typename itk::NumericTraits<OutputPixelType>::RealType       RealType;
  //typedef typename itk::NumericTraits<RealType>::AccumulateType        AccumulateType;
  //typedef typename itk::NumericTraits<OutputPixelType>::ScalarRealType ScalarRealType;

  itkSetMacro(PrintResults, bool);
  itkSetMacro(ResultFileName, std::string);
  itkSetMacro(Workspace, std::string);

protected:
  NMDifferenceImageFilter();
  ~NMDifferenceImageFilter() = default;

  void BeforeThreadedGenerateData();
  void AfterThreadedGenerateData();

  bool m_PrintResults;
  std::string m_ResultFileName;
  std::string m_Workspace;


}; // of class declaration

} // end namespace otb

#ifndef OTB_MANUAL_INSTANTIATION
#include "otbNMDifferenceImageFilter.txx"
#endif

#endif // include guard
