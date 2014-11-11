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
 * NMMacros.h
 *
 *  Created on: 18/05/2012
 *      Author: alex
 *
 */

#ifndef NMMACROS_H_
#define NMMACROS_H_

#include "nmlog.h"

using namespace std;


/** >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 *  >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * 	    QT-BASED CONVENIENCE MACROS FOR THE LAZY DEVELOPER
 *  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
 *  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
 */

/**
 * VISIBLE USER FEEDBACK (i.e. MessageBox calls)
 *
 */
#define NMBoxInfo( title, message )       \
std::stringstream strtitle;               \
strtitle << title;                        \
std::stringstream strmsg;                 \
strmsg << message;                        \
QMessageBox::information(this,            \
		QString(strtitle.str().c_str()),  \
		QString(strmsg.str().c_str()));

#define NMBoxWarn( title, message )       \
std::stringstream strtitle;               \
strtitle << title;                        \
std::stringstream strmsg;                 \
strmsg << message;                        \
QMessageBox::warning(this,                \
		QString(strtitle.str().c_str()),  \
		QString(strmsg.str().c_str()));

#define NMBoxErr( title, message )        \
std::stringstream strtitle;               \
strtitle << title;                        \
std::stringstream strmsg;                 \
strmsg << message;                        \
QMessageBox::critical(this,               \
		QString(strtitle.str().c_str()),  \
		QString(strmsg.str().c_str()));


/**
 * PROPERTY GETTER & SETTER METHODS
 */

#define NMPropertyGetSet( name, type ) \
void set ## name(type _arg) \
{ \
	this->m ## name = _arg; \
    emit nmChanged();\
}	\
type get ## name() \
{ \
	return this->m ## name; \
}

/** >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 *  >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * 	   NMPROCESS-RELATED MACROS TO FACILITATE TEMPLATE HANDLING
 *  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
 *  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
 */

/** ==============================================================
 * 					setRAT
 *  ==============================================================
 */

#define callSetRAT( inputType, outputType, wrapName ) \
{ \
    if (this->mInputNumDimensions == 1)		\
    {		\
        wrapName< inputType, outputType, 1>::setRAT(this->mOtbProcess, \
                this->mInputNumBands, numInput, rat);	\
    }		\
    else if (this->mInputNumDimensions == 2) \
    { \
        wrapName< inputType, outputType, 2 >::setRAT(this->mOtbProcess, \
                this->mInputNumBands, numInput, rat);	\
    } \
    else if (this->mInputNumDimensions == 3) \
    { \
        wrapName< inputType, outputType, 3 >::setRAT(this->mOtbProcess, \
                this->mInputNumBands, numInput, rat);	\
    }\
}

#define callOutputTypeSetRAT( outputType, wrapName ) \
{ \
    if (this->mOutputNumDimensions == 1)		\
    {		\
        wrapName< outputType, 1>::setRAT(this->mOtbProcess, \
                this->mOutputNumBands, numInput, rat);	\
    }		\
    else if (this->mOutputNumDimensions == 2) \
    { \
        wrapName< outputType, 2 >::setRAT(this->mOtbProcess, \
                this->mOutputNumBands, numInput, rat);	\
    } \
    else if (this->mOutputNumDimensions == 3) \
    { \
        wrapName< outputType, 3 >::setRAT(this->mOtbProcess, \
                this->mOutputNumBands, numInput, rat);	\
    }\
}

#define callInputTypeSetRAT( inputType, wrapName ) \
{ \
    if (this->mInputNumDimensions == 1)		\
    {		\
        wrapName< inputType, 1>::setRAT(this->mOtbProcess, \
                this->mInputNumBands, numInput, rat);	\
    }		\
    else if (this->mInputNumDimensions == 2) \
    { \
        wrapName< inputType, 2 >::setRAT(this->mOtbProcess, \
                this->mInputNumBands, numInput, rat);	\
    } \
    else if (this->mInputNumDimensions == 3) \
    { \
        wrapName< inputType, 3 >::setRAT(this->mOtbProcess, \
                this->mInputNumBands, numInput, rat);	\
    }\
}

/** ==============================================================
 * 					getRAT
 *  ==============================================================
 */

#define callGetRAT( inputType, outputType, wrapName ) \
{ \
    if (this->mInputNumDimensions == 1)		\
    {		\
        rat = wrapName< inputType, outputType, 1>::getRAT(this->mOtbProcess, \
                this->mInputNumBands, idx);	\
    }		\
    else if (this->mInputNumDimensions == 2) \
    { \
        rat = wrapName< inputType, outputType, 2 >::getRAT(this->mOtbProcess, \
                this->mInputNumBands, idx);	\
    } \
    else if (this->mInputNumDimensions == 3) \
    { \
        rat = wrapName< inputType, outputType, 3 >::getRAT(this->mOtbProcess, \
                this->mInputNumBands, idx);	\
    }\
}

#define callOutputTypeGetRAT( outputType, wrapName ) \
{ \
    if (this->mOutputNumDimensions == 1)		\
    {		\
        rat = wrapName< outputType, 1>::getRAT(this->mOtbProcess, \
                this->mOutputNumBands, idx);	\
    }		\
    else if (this->mOutputNumDimensions == 2) \
    { \
        rat = wrapName< outputType, 2 >::getRAT(this->mOtbProcess, \
                this->mOutputNumBands, idx);	\
    } \
    else if (this->mOutputNumDimensions == 3) \
    { \
        rat = wrapName< outputType, 3 >::getRAT(this->mOtbProcess, \
                this->mOutputNumBands, idx);	\
    }\
}

