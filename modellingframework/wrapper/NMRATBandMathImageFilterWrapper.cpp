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
//#include "NMImageLayer.h"
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
	step = this->mapHostIndexToPolicyIndex(givenStep, this->mMapExpressions.size());
	QString currentExpression;
	if (step < this->mMapExpressions.size())
	{
		currentExpression = this->mMapExpressions.at(step);//.toLower();
		this->setInternalExpression(currentExpression);
	}
	else
	{
		NMMfwException e(NMMfwException::NMProcess_MissingParameter);
		e.setMsg("NMRATBandMathImageWrapper: No map expression specified!");
		throw e;

		NMErr(ctx, << "no map expression available!");
        NMDebugCtx(ctx, << "done!");
		return;
	}

	step = this->mapHostIndexToPolicyIndex(givenStep, this->mNumExpressions.size());
	if (step < this->mNumExpressions.size())
	{
		bool bOK;
		int numExpr = this->mNumExpressions.at(step).toInt(&bOK);
		if (bOK)
			this->setInternalNumExpression(numExpr);
	}
	else
	{
		NMDebugAI(<< "no number of expressions given, so we assume,"
				<< " we've got one expression!" << std::endl);
	}

    this->setInternalUseTableCache(mUseTableColumnCache);

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
            QString inputCompName = NMModelController::getComponentNameFromInputSpec(input);
            NMModelComponent* comp = NMModelController::getInstance()->getComponent(inputCompName);
            if (comp == 0)
            {
                NMMfwException e(NMMfwException::NMModelController_UnregisteredModelComponent);
                std::stringstream msg;
                msg << "'" << inputCompName.toStdString() << "'";
                e.setMsg(msg.str());
                NMDebugCtx(ctx, << "done!");
                throw e;
            }

            NMDebugAI(<< "img-name #" << cnt << ": " << inputCompName.toStdString() << std::endl;)

            // make sure the input is linked into the pipeline properly, otherwise, we might get
            // an exception here, especially if a sink process component is being executed individually
            // for testing purposes
            comp->linkComponents(step, repo);

            QSharedPointer<NMItkDataObjectWrapper> dw = NMModelController::getInstance()->getOutputFromSource(input);
            if (dw.isNull())
			{
                ++cnt;
                continue;
			}

			otb::AttributeTable::Pointer tab = dw->getOTBTab();
			std::vector<std::string> vcolnames;
			if (tab.IsNotNull())
			{
				for (int c=0; c < tab->GetNumCols(); ++c)
				{
					std::string colname = tab->GetColumnName(c);
					QString cn(colname.c_str());
					//cn = cn.toLower();
					if (currentExpression.contains(cn, Qt::CaseInsensitive))
					{
						vcolnames.push_back(cn.toStdString());
					}
				}
                this->setNthAttributeTable(cnt, tab, vcolnames);
			}

            // we check, whether the input has a UserID defined, and if so, we set it as the
            // nth input variable name
            if (!comp->getUserID().isEmpty())
            {
                this->setInternalNthInputName(cnt, comp->getUserID());
            }
            else
            {
                NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                std::stringstream msg;
                msg << "'" << inputCompName.toStdString() << "'"
                    << " Missing UserID!";
                e.setMsg(msg.str());
                NMDebugCtx(ctx, << "done!");
                throw e;
            }
			++cnt;
		}
	}

	//we've got some tables and associated variable names
	// in any case, we need the same amount of tables and table column names
	// and the parameter pos has to point to a valid position within those
	// lists
	//if ((step < this->mInputTables.size())   	 &&
	//	(step < this->mInputTableVarNames.size()) &&
	//	(this->mInputTableVarNames.size() == this->mInputTables.size())
	//   )
	//{
	//	QStringList inputTableComps = this->mInputTables.at(step);
	//	QList<QStringList> inputTabVarNamesList = this->mInputTableVarNames.at(
	//			step);
    //
	//	// check, whether both lists have the same number of entries, otherwise
	//	// we have to pull out
	//	if (inputTableComps.size() == inputTabVarNamesList.size())
	//	{
	//		for (unsigned int iT = 0; iT < inputTableComps.size(); ++iT)
	//		{
	//			QString tablecompname = inputTableComps.at(iT);
	//			if (tablecompname.isEmpty())
	//				return;
    //
	//			// get the associated table object from the named component
	//			QMap<QString, NMModelComponent*>::const_iterator compIt =
	//					repo.find(tablecompname);
    //
	//			otb::AttributeTable::Pointer table;// = 0;
	//			NMImageLayer* layer = qobject_cast<NMImageLayer*>(compIt.value());
	//			if (layer != 0)
	//			{
	//				table = layer->getRasterAttributeTable(1);
	//			}
	//			else
	//			{
	//				NMIterableComponent* ic =
	//						qobject_cast<NMIterableComponent*>(compIt.value());
	//				if (ic != 0 && ic->getProcess() != 0)
	//				{
	//					NMImageReader* reader = qobject_cast<NMImageReader*>(
	//							ic->getProcess());
	//					if (reader != 0)
	//					{
	//						table = reader->getRasterAttributeTable(1);//.GetPointer();
	//					}
	//				}
	//			}
    //
	//			// now let's get the variable names
	//			if (table.IsNotNull())// != 0)
	//			{
	//				QStringList varNames = inputTabVarNamesList.at(iT);
	//				if (varNames.size() != 0)
	//				{
	//					std::vector<QString> qcolnames = varNames.toVector().toStdVector();
	//					std::vector<std::string> colnames;
	//					colnames.resize(qcolnames.size());
    //
	//					for (unsigned int qn = 0; qn < qcolnames.size(); ++qn)
	//						colnames[qn] = qcolnames[qn].toStdString();
    //
	//					// at the end we've got one call to setnthattributetable
	//					this->setNthAttributeTable(iT, table, colnames);
	//				}
	//			}
	//			else
	//			{
	//				NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
	//				e.setMsg("NMRATBandMathImageWrapper: Invalid attribute table input parameter!");
	//				throw e;
	//			}
	//		}
	//	}
	//}

	NMDebugCtx(ctx, << "done!");
}
