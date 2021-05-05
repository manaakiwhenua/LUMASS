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
#include "NMMacros.h"
#include "NMProcess.h"
#include "NMModelController.h"
#include "NMRATBandMathImageFilterWrapper.h"
#include "NMImageReader.h"
#include "NMModelComponent.h"
#include "NMIterableComponent.h"
#include "NMSequentialIterComponent.h"
#include "NMMfwException.h"

#include <string>
#include <vector>
#include "itkProcessObject.h"
#include "itkDataObject.h"
#include "otbImage.h"
#include "otbRATBandMathImageFilter.h"
#include "NMItkDataObjectWrapper.h"
#include "NMOtbAttributeTableWrapper.h"
#include "otbAttributeTable.h"

#include <QVariant>
#include <QVector>
#include <QString>
#include <QStringList>

/** Helper Classes */
template <class inputType, class outputType, unsigned int Dimension>
class NMRATBandMathImageFilterWrapper_Internal
{
public:
	typedef otb::Image<inputType, Dimension> ImgType;
    typedef otb::RATBandMathImageFilter<ImgType> FilterType;
    //typedef otb::AutoResampleMapAlgebraFilter<ImgType> FilterType;
	typedef typename FilterType::Pointer FilterTypePointer;

	static void createInstance(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands)
		{
			FilterTypePointer f = FilterType::New();
			otbFilter = f;
		}

	static void setExpression(itk::ProcessObject::Pointer& otbFilter,
			QString expression)
		{
			FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
			filter->SetExpression(expression.toStdString());
		}

	static void setNbExpr(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numExpr)
		{
			FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
			filter->SetNbExpr(numExpr);
		}

    static void setUseTableColumnCache(itk::ProcessObject::Pointer& otbFilter,
            bool useCache)
        {
            FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
            filter->SetUseTableColumnCache(useCache);
        }

	static void setNthInput(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands, unsigned int idx, itk::DataObject* dataObj)//, QString varName)
		{
			//NMDebugCtx(ctxNMRATBandMathWrapper, << "...");
			ImgType* img = dynamic_cast<ImgType*>(dataObj);
			FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());

			//NMDebugAI(<< "used input ..." << std::endl);
			//NMDebugAI(<< "  index: " << idx << std::endl);
			//NMDebugAI(<< "  varName: " << varName.toStdString() << std::endl);

			filter->SetNthInput(idx, img); //, varName.toStdString());

			//NMDebugCtx(ctxNMRATBandMathWrapper, << "done!");
		}

    static void setNthInputName(itk::ProcessObject::Pointer& otbFilter,
            unsigned int idx, const std::string& name)
        {
            FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
            filter->SetNthInputName(idx, name);
        }

	static void setNthAttributeTable(itk::ProcessObject::Pointer& otbFilter,
			unsigned int idx, otb::AttributeTable::Pointer tab, std::vector<std::string> varNames)
		{
			FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
			filter->SetNthAttributeTable(idx, tab, varNames);
		}

	static itk::DataObject* getOutput(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands, unsigned int idx)
		{
			FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
			return dynamic_cast<ImgType*>(filter->GetOutput(idx));
		}
};