#define callInputTypeGetRAT( inputType, wrapName ) \
{ \
    if (this->mInputNumDimensions == 1)		\
    {		\
        rat = wrapName< inputType, 1>::getRAT(this->mOtbProcess, \
                this->mInputNumBands, idx);	\
    }		\
    else if (this->mInputNumDimensions == 2) \
    { \
        rat = wrapName< inputType, 2 >::getRAT(this->mOtbProcess, \
                this->mInputNumBands, idx);	\
    } \
    else if (this->mInputNumDimensions == 3) \
    { \
        rat = wrapName< inputType, 3 >::getRAT(this->mOtbProcess, \
                this->mInputNumBands, idx);	\
    }\
}

/** ==============================================================
 * 					getOutput
 *  ==============================================================
 */

#define callGetOutput( inputType, outputType, wrapName ) \
{ \
	if (this->mInputNumDimensions == 1)		\
	{		\
		img = wrapName< inputType, outputType, 1>::getOutput(this->mOtbProcess, \
				this->mInputNumBands, idx);	\
	}		\
	else if (this->mInputNumDimensions == 2) \
	{ \
		img = wrapName< inputType, outputType, 2 >::getOutput(this->mOtbProcess, \
				this->mInputNumBands, idx);	\
	} \
	else if (this->mInputNumDimensions == 3) \
	{ \
		img = wrapName< inputType, outputType, 3 >::getOutput(this->mOtbProcess, \
				this->mInputNumBands, idx);	\
	}\
}

#define callOutputTypeGetOutput( outputType, wrapName ) \
{ \
	if (this->mOutputNumDimensions == 1)		\
	{		\
		img = wrapName< outputType, 1>::getOutput(this->mOtbProcess, \
				this->mOutputNumBands, idx);	\
	}		\
	else if (this->mOutputNumDimensions == 2) \
	{ \
		img = wrapName< outputType, 2 >::getOutput(this->mOtbProcess, \
				this->mOutputNumBands, idx);	\
	} \
	else if (this->mOutputNumDimensions == 3) \
	{ \
		img = wrapName< outputType, 3 >::getOutput(this->mOtbProcess, \
				this->mOutputNumBands, idx);	\
	}\
}

#define callInputTypeGetOutput( inputType, wrapName ) \
{ \
	if (this->mInputNumDimensions == 1)		\
	{		\
		img = wrapName< inputType, 1>::getOutput(this->mOtbProcess, \
				this->mInputNumBands, idx);	\
	}		\
	else if (this->mInputNumDimensions == 2) \
	{ \
		img = wrapName< inputType, 2 >::getOutput(this->mOtbProcess, \
				this->mInputNumBands, idx);	\
	} \
	else if (this->mInputNumDimensions == 3) \
	{ \
		img = wrapName< inputType, 3 >::getOutput(this->mOtbProcess, \
				this->mInputNumBands, idx);	\
	}\
}

/** ==============================================================
 * 					createInstance
 *  ==============================================================
 */
#define callCreator( inputType, outputType, wrapName ) \
{ \
	if (this->mInputNumDimensions == 1)		\
	{		\
		wrapName< inputType, outputType, 1>::createInstance(this->mOtbProcess, \
				this->mInputNumBands);	\
	}		\
	else if (this->mInputNumDimensions == 2) \
	{ \
		wrapName< inputType, outputType, 2 >::createInstance(this->mOtbProcess, \
				this->mInputNumBands);	 \
	} \
	else if (this->mInputNumDimensions == 3) \
	{ \
		wrapName< inputType, outputType, 3 >::createInstance(this->mOtbProcess, \
				this->mInputNumBands);	 \
	}\
}

#define callInputTypeCreator( inputType, wrapName ) \
{ \
	if (this->mInputNumDimensions == 1)		\
	{		\
		wrapName< inputType, 1>::createInstance(this->mOtbProcess, \
				this->mInputNumBands);	\
	}		\
	else if (this->mInputNumDimensions == 2) \
	{ \
		wrapName< inputType, 2 >::createInstance(this->mOtbProcess, \
				this->mInputNumBands);	 \
	} \
	else if (this->mInputNumDimensions == 3) \
	{ \
		wrapName< inputType, 3 >::createInstance(this->mOtbProcess, \
				this->mInputNumBands);	 \
	}\
}

#define callOutputTypeCreator( outputType, wrapName ) \
{ \
	if (this->mOutputNumDimensions == 1)		\
	{		\
		wrapName< outputType, 1>::createInstance(this->mOtbProcess, \
				this->mOutputNumBands);	\
	}		\
	else if (this->mOutputNumDimensions == 2) \
	{ \
		wrapName< outputType, 2 >::createInstance(this->mOtbProcess, \
				this->mOutputNumBands);	 \
	} \
	else if (this->mOutputNumDimensions == 3) \
	{ \
		wrapName< outputType, 3 >::createInstance(this->mOtbProcess, \
				this->mOutputNumBands);	 \
	}\
}

/** ==============================================================
 * 					setNthInput
 *  ==============================================================
 */
