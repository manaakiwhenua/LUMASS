

#include "otbNMDifferenceImageFilter.h"
#include "nmlog.h"

#include <ctime>
#include <fstream>
#include <iostream>

namespace otb
{

template <class TInputImage, class TOutputImage>
NMDifferenceImageFilter<TInputImage, TOutputImage>::NMDifferenceImageFilter()
    : m_PrintResults(false)
{
}

template <class TInputImage, class TOutputImage>
void
NMDifferenceImageFilter<TInputImage, TOutputImage>::BeforeThreadedGenerateData()
{
    Superclass::Reset();
}

template <class TInputImage, class TOutputImage>
void
NMDifferenceImageFilter<TInputImage, TOutputImage>::AfterThreadedGenerateData()
{
    Superclass::Synthetize();

    std::stringstream resstr;
    resstr << "Total difference = " << this->m_TotalDifference
           << " | Number of different pixels = "
           << this->m_NumberOfPixelsWithDifferences;

    NMDebugAI(<< "CompareImage: " << resstr.str() << std::endl);


    if (m_PrintResults)
    {
        NMProcInfo(<< resstr.str());

        // inspirations from:
        // https://stackoverflow.com/questions/9527960/how-do-i-construct-an-iso-8601-datetime-in-c
        // https://cplusplus.com/reference/ctime/strftime/

        time_t curTime;
        time (&curTime);

        char timeStr[sizeof "20250218T142715"];
        const int len = sizeof(timeStr);
        strftime(timeStr, sizeof(timeStr), "%Y%m%dT%H%M%S", localtime(&curTime));

        if (m_ResultFileName.empty())
        {
            m_ResultFileName = m_Workspace + "/CompareImage_result_" + timeStr + ".txt";
        }

        std::ofstream outfile(m_ResultFileName);
        if (outfile.is_open())
        {
            outfile << "TotalDifference=" << this->m_TotalDifference << std::endl;
            outfile << "NumberOfPixelsWithDifference=" << this->m_NumberOfPixelsWithDifferences << std::endl;
            outfile.close();
        }
    }
}




} // end of otb namespace