template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, unsigned char, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, char, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, unsigned short, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, short, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, unsigned int, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, int, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, unsigned long, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, long, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, float, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, double, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<char, unsigned char, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<char, char, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<char, unsigned short, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<char, short, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<char, unsigned int, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<char, int, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<char, unsigned long, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<char, long, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<char, float, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<char, double, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, unsigned char, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, char, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, unsigned short, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, short, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, unsigned int, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, int, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, unsigned long, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, long, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, float, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, double, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<short, unsigned char, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<short, char, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<short, unsigned short, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<short, short, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<short, unsigned int, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<short, int, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<short, unsigned long, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<short, long, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<short, float, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<short, double, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, unsigned char, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, char, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, unsigned short, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, short, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, unsigned int, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, int, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, unsigned long, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, long, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, float, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, double, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<int, unsigned char, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<int, char, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<int, unsigned short, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<int, short, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<int, unsigned int, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<int, int, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<int, unsigned long, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<int, long, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<int, float, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<int, double, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, unsigned char, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, char, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, unsigned short, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, short, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, unsigned int, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, int, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, unsigned long, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, long, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, float, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, double, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<long, unsigned char, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<long, char, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<long, unsigned short, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<long, short, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<long, unsigned int, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<long, int, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<long, unsigned long, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<long, long, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<long, float, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<long, double, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<float, unsigned char, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<float, char, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<float, unsigned short, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<float, short, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<float, unsigned int, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<float, int, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<float, unsigned long, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<float, long, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<float, float, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<float, double, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<double, unsigned char, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<double, char, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<double, unsigned short, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<double, short, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<double, unsigned int, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<double, int, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<double, unsigned long, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<double, long, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<double, float, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<double, double, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, unsigned char, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, char, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, unsigned short, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, short, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, unsigned int, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, int, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, unsigned long, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, long, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, float, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, double, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<char, unsigned char, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<char, char, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<char, unsigned short, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<char, short, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<char, unsigned int, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<char, int, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<char, unsigned long, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<char, long, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<char, float, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<char, double, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, unsigned char, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, char, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, unsigned short, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, short, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, unsigned int, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, int, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, unsigned long, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, long, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, float, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, double, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<short, unsigned char, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<short, char, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<short, unsigned short, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<short, short, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<short, unsigned int, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<short, int, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<short, unsigned long, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<short, long, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<short, float, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<short, double, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, unsigned char, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, char, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, unsigned short, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, short, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, unsigned int, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, int, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, unsigned long, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, long, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, float, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, double, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<int, unsigned char, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<int, char, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<int, unsigned short, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<int, short, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<int, unsigned int, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<int, int, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<int, unsigned long, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<int, long, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<int, float, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<int, double, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, unsigned char, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, char, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, unsigned short, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, short, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, unsigned int, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, int, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, unsigned long, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, long, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, float, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, double, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<long, unsigned char, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<long, char, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<long, unsigned short, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<long, short, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<long, unsigned int, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<long, int, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<long, unsigned long, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<long, long, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<long, float, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<long, double, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<float, unsigned char, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<float, char, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<float, unsigned short, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<float, short, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<float, unsigned int, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<float, int, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<float, unsigned long, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<float, long, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<float, float, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<float, double, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<double, unsigned char, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<double, char, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<double, unsigned short, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<double, short, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<double, unsigned int, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<double, int, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<double, unsigned long, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<double, long, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<double, float, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<double, double, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, unsigned char, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, char, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, unsigned short, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, short, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, unsigned int, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, int, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, unsigned long, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, long, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, float, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, double, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<char, unsigned char, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<char, char, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<char, unsigned short, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<char, short, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<char, unsigned int, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<char, int, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<char, unsigned long, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<char, long, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<char, float, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<char, double, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, unsigned char, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, char, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, unsigned short, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, short, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, unsigned int, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, int, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, unsigned long, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, long, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, float, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, double, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<short, unsigned char, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<short, char, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<short, unsigned short, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<short, short, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<short, unsigned int, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<short, int, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<short, unsigned long, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<short, long, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<short, float, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<short, double, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, unsigned char, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, char, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, unsigned short, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, short, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, unsigned int, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, int, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, unsigned long, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, long, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, float, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, double, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<int, unsigned char, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<int, char, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<int, unsigned short, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<int, short, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<int, unsigned int, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<int, int, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<int, unsigned long, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<int, long, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<int, float, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<int, double, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, unsigned char, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, char, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, unsigned short, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, short, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, unsigned int, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, int, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, unsigned long, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, long, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, float, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, double, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<long, unsigned char, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<long, char, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<long, unsigned short, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<long, short, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<long, unsigned int, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<long, int, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<long, unsigned long, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<long, long, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<long, float, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<long, double, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<float, unsigned char, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<float, char, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<float, unsigned short, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<float, short, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<float, unsigned int, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<float, int, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<float, unsigned long, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<float, long, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<float, float, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<float, double, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<double, unsigned char, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<double, char, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<double, unsigned short, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<double, short, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<double, unsigned int, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<double, int, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<double, unsigned long, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<double, long, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<double, float, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<double, double, 3>;