#define callSetInput( inputType, outputType, wrapName ) \
{ \
	if (this->mInputNumDimensions == 1) \
	{ \
		wrapName< inputType, outputType, 1 >::setNthInput( \
				this->mOtbProcess, this->mInputNumBands, numInput, img); \
	} \
	else if (this->mInputNumDimensions == 2) \
	{ \
		wrapName< inputType, outputType, 2 >::setNthInput( \
				this->mOtbProcess, this->mInputNumBands, numInput, img); \
	} \
	else if (this->mInputNumDimensions == 3) \
	{ \
		wrapName< inputType, outputType, 3 >::setNthInput( \
				this->mOtbProcess, this->mInputNumBands, numInput, img); \
	}\
}

#define callInputTypeSetInput( imgType, wrapName ) \
{ \
	if (this->mInputNumDimensions == 1) \
	{ \
		wrapName< imgType, 1 >::setNthInput( \
				this->mOtbProcess, this->mInputNumBands, numInput, img); \
	} \
	else if (this->mInputNumDimensions == 2) \
	{ \
		wrapName< imgType, 2 >::setNthInput( \
				this->mOtbProcess, this->mInputNumBands, numInput, img); \
	} \
	else if (this->mInputNumDimensions == 3) \
	{ \
		wrapName< imgType, 3 >::setNthInput( \
				this->mOtbProcess, this->mInputNumBands, numInput, img); \
	}\
}

#define callOutputTypeSetInput( imgType, wrapName ) \
{ \
	if (this->mOutputNumDimensions == 1) \
	{ \
		wrapName< imgType, 1 >::setNthInput( \
				this->mOtbProcess, this->mInputNumBands, numInput, img); \
	} \
	else if (this->mOutputNumDimensions == 2) \
	{ \
		wrapName< imgType, 2 >::setNthInput( \
				this->mOtbProcess, this->mInputNumBands, numInput, img); \
	} \
	else if (this->mOutputNumDimensions == 3) \
	{ \
		wrapName< imgType, 3 >::setNthInput( \
				this->mOtbProcess, this->mInputNumBands, numInput, img); \
	}\
}

/** ==============================================================
 * 					internalLinkParameters
 *  ==============================================================
 */
#define callInternalLinkParameters( inputType, outputType, wrapName ) \
{ \
	if (this->mInputNumDimensions == 1) \
	{ \
		wrapName< inputType, outputType, 1 >::internalLinkParameters( \
				this->mOtbProcess, this->mInputNumBands, \
				this, step, repo); \
	} \
	else if (this->mInputNumDimensions == 2) \
	{ \
		wrapName< inputType, outputType, 2 >::internalLinkParameters( \
				this->mOtbProcess, this->mInputNumBands, \
				this, step, repo); \
	} \
	else if (this->mInputNumDimensions == 3) \
	{ \
		wrapName< inputType, outputType, 3 >::internalLinkParameters( \
				this->mOtbProcess, this->mInputNumBands, \
				this, step, repo); \
	}\
}


#define callInputTypeInternalLinkParameters( inputType, wrapName ) \
{ \
	if (this->mInputNumDimensions == 1) \
	{ \
		wrapName< inputType, 1 >::internalLinkParameters( \
				this->mOtbProcess, this->mInputNumBands, \
				this, step, repo); \
	} \
	else if (this->mInputNumDimensions == 2) \
	{ \
		wrapName< inputType, 2 >::internalLinkParameters( \
				this->mOtbProcess, this->mInputNumBands, \
				this, step, repo); \
	} \
	else if (this->mInputNumDimensions == 3) \
	{ \
		wrapName< inputType, 3 >::internalLinkParameters( \
				this->mOtbProcess, this->mInputNumBands, \
				this, step, repo); \
	}\
}


/** ==============================================================
 * 					internalExecute
 *  ==============================================================
 */
#define callInputTypeInternalExecute( inputType, wrapName )	\
{ \
	if (this->mInputNumDimensions == 1) \
	{ \
		wrapName< inputType, 1 >::execute( \
				this->mOtbProcess, this->mInputNumBands, this); \
	} \
	else if (this->mInputNumDimensions == 2) \
	{ \
		wrapName< inputType, 2 >::execute( \
				this->mOtbProcess, this->mInputNumBands, this); \
	} \
	else if (this->mInputNumDimensions == 3) \
	{ \
		wrapName< inputType, 3 >::execute( \
				this->mOtbProcess, this->mInputNumBands, this); \
	}\
}


// **********************************************************************************
/*  ===============================================================================
 *	  Local Macro per Type : 'SINGLE' TYPE SWITCH ONLY MACRO
 *	===============================================================================
 */

