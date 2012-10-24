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
#include "NMRATBandMathImageFilterWrapper.h"
#include "NMImageReader.h"
#include "NMModelComponent.h"
#include "NMImageLayer.h"

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

	static void setNthAttributeTable(itk::ProcessObject::Pointer& otbFilter,
			unsigned int idx, otb::AttributeTable* tab, std::vector<std::string>& varNames)
		{
			FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
			filter->SetNthAttributeTable(idx, tab, varNames);
		}

	static itk::DataObject* getOutput(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands)
		{
			FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
			return dynamic_cast<ImgType*>(filter->GetOutput());
		}
};

GetOutputWrap( NMRATBandMathImageFilterWrapper, NMRATBandMathImageFilterWrapper_Internal )
InstantiateObjectWrap( NMRATBandMathImageFilterWrapper, NMRATBandMathImageFilterWrapper_Internal )
SetNthInputWrap( NMRATBandMathImageFilterWrapper, NMRATBandMathImageFilterWrapper_Internal )


#define callSetNthAttributeTable( filterPixelType, wrapName )											\
if (this->mInputNumDimensions == 2)                                                         \
{                                                                                           \
	wrapName<filterPixelType, filterPixelType, 2>::setNthAttributeTable(     \
			this->mOtbProcess, idx, table, colnames);                                          \
}                                                                                           \
else if (this->mInputNumDimensions == 3)                                                    \
{                                                                                           \
	wrapName<filterPixelType, filterPixelType, 3>::setNthAttributeTable(     \
			this->mOtbProcess, idx, table, colnames);                                          \
}                                                                                           \