GetOutputWrap( NMRATBandMathImageFilterWrapper, NMRATBandMathImageFilterWrapper_Internal )
InstantiateObjectWrap( NMRATBandMathImageFilterWrapper, NMRATBandMathImageFilterWrapper_Internal )
SetNthInputWrap( NMRATBandMathImageFilterWrapper, NMRATBandMathImageFilterWrapper_Internal )


#define callSetNthAttributeTable( filterPixelType, wrapName )											\
if (this->mInputNumDimensions == 1)                                                         \
{                                                                                           \
    wrapName<filterPixelType, filterPixelType, 1>::setNthAttributeTable(     \
            this->mOtbProcess, idx, table, colnames);                                          \
}                                                                                           \
else if (this->mInputNumDimensions == 2)                                                         \
{                                                                                           \
	wrapName<filterPixelType, filterPixelType, 2>::setNthAttributeTable(     \
			this->mOtbProcess, idx, table, colnames);                                          \
}                                                                                           \
else if (this->mInputNumDimensions == 3)                                                    \
{                                                                                           \
	wrapName<filterPixelType, filterPixelType, 3>::setNthAttributeTable(     \
			this->mOtbProcess, idx, table, colnames);                                          \
}                                                                                           \



#define callSetExpression( filterPixelType, wrapName ) \
{ \
    if (this->mInputNumDimensions == 1) \
    { \
        NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 1 >::setExpression( \
                this->mOtbProcess, expression); \
    } \
    else if (this->mInputNumDimensions == 2) \
	{ \
		NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 2 >::setExpression( \
				this->mOtbProcess, expression); \
	} \
	else if (this->mInputNumDimensions == 3) \
	{ \
		NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 3 >::setExpression( \
				this->mOtbProcess, expression); \
	}\
}

#define callSetNbExpr( filterPixelType, wrapName ) \
{ \
    if (this->mInputNumDimensions == 1) \
    { \
        NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 1 >::setNbExpr( \
                this->mOtbProcess, numExpr); \
    } \
    else if (this->mInputNumDimensions == 2) \
	{ \
		NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 2 >::setNbExpr( \
				this->mOtbProcess, numExpr); \
	} \
	else if (this->mInputNumDimensions == 3) \
	{ \
		NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 3 >::setNbExpr( \
				this->mOtbProcess, numExpr); \
	}\
}


#define callSetUseTableCache( filterPixelType, wrapName ) \
{ \
    if (this->mInputNumDimensions == 1) \
    { \
        NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 1 >::setUseTableColumnCache( \
                this->mOtbProcess, useCache); \
    } \
    else if (this->mInputNumDimensions == 2) \
    { \
        NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 2 >::setUseTableColumnCache( \
                this->mOtbProcess, useCache); \
    } \
    else if (this->mInputNumDimensions == 3) \
    { \
        NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 3 >::setUseTableColumnCache( \
                this->mOtbProcess, useCache); \
    }\
}


#define callSetNthInputName( filterPixelType, wrapName ) \
{ \
    if (this->mInputNumDimensions == 1) \
    { \
        NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 1 >::setNthInputName( \
                this->mOtbProcess, idx, name); \
    } \
    else if (this->mInputNumDimensions == 2) \
    { \
        NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 2 >::setNthInputName( \
                this->mOtbProcess, idx, name); \
    } \
    else if (this->mInputNumDimensions == 3) \
    { \
        NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 3 >::setNthInputName( \
                this->mOtbProcess, idx, name); \
    }\
}

/** Constructors/Destructors
 */


NMRATBandMathImageFilterWrapper::NMRATBandMathImageFilterWrapper(QObject* parent)
{
	this->setParent(parent);
	this->ctx = "NMRATBandMathImageFilterWrapper";
	this->mbIsInitialised = false;
	this->setObjectName(tr("NMRATBandMathImageFilterWrapper"));
	this->mInputComponentType = otb::ImageIOBase::FLOAT;
	this->mOutputComponentType = otb::ImageIOBase::FLOAT;
	this->mInputNumDimensions = 2;
	this->mOutputNumDimensions = 2;
	this->mInputNumBands = 1;
	this->mOutputNumBands = 1;
	this->mParamPos = 0;
    this->mUseTableColumnCache = false;
	this->mParameterHandling = NMProcess::NM_USE_UP;
}

NMRATBandMathImageFilterWrapper::~NMRATBandMathImageFilterWrapper(void)
{
}