#define LocalMacroPerSingleType( macroName ) \
    case otb::ImageIOBase::UCHAR:                                               \
        macroName( unsigned char );                                         \
        break;                                                                  \
    case otb::ImageIOBase::CHAR:                                                \
        macroName( char );                                                  \
        break;                                                                  \
    case otb::ImageIOBase::USHORT:                                              \
        macroName( unsigned short );                                        \
        break;                                                                  \
    case otb::ImageIOBase::SHORT:                                               \
        macroName( short );                                                 \
        break;                                                                  \
    case otb::ImageIOBase::UINT:                                                \
        macroName( unsigned int );                                          \
        break;                                                                  \
    case otb::ImageIOBase::INT:                                                 \
        macroName( int );                                                   \
        break;                                                                  \
    case otb::ImageIOBase::ULONG:                                               \
        macroName( unsigned long );                                         \
        break;                                                                  \
    case otb::ImageIOBase::LONG:                                                \
        macroName( long );                                                  \
        break;                                                                  \
    case otb::ImageIOBase::FLOAT:                                               \
        macroName( float );                                                 \
        break;                                                                  \
    case otb::ImageIOBase::DOUBLE:                                              \
        macroName( double );                                                \
        break;


/*  ===============================================================================
 *	  'OUTER' (ie INPUT) TYPE SWITCH MACRO
 *	===============================================================================
 */

#define MacroPerType( macroName, wrapName ) \
	case otb::ImageIOBase::UCHAR:                                               \
		macroName( unsigned char, wrapName );                                         \
		break;                                                                  \
	case otb::ImageIOBase::CHAR:                                                \
		macroName( char, wrapName );                                                  \
		break;                                                                  \
	case otb::ImageIOBase::USHORT:                                              \
		macroName( unsigned short, wrapName );                                        \
		break;                                                                  \
	case otb::ImageIOBase::SHORT:                                               \
		macroName( short, wrapName );                                                 \
		break;                                                                  \
	case otb::ImageIOBase::UINT:                                                \
		macroName( unsigned int, wrapName );                                          \
		break;                                                                  \
	case otb::ImageIOBase::INT:                                                 \
		macroName( int, wrapName );                                                   \
		break;                                                                  \
	case otb::ImageIOBase::ULONG:                                               \
		macroName( unsigned long, wrapName );                                         \
		break;                                                                  \
	case otb::ImageIOBase::LONG:                                                \
		macroName( long, wrapName );                                                  \
		break;                                                                  \
	case otb::ImageIOBase::FLOAT:                                               \
		macroName( float, wrapName );                                                 \
		break;                                                                  \
	case otb::ImageIOBase::DOUBLE:                                              \
		macroName( double, wrapName );                                                \
		break;

/*  ===============================================================================
 *  NESTED 'INNER' (ie OUTPUT) TYPE SWITCH MACRO
 *  ===============================================================================
 */
#define outputTypeSwitch( inputType, macroName, wrapperClassName ) \
switch(this->mOutputComponentType)	\
{		\
case otb::ImageIOBase::UCHAR:                                               \
	macroName( inputType, unsigned char, wrapperClassName );                                         \
	break;                                                                  \
case otb::ImageIOBase::CHAR:                                                \
	macroName( inputType, char, wrapperClassName );                                                  \
	break;                                                                  \
case otb::ImageIOBase::USHORT:                                              \
	macroName( inputType, unsigned short, wrapperClassName );                                        \
	break;                                                                  \
case otb::ImageIOBase::SHORT:                                               \
	macroName( inputType, short, wrapperClassName );                                                 \
	break;                                                                  \
case otb::ImageIOBase::UINT:                                                \
	macroName( inputType, unsigned int, wrapperClassName );                                          \
	break;                                                                  \
case otb::ImageIOBase::INT:                                                 \
	macroName( inputType, int, wrapperClassName );                                                   \
	break;                                                                  \
case otb::ImageIOBase::ULONG:                                               \
	macroName( inputType, unsigned long, wrapperClassName );                                         \
	break;                                                                  \
case otb::ImageIOBase::LONG:                                                \
	macroName( inputType, long, wrapperClassName );                                                  \
	break;                                                                  \
case otb::ImageIOBase::FLOAT:                                               \
	macroName( inputType, float, wrapperClassName );                                                 \
	break;                                                                  \
case otb::ImageIOBase::DOUBLE:                                              \
	macroName( inputType, double, wrapperClassName );                                                \
	break;	\
default:	\
	break;	\
}

// **********************************************************************************


/**
 *  VIRTUAL METHOD BODY MACROS for ever recurring functionality
 */

#define LinkInputTypeInternalParametersWrap( ClassName, wrapName)		\
void ClassName::linkParameters(unsigned int step,                       \
		const QMap<QString, NMModelComponent*>& repo)                   \
{                                                                       \
	if (!this->mbIsInitialised)                                         \
		return;                                                         \
	switch(this->mInputComponentType)                                   \
	{                                                                   \
	MacroPerType( callInputTypeInternalLinkParameters, wrapName )       \
	default:                                                            \
		break;                                                          \
	}                                                                   \
}