#define callSetExpression( filterPixelType ) \
{ \
	if (this->mInputNumDimensions == 2) \
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


/** Constructors/Destructors
 */


NMRATBandMathImageFilterWrapper::NMRATBandMathImageFilterWrapper(QObject* parent)
{
	this->setParent(parent);
	this->ctx = "NMRATBandMathImageFilterWrapper";
	this->mbIsInitialised = false;
	this->setObjectName(tr("NMRATBandMathImageFilterWrapper"));
	this->mInputComponentType = itk::ImageIOBase::FLOAT;
	this->mOutputComponentType = itk::ImageIOBase::FLOAT;
	this->mInputNumDimensions = 2;
	this->mOutputNumDimensions = 2;
	this->mInputNumBands = 1;
	this->mOutputNumBands = 1;
	this->mParamPos = 0;
	this->mParameterHandling = NMProcess::NM_USE_UP;
}

NMRATBandMathImageFilterWrapper::~NMRATBandMathImageFilterWrapper(void)
{
}

void
NMRATBandMathImageFilterWrapper
::setNthAttributeTable(unsigned int idx,
		otb::AttributeTable* table,
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

//void NMRATBandMathImageFilterWrapper::setNthInput(unsigned int numInput,
//		NMItkDataObjectWrapper* imgWrapper)
//{
//	if (!this->mbIsInitialised)
//		return;
//
//	itk::DataObject* img = imgWrapper->getDataObject();
//	switch (this->mInputComponentType)
//	{
//	case itk::ImageIOBase::UCHAR:
//        callSetInput( unsigned char );
//		break;
//	case itk::ImageIOBase::CHAR:
//		callSetInput( char );
//		break;
//	case itk::ImageIOBase::USHORT:
//		callSetInput( unsigned short );
//		break;
//	case itk::ImageIOBase::SHORT:
//		callSetInput( short );
//		break;
//	case itk::ImageIOBase::UINT:
//		callSetInput( unsigned int );
//		break;
//	case itk::ImageIOBase::INT:
//		callSetInput( int );
//		break;
//	case itk::ImageIOBase::ULONG:
//		callSetInput( unsigned long );
//		break;
//	case itk::ImageIOBase::LONG:
//		callSetInput( long );
//		break;
//	case itk::ImageIOBase::FLOAT:
//		callSetInput( float );
//		break;
//	case itk::ImageIOBase::DOUBLE:
//		callSetInput( double );
//		break;
//	default:
//		break;
//	}
//}



//NMItkDataObjectWrapper* NMRATBandMathImageFilterWrapper::getOutput(void)
//{
//	GetOutputWrap()
//}

void NMRATBandMathImageFilterWrapper::setExpression(QString expression)
{
	if (!this->mbIsInitialised)
		return;

	switch (this->mInputComponentType)
	{
	case itk::ImageIOBase::UCHAR:
		callSetExpression( unsigned char );
		break;
	case itk::ImageIOBase::CHAR:
		callSetExpression( char );
		break;
	case itk::ImageIOBase::USHORT:
		callSetExpression( unsigned short );
		break;
	case itk::ImageIOBase::SHORT:
		callSetExpression( short );
		break;
	case itk::ImageIOBase::UINT:
		callSetExpression( unsigned int );
		break;
	case itk::ImageIOBase::INT:
		callSetExpression( int );
		break;
	case itk::ImageIOBase::ULONG:
		callSetExpression( unsigned long );
		break;
	case itk::ImageIOBase::LONG:
		callSetExpression( long );
		break;
	case itk::ImageIOBase::FLOAT:
		callSetExpression( float );
		break;
	case itk::ImageIOBase::DOUBLE:
		callSetExpression( double );
		break;
	default:
		break;
	}
}

void
NMRATBandMathImageFilterWrapper
::linkParameters(unsigned int step, const QMap<QString, NMModelComponent*>& repo)
{
	NMDebugCtx(ctx, << "...");

//	if (step > 0)
//	{
//		this->mbIsInitialised = false;
//		this->instantiateObject();
//	}

	if (step > this->mMapExpressions.size()-1)
		step = 0;

	// now let's set this process' special properties
	// set the calculation expression
	if (step < this->mMapExpressions.size())
	{
		this->setExpression(this->mMapExpressions.at(step));
	}
	else
	{
		NMErr(ctx, << "no map expression available!");
		return;
	}


	// let's see whether we've got some tables and associated variable names
	// in any case, we need the same amount of tables and table column names
	// and the parameter pos has to point to a valid position within those
	// lists
	if ((step < this->mInputTables.size())   	 &&
		(step < this->mInputTableVarNames.size()) &&
		(this->mInputTableVarNames.size() == this->mInputTables.size())
	   )
	{
		QStringList inputTableComps = this->mInputTables.at(step);
		QList<QStringList> inputTabVarNamesList = this->mInputTableVarNames.at(
				step);

		// check, whether both lists have the same number of entries, otherwise
		// we have to pull out
		if (inputTableComps.size() == inputTabVarNamesList.size())
		{
			for (unsigned int iT = 0; iT < inputTableComps.size(); ++iT)
			{
				QString tablecompname = inputTableComps.at(iT);
				if (tablecompname.isEmpty())
					return;

				// get the associated table object from the named component
				QMap<QString, NMModelComponent*>::const_iterator compIt =
						repo.find(tablecompname);

				otb::AttributeTable::Pointer table;// = 0;
				NMImageLayer* layer = qobject_cast<NMImageLayer*>(compIt.value());
				if (layer != 0)
				{
					table = layer->getRasterAttributeTable(1);
				}
				else
				{
					NMImageReader* reader = qobject_cast<NMImageReader*>(
							compIt.value()->getProcess());
					if (reader != 0)
					{
						table = reader->getRasterAttributeTable(1);//.GetPointer();
					}
				}

				// now let's get the variable names
				if (table.IsNotNull())// != 0)
				{
					QStringList varNames = inputTabVarNamesList.at(iT);
					if (varNames.size() != 0)
					{
						std::vector<QString> qcolnames = varNames.toVector().toStdVector();
						std::vector<std::string> colnames;
						colnames.resize(qcolnames.size());

						for (unsigned int qn = 0; qn < qcolnames.size(); ++qn)
							colnames[qn] = qcolnames[qn].toStdString();

						// at the end we've got one call to setnthattributetable
						this->setNthAttributeTable(iT, table, colnames);
					}
				}
			}
		}
	}

	NMDebugCtx(ctx, << "done!");
}