void
NMRATBandMathImageFilterWrapper
::setNthAttributeTable(unsigned int idx,
		otb::AttributeTable::Pointer table,
		std::vector<std::string> tableColumns)
{
	if (!this->mbIsInitialised)
		return;

	std::vector<std::string>& colnames = tableColumns;
	switch(this->mInputComponentType)
	{
	MacroPerType( callSetNthAttributeTable, NMRATBandMathImageFilterWrapper_Internal )
	default:
		break;
	}
}

void
NMRATBandMathImageFilterWrapper
::setInternalNumExpression(unsigned int numExpr)
{
	if (!this->mbIsInitialised)
		return;

	switch(this->mInputComponentType)
	{
	MacroPerType( callSetNbExpr, NMRATBandMathImageFilterWrapper_Internal )
	default:
		break;
	}
}

void
NMRATBandMathImageFilterWrapper
::setInternalUseTableCache(bool useCache)
{
    if (!this->mbIsInitialised)
        return;

    switch(this->mInputComponentType)
    {
    MacroPerType( callSetUseTableCache, NMRATBandMathImageFilterWrapper_Internal )
    default:
        break;
    }
}

void
NMRATBandMathImageFilterWrapper
::setInternalNthInputName(unsigned int idx, const QString& varName)
{
    if (!this->mbIsInitialised)
        return;

    std::string name = varName.toStdString();

    switch(this->mInputComponentType)
    {
    MacroPerType( callSetNthInputName, NMRATBandMathImageFilterWrapper_Internal )
    default:
        break;
    }
}

void NMRATBandMathImageFilterWrapper::setInternalExpression(QString expression)
{
	if (!this->mbIsInitialised)
		return;

	switch (this->mInputComponentType)
	{
	MacroPerType( callSetExpression, NMRATBandMathImageFilterWrapper_Internal)
	default:
		break;
	}
}