#define LinkInternalParametersWrap( ClassName, wrapName )                                              \
void ClassName::linkParameters(unsigned int step,							\
		const QMap<QString, NMModelComponent*>& repo)                        \
{                                                                               \
	if (!this->mbIsInitialised)                                                 \
	 return; \
	switch (this->mInputComponentType)                                         \
	{                                                                           \
	case otb::ImageIOBase::UCHAR:                                               \
		outputTypeSwitch( unsigned char, callInternalLinkParameters, wrapName );                                         \
		break;                                                                  \
	case otb::ImageIOBase::CHAR:                                                \
		outputTypeSwitch( char, callInternalLinkParameters, wrapName );                                                  \
		break;                                                                  \
	case otb::ImageIOBase::USHORT:                                              \
		outputTypeSwitch( unsigned short, callInternalLinkParameters, wrapName );                                        \
		break;                                                                  \
	case otb::ImageIOBase::SHORT:                                               \
		outputTypeSwitch( short, callInternalLinkParameters, wrapName );                                                 \
		break;                                                                  \
	case otb::ImageIOBase::UINT:                                                \
		outputTypeSwitch( unsigned int, callInternalLinkParameters, wrapName );                                          \
		break;                                                                  \
	case otb::ImageIOBase::INT:                                                 \
		outputTypeSwitch( int, callInternalLinkParameters, wrapName );                                                   \
		break;                                                                  \
	case otb::ImageIOBase::ULONG:                                               \
		outputTypeSwitch( unsigned long, callInternalLinkParameters, wrapName );                                         \
		break;                                                                  \
	case otb::ImageIOBase::LONG:                                                \
		outputTypeSwitch( long, callInternalLinkParameters, wrapName );                                                  \
		break;                                                                  \
	case otb::ImageIOBase::FLOAT:                                               \
		outputTypeSwitch( float, callInternalLinkParameters, wrapName );                                                 \
		break;                                                                  \
	case otb::ImageIOBase::DOUBLE:                                              \
		outputTypeSwitch( double, callInternalLinkParameters, wrapName );                                                \
		break;																	\
	default:                                                                    \
		break;                                                                  \
	}                                                                           \
}



/** instantiation of process object, which is templated
 *  over INPUT and OUTPUT types
 */
#define InstantiateObjectWrap( ClassName, wrapName )                                              \
void ClassName::instantiateObject(void)        \
{                                                                               \
	if (this->mbIsInitialised)                                                 \
	 return; \
	bool init = true;               \
	switch (this->mInputComponentType)                                         \
	{                                                                           \
	case otb::ImageIOBase::UCHAR:                                               \
		outputTypeSwitch( unsigned char, callCreator, wrapName );                                         \
		break;                                                                  \
	case otb::ImageIOBase::CHAR:                                                \
		outputTypeSwitch( char, callCreator, wrapName );                                                  \
		break;                                                                  \
	case otb::ImageIOBase::USHORT:                                              \
		outputTypeSwitch( unsigned short, callCreator, wrapName );                                        \
		break;                                                                  \
	case otb::ImageIOBase::SHORT:                                               \
		outputTypeSwitch( short, callCreator, wrapName );                                                 \
		break;                                                                  \
	case otb::ImageIOBase::UINT:                                                \
		outputTypeSwitch( unsigned int, callCreator, wrapName );                                          \
		break;                                                                  \
	case otb::ImageIOBase::INT:                                                 \
		outputTypeSwitch( int, callCreator, wrapName );                                                   \
		break;                                                                  \
	case otb::ImageIOBase::ULONG:                                               \
		outputTypeSwitch( unsigned long, callCreator, wrapName );                                         \
		break;                                                                  \
	case otb::ImageIOBase::LONG:                                                \
		outputTypeSwitch( long, callCreator, wrapName );                                                  \
		break;                                                                  \
	case otb::ImageIOBase::FLOAT:                                               \
		outputTypeSwitch( float, callCreator, wrapName );                                                 \
		break;                                                                  \
	case otb::ImageIOBase::DOUBLE:                                              \
		outputTypeSwitch( double, callCreator, wrapName );                                                \
		break;																	\
	default:                                                                    \
		init = false;               \
		break;                                                                  \
	}                                                                           \
	this->mbIsInitialised = init;    \
	NMDebugAI( << "... " << this->objectName().toStdString() << " - " << init << std::endl); \
}

/** instantiation of process object, which is templated
 *  over OUTPUT type
 */
#define InstantiateOutputTypeObjectWrap( ClassName, WrapName)			\
void ClassName::instantiateObject(void)                                 \
{                                                                       \
	if (this->mbIsInitialised)                                          \
	 return;                                                           \
	bool init = true;                                                   \
	switch (this->mOutputComponentType)                                 \
	{                                                                   \
	MacroPerType( callOutputTypeCreator, WrapName)                       \
	default:                                                            \
		init = false;                                                   \
		break;                                                          \
	}                                                                   \
	this->mbIsInitialised = init;                                      \
	NMDebugAI( << "... " << this->objectName().toStdString()            \
			<< " - " << init << std::endl);                            \
}

/** instantiation of process object, which is templated
 *  over INPUT type
 */
#define InstantiateInputTypeObjectWrap( ClassName, WrapName)			\
void ClassName::instantiateObject(void)                                 \
{                                                                       \
	if (this->mbIsInitialised)                                          \
	 return;                                                           \
	bool init = true;                                                   \
	switch (this->mInputComponentType)                                 \
	{                                                                   \
	MacroPerType( callInputTypeCreator, WrapName)                       \
	default:                                                            \
		init = false;                                                   \
		break;                                                          \
	}                                                                   \
	this->mbIsInitialised = init;                                      \
	NMDebugAI( << "... " << this->objectName().toStdString()            \
			<< " - " << init << std::endl);                            \
}

/** get the output of the process object, which is templated
 *  over the OUTPUT component type of this wrapper class
 */
#define GetOutputTypeOutputWrap( ClassName, InternalWrapper )                     \
QSharedPointer<NMItkDataObjectWrapper> ClassName::getOutput(unsigned int idx)							      \
{                                                                                 \
    QSharedPointer<NMItkDataObjectWrapper> ret;                                    \
    ret.clear();                                                                    \
    if (!this->mbIsInitialised)                                                   \
        return ret;                                                                 \
                                                                                  \
	itk::DataObject* img = 0;                                                     \
                                                                                  \
	switch (this->mOutputComponentType)                                           \
	{                                                                             \
	MacroPerType( callOutputTypeGetOutput, InternalWrapper )                       \
	default:                                                                      \
		break;                                                                    \
	}                                                                             \
                                                                                  \
    QSharedPointer<NMItkDataObjectWrapper> dw(                                  \
			new NMItkDataObjectWrapper(this, img, this->mOutputComponentType,     \
            this->mOutputNumDimensions, this->mOutputNumBands));                   \
                                                                                  \
	return dw;																	  \
}

/** get the output of the process object, which is templated
 *  over the INPUT component type of this wrapper class
 */
#define GetInputTypeOutputWrap( ClassName, InternalWrapper )                     \
QSharedPointer<NMItkDataObjectWrapper> ClassName::getOutput(unsigned int idx)							      \
{                                                                                 \
    QSharedPointer<NMItkDataObjectWrapper> ret;                                    \
    ret.clear();                                                                    \
	if (!this->mbIsInitialised)                                                   \
        return ret;                                                                 \
                                                                                  \
	itk::DataObject* img = 0;                                                     \
                                                                                  \
	switch (this->mInputComponentType)                                           \
	{                                                                             \
	MacroPerType( callInputTypeGetOutput, InternalWrapper )                       \
	default:                                                                      \
		break;                                                                    \
	}                                                                             \
                                                                                  \
    QSharedPointer<NMItkDataObjectWrapper> dw(                                    \
			new NMItkDataObjectWrapper(this, img, this->mInputComponentType,     \
            this->mInputNumDimensions, this->mInputNumBands));                   \
                                                                                  \
	return dw;																	  \
}

/** get the output of the process object, which is templated
 *  over the INPUT and OUTPUT component type of this wrapper class
 */
#define GetOutputWrap( ClassName, InternalWrapper )                                              \
QSharedPointer<NMItkDataObjectWrapper> ClassName::getOutput(unsigned int idx)					\
{                                                                               \
    QSharedPointer<NMItkDataObjectWrapper> ret;                                    \
    ret.clear();                                                                    \
    if (!this->mbIsInitialised)                                                   \
        return ret;                                                                 \
                                                                                \
	itk::DataObject* img = 0;                                                   \
                                                                                \
	switch (this->mInputComponentType)                                         \
	{                                                                           \
	case otb::ImageIOBase::UCHAR:                                               \
		outputTypeSwitch( unsigned char, callGetOutput, InternalWrapper );                                         \
		break;                                                                  \
	case otb::ImageIOBase::CHAR:                                                \
		outputTypeSwitch( char, callGetOutput, InternalWrapper );                                                  \
		break;                                                                  \
	case otb::ImageIOBase::USHORT:                                              \
		outputTypeSwitch( unsigned short, callGetOutput, InternalWrapper );                                        \
		break;                                                                  \
	case otb::ImageIOBase::SHORT:                                               \
		outputTypeSwitch( short, callGetOutput, InternalWrapper );                                                 \
		break;                                                                  \
	case otb::ImageIOBase::UINT:                                                \
		outputTypeSwitch( unsigned int, callGetOutput, InternalWrapper );                                          \
		break;                                                                  \
	case otb::ImageIOBase::INT:                                                 \
		outputTypeSwitch( int, callGetOutput, InternalWrapper );                                                   \
		break;                                                                  \
	case otb::ImageIOBase::ULONG:                                               \
		outputTypeSwitch( unsigned long, callGetOutput, InternalWrapper );                                         \
		break;                                                                  \
	case otb::ImageIOBase::LONG:                                                \
		outputTypeSwitch( long, callGetOutput, InternalWrapper );                                                  \
		break;                                                                  \
	case otb::ImageIOBase::FLOAT:                                               \
		outputTypeSwitch( float, callGetOutput, InternalWrapper );                                                 \
		break;                                                                  \
	case otb::ImageIOBase::DOUBLE:                                              \
		outputTypeSwitch( double, callGetOutput, InternalWrapper );                                                \
		break;																	\
	default:                                                                    \
		break;                                                                  \
	}                                                                           \
                                                                                \
    QSharedPointer<NMItkDataObjectWrapper> dw(                                  \
			new NMItkDataObjectWrapper(this, img, this->mOutputComponentType,   \
            this->mOutputNumDimensions, this->mOutputNumBands));                 \
                                                                                \
	return dw;																	\
}

/** get the output of the process object, which is templated
 *  over the INPUT and OUTPUT component type of this wrapper class
 */