void
NMRATBandMathImageFilterWrapper
::linkParameters(unsigned int step, const QMap<QString, NMModelComponent*>& repo)
{
	NMDebugCtx(ctx, << "...");

	int givenStep = step;
	// now let's set this process' special properties
	// set the calculation expression
//	step = this->mapHostIndexToPolicyIndex(givenStep, this->mMapExpressions.size());
//	QString currentExpression;
//	if (step < this->mMapExpressions.size())
//	{
//		currentExpression = this->mMapExpressions.at(step);//.toLower();
//		this->setInternalExpression(currentExpression);
//	}

    // this now takes care of pre-processing the current map expression
    // to fetch any constant epxression values (i.e. model parameters)
    // from other model components
    QVariant exprParam = this->getParameter("MapExpressions");
    QString currentExpression;
    if (exprParam.isValid())
    {
        currentExpression = exprParam.toString();
        this->setInternalExpression(currentExpression);

        QString curExrProvN = QString("nm:MapExpression=\"%1\"")
                              .arg(currentExpression);
        this->addRunTimeParaProvN(curExrProvN);
    }
    else
	{
		NMMfwException e(NMMfwException::NMProcess_MissingParameter);
        e.setSource(this->parent()->objectName().toStdString());
		e.setDescription("NMRATBandMathImageWrapper: No map expression specified!");
		throw e;

        NMLogError(<< ctx << ": no map expression available!");
        NMDebugCtx(ctx, << "done!");
		return;
	}

	step = this->mapHostIndexToPolicyIndex(givenStep, this->mNumExpressions.size());
	if (step < this->mNumExpressions.size())
	{
		bool bOK;
		int numExpr = this->mNumExpressions.at(step).toInt(&bOK);
		if (bOK)
        {
			this->setInternalNumExpression(numExpr);
            QString numExprProvN = QString("nm:NumExpressions=\"%1\"").arg(numExpr);
            this->addRunTimeParaProvN(numExprProvN);
        }
	}
	else
	{
		NMDebugAI(<< "no number of expressions given, so we assume,"
				<< " we've got one expression!" << std::endl);
	}

    this->setInternalUseTableCache(mUseTableColumnCache);

    NMModelController* ctrl = this->getModelController();
    if (ctrl == 0)
    {
        NMMfwException e(NMMfwException::NMModelController_UnregisteredModelComponent);
        e.setSource(this->parent()->objectName().toStdString());
        std::stringstream msg;
        msg << "no ModelController available!" << std::endl;

        NMLogError(<< ctx << msg .str() << std::endl);

        NMDebugCtx(ctx, << "done!");
        e.setDescription(msg.str());
        throw e;
    }

	// we go through every input image, check, whether a table is
	// available for this step, and then we identify the
	// columns uses in the currentExpression and pass their
	// names to the internal process object
	step = this->mapHostIndexToPolicyIndex(givenStep,
			this->mInputComponents.size());
	QStringList currentInputs;
	if (step < this->mInputComponents.size())
	{
		currentInputs = this->mInputComponents.at(step);
		int cnt = 0;
		foreach (const QString& input, currentInputs)
		{
            QString inputCompName = ctrl->getComponentNameFromInputSpec(input);
            NMModelComponent* comp = ctrl->getComponent(inputCompName);
            if (comp == 0)
            {
                NMMfwException e(NMMfwException::NMModelController_UnregisteredModelComponent);
                e.setSource(this->parent()->objectName().toStdString());
                std::stringstream msg;
                msg << "couldn't find " << inputCompName.toStdString() << "'!" << std::endl;

                NMLogError(<< ctx << msg .str() << std::endl);


                NMDebugCtx(ctx, << "done!");
                e.setDescription(msg.str());
                throw e;
            }

            // is this component taking part in this 'round'?
            NMSequentialIterComponent* sic = qobject_cast<NMSequentialIterComponent*>(comp);
            if (    sic != nullptr
                 && sic->evalNumIterationsExpression(step+1) == 0
               )
            {
                NMLogDebug(<< ctx << inputCompName.toStdString() << "::NumIterationExpression = 0 : we're skipping it!" );
                continue;
            }


            NMDebugAI(<< "img-name #" << cnt << ": " << inputCompName.toStdString() << std::endl;)

            // make sure the input is linked into the pipeline properly, otherwise, we might get
            // an exception here, especially if a sink process component is being executed individually
            // for testing purposes
            comp->linkComponents(step, repo);

            QSharedPointer<NMItkDataObjectWrapper> dw = ctrl->getOutputFromSource(input);
            if (dw.isNull())
			{
                ++cnt;
                continue;
			}

			otb::AttributeTable::Pointer tab = dw->getOTBTab();
			std::vector<std::string> vcolnames;
			if (tab.IsNotNull())
			{
                QStringList colNamesProvN;
				for (int c=0; c < tab->GetNumCols(); ++c)
				{
					std::string colname = tab->GetColumnName(c);
					QString cn(colname.c_str());
					//cn = cn.toLower();
					if (currentExpression.contains(cn, Qt::CaseInsensitive))
					{
						vcolnames.push_back(cn.toStdString());
                        colNamesProvN << cn;
					}
				}
                if (vcolnames.size() > 0)
                {
                    this->setNthAttributeTable(cnt, tab, vcolnames);

                    QString colNamesProvNAttr = QString("nm:TableColumns-%1=\"%2\"")
                                                .arg(cnt)
                                                .arg(colNamesProvN.join(" "));
                    this->addRunTimeParaProvN(colNamesProvNAttr);
                }
			}


            // replace any LookupParameters with their actual formula value
            // internal func wich looks after that!


            // we check, whether the input has a UserID defined, and if so, we set it as the
            // nth input variable name
            if (!comp->getUserID().isEmpty())
            {
                this->setInternalNthInputName(cnt, comp->getUserID());
                QString inputNameProvNAttr = QString("nm:InputUserID-%1=\"%2\"")
                                             .arg(cnt)
                                             .arg(comp->getUserID());
                this->addRunTimeParaProvN(inputNameProvNAttr);
            }
            else
            {
                NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                e.setSource(this->parent()->objectName().toStdString());
                std::stringstream msg;
                msg << "'" << inputCompName.toStdString() << "'"
                    << " Missing UserID!";
                e.setDescription(msg.str());
                NMLogError(<< ctx << msg .str());
                NMDebugCtx(ctx, << "done!");
                throw e;
            }
			++cnt;
		}
	}


	NMDebugCtx(ctx, << "done!");
}