#define GetOutputRATWrap( ClassName, InternalWrapper )                                              \
QSharedPointer<NMItkDataObjectWrapper> ClassName::getOutput(unsigned int idx)					\
{                                                                               \
    QSharedPointer<NMItkDataObjectWrapper> ret;                                    \
    ret.clear();                                                                    \
    if (!this->mbIsInitialised)                                                   \
        return ret;                                                                 \
                                                                                \
    itk::DataObject* img = 0;                                                   \
    otb::AttributeTable::Pointer rat = 0;                                       \
                                                                                \
    switch (this->mInputComponentType)                                         \
    {                                                                           \
    case otb::ImageIOBase::UCHAR:                                               \
        outputTypeSwitch( unsigned char, callGetOutput, InternalWrapper );                                         \
        break;                                                                  \
    case otb::ImageIOBase::CHAR:                                                \
        outputTypeSwitch( char, callGetOutput, InternalWrapper );                                                  \
        break;                                                                  \
    case otb::ImageIOBase::USHORT:                                              \
        outputTypeSwitch( unsigned short, callGetOutput, InternalWrapper );                                        \
        break;                                                                  \
    case otb::ImageIOBase::SHORT:                                               \
        outputTypeSwitch( short, callGetOutput, InternalWrapper );                                                 \
        break;                                                                  \
    case otb::ImageIOBase::UINT:                                                \
        outputTypeSwitch( unsigned int, callGetOutput, InternalWrapper );                                          \
        break;                                                                  \
    case otb::ImageIOBase::INT:                                                 \
        outputTypeSwitch( int, callGetOutput, InternalWrapper );                                                   \
        break;                                                                  \
    case otb::ImageIOBase::ULONG:                                               \
        outputTypeSwitch( unsigned long, callGetOutput, InternalWrapper );                                         \
        break;                                                                  \
    case otb::ImageIOBase::LONG:                                                \
        outputTypeSwitch( long, callGetOutput, InternalWrapper );                                                  \
        break;                                                                  \
    case otb::ImageIOBase::FLOAT:                                               \
        outputTypeSwitch( float, callGetOutput, InternalWrapper );                                                 \
        break;                                                                  \
    case otb::ImageIOBase::DOUBLE:                                              \
        outputTypeSwitch( double, callGetOutput, InternalWrapper );                                                \
        break;																	\
    default:                                                                    \
        break;                                                                  \
    }                                                                           \
                                                                                \
    QSharedPointer<NMItkDataObjectWrapper> dw(                                  \
            new NMItkDataObjectWrapper(this, img, this->mOutputComponentType,   \
            this->mOutputNumDimensions, this->mOutputNumBands));                 \
                                                                                \
    dw->setOTBTab(this->getRAT(idx)->getOTBTab());                                           \
                                                                                \
    return dw;																	\
}

/** set the nth input of a process object, which is
 *  templated over the INPUT and OUTPUT type of this
 *  wrapper class
 */
#define SetNthInputWrap( ClassName, InternalWrapper )						\
void ClassName::setNthInput(unsigned int numInput,  		\
        QSharedPointer<NMItkDataObjectWrapper> imgWrapper)                 \
{                                                           \
	if (!this->mbIsInitialised)                             \
		return;                                             \
                                                            \
	itk::DataObject* img = imgWrapper->getDataObject();     \
	switch (this->mInputComponentType)                      \
	{                                                       \
	case otb::ImageIOBase::UCHAR:                                               \
		outputTypeSwitch( unsigned char, callSetInput, InternalWrapper );                                         \
		break;                                                                  \
	case otb::ImageIOBase::CHAR:                                                \
		outputTypeSwitch( char, callSetInput, InternalWrapper );                                                  \
		break;                                                                  \
	case otb::ImageIOBase::USHORT:                                              \
		outputTypeSwitch( unsigned short, callSetInput, InternalWrapper );                                        \
		break;                                                                  \
	case otb::ImageIOBase::SHORT:                                               \
		outputTypeSwitch( short, callSetInput, InternalWrapper );                                                 \
		break;                                                                  \
	case otb::ImageIOBase::UINT:                                                \
		outputTypeSwitch( unsigned int, callSetInput, InternalWrapper );                                          \
		break;                                                                  \
	case otb::ImageIOBase::INT:                                                 \
		outputTypeSwitch( int, callSetInput, InternalWrapper );                                                   \
		break;                                                                  \
	case otb::ImageIOBase::ULONG:                                               \
		outputTypeSwitch( unsigned long, callSetInput, InternalWrapper );                                         \
		break;                                                                  \
	case otb::ImageIOBase::LONG:                                                \
		outputTypeSwitch( long, callSetInput, InternalWrapper );                                                  \
		break;                                                                  \
	case otb::ImageIOBase::FLOAT:                                               \
		outputTypeSwitch( float, callSetInput, InternalWrapper );                                                 \
		break;                                                                  \
	case otb::ImageIOBase::DOUBLE:                                              \
		outputTypeSwitch( double, callSetInput, InternalWrapper );                                                \
		break;																	\
	default:                                                \
		break;                                              \
	}                                                       \
}

/** set the nth input of a process object, which is
 *  templated over the INPUT type of this
 *  wrapper class
 */
#define SetInputTypeNthInputWrap( ClassName, InternalWrapper )						\
void ClassName::setNthInput(unsigned int numInput,  		\
        QSharedPointer<NMItkDataObjectWrapper> imgWrapper)                 \
{                                                           \
	if (!this->mbIsInitialised)                             \
		return;                                             \
                                                            \
	itk::DataObject* img = imgWrapper->getDataObject();     \
	switch (this->mInputComponentType)                      \
	{       \
	MacroPerType( callInputTypeSetInput, InternalWrapper )                       \
	default:                                                                      \
		break;                                                                    \
	}                                                                             \
}

/** get the raster attribute table (RAT) of the process object,
 *  which is templated over the INPUT and OUTPUT component type
 *  of this wrapper class
 */
#define GetRATWrap( ClassName, InternalWrapper )                                              \
QSharedPointer<NMItkDataObjectWrapper> ClassName::getRAT(unsigned int idx)					\
{                                                                               \
    QSharedPointer<NMItkDataObjectWrapper> ret;                                    \
    ret.clear();                                                                    \
    if (!this->mbIsInitialised)                                                   \
        return ret;                                                                 \
                                                                                \
    otb::AttributeTable::Pointer rat = 0;                                       \
                                                                                \
    switch (this->mInputComponentType)                                          \
    {                                                                           \
    case otb::ImageIOBase::UCHAR:                                               \
        outputTypeSwitch( unsigned char, callGetRAT, InternalWrapper );         \
        break;                                                                  \
    case otb::ImageIOBase::CHAR:                                                \
        outputTypeSwitch( char, callGetRAT, InternalWrapper );                  \
        break;                                                                  \
    case otb::ImageIOBase::USHORT:                                              \
        outputTypeSwitch( unsigned short, callGetRAT, InternalWrapper );        \
        break;                                                                  \
    case otb::ImageIOBase::SHORT:                                               \
        outputTypeSwitch( short, callGetRAT, InternalWrapper );                 \
        break;                                                                  \
    case otb::ImageIOBase::UINT:                                                \
        outputTypeSwitch( unsigned int, callGetRAT, InternalWrapper );          \
        break;                                                                  \
    case otb::ImageIOBase::INT:                                                 \
        outputTypeSwitch( int, callGetRAT, InternalWrapper );                   \
        break;                                                                  \
    case otb::ImageIOBase::ULONG:                                               \
        outputTypeSwitch( unsigned long, callGetRAT, InternalWrapper );         \
        break;                                                                  \
    case otb::ImageIOBase::LONG:                                                \
        outputTypeSwitch( long, callGetRAT, InternalWrapper );                  \
        break;                                                                  \
    case otb::ImageIOBase::FLOAT:                                               \
        outputTypeSwitch( float, callGetRAT, InternalWrapper );                 \
        break;                                                                  \
    case otb::ImageIOBase::DOUBLE:                                              \
        outputTypeSwitch( double, callGetRAT, InternalWrapper );                \
        break;																	\
    default:                                                                    \
        break;                                                                  \
    }                                                                           \
                                                                                \
    QSharedPointer<NMItkDataObjectWrapper> dw(                                  \
            new NMItkDataObjectWrapper(this, 0, this->mOutputComponentType,     \
            this->mOutputNumDimensions, this->mOutputNumBands));                 \
    dw->setOTBTab(rat);                                                         \
                                                                                \
    return dw;																	\
}

/** get the raster attribute table (RAT) of the process object,
 *  which is templated over the INPUT and OUTPUT component type
 *  of this wrapper class
 */
#define SetRATWrap( ClassName, InternalWrapper )						\
void ClassName::setRAT(unsigned int numInput,  		\
        QSharedPointer<NMItkDataObjectWrapper> imgWrapper)                 \
{                                                                               \
    if (!this->mbIsInitialised)                                                 \
        return;                                                               \
                                                                                \
    otb::AttributeTable::Pointer rat = imgWrapper->getOTBTab();                 \
                                                                                \
    switch (this->mInputComponentType)                                          \
    {                                                                           \
    case otb::ImageIOBase::UCHAR:                                               \
        outputTypeSwitch( unsigned char, callSetRAT, InternalWrapper );         \
        break;                                                                  \
    case otb::ImageIOBase::CHAR:                                                \
        outputTypeSwitch( char, callGetRAT, InternalWrapper );                  \
        break;                                                                  \
    case otb::ImageIOBase::USHORT:                                              \
        outputTypeSwitch( unsigned short, callSetRAT, InternalWrapper );        \
        break;                                                                  \
    case otb::ImageIOBase::SHORT:                                               \
        outputTypeSwitch( short, callGetRAT, InternalWrapper );                 \
        break;                                                                  \
    case otb::ImageIOBase::UINT:                                                \
        outputTypeSwitch( unsigned int, callSetRAT, InternalWrapper );          \
        break;                                                                  \
    case otb::ImageIOBase::INT:                                                 \
        outputTypeSwitch( int, callGetRAT, InternalWrapper );                   \
        break;                                                                  \
    case otb::ImageIOBase::ULONG:                                               \
        outputTypeSwitch( unsigned long, callSetRAT, InternalWrapper );         \
        break;                                                                  \
    case otb::ImageIOBase::LONG:                                                \
        outputTypeSwitch( long, callSetRAT, InternalWrapper );                  \
        break;                                                                  \
    case otb::ImageIOBase::FLOAT:                                               \
        outputTypeSwitch( float, callSetRAT, InternalWrapper );                 \
        break;                                                                  \
    case otb::ImageIOBase::DOUBLE:                                              \
        outputTypeSwitch( double, callSetRAT, InternalWrapper );                \
        break;																	\
    default:                                                                    \
        break;                                                                  \
    }                                                                           \
}


#endif /* NMMACROS_H_ */
