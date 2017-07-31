/* NMMosra.h
 *
 * Copyright 2011 Alexander Herzig
 *
 * This file is part of 'LUMASS', which is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*!
 * \brief NMMosra implements multi-objective spatial resource allocation.
 *
 * NMMosra assumes resources are allocated to spatial units with an area > 0
 * (i.e. polygons, raster (not implemented yet)), which are provided by
 * an instance of NMLayer. Using this class involves the following recommended
 * sequence of member calls:
 *
 * 1. create the object: NMMosra* mosra = new NMMosra();
 * 2. (optional) set the time out : mosroa->setTimeOut(timeout);
 * 3. load the problem configuration: mosra->loadSettings(myLOSfile);
 * 4. set the specified NMLayer instance: mosra->setLayer(myNMLayer);
 * 5. solve the problem: mosra->solveLp();
 * 6. if 5. was successful, extract results and write them into the layer: mosra->mapLp()
 * 7. (optional) summarise the results: mosra->sumResults();
 * 8. (optional) write a report: mosra->writeReport();
 *
 * Additionally, you may want to export the formulated problem as an LP-file
 * (mosra->getLp()->WriteLp(filename)) or you may want to reflect the results
 * in your layer by calling layer->mapUniqueValues("OPT_STR").
 *
 *
 */

#ifndef NMMOSRA_H_
#define NMMOSRA_H_

//#include "nmlog.h"
//#define ctxNMMosra "NMMosra"

#include <QObject>
#include <QMap>
#include <QStringList>
//#include <QStandardItemModel>

#include "LpHelper.h"
//#include "NMLayer.h"

#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "otbAttributeTable.h"

class NMLogger;

/////////////////////////////////////////////////////////////////
/// \brief The NMMosraDataSet class
///        provides an abstract layer around
///        different (table-based) data sources the
///        the optimisation could run on
/////////////////////////////////////////////////////////////////

class NMMosraDataSet : public QObject
{

public:
    NMMosraDataSet(QObject* parent=0);
    ~NMMosraDataSet(){}

    enum NMMosraDataSetDataType
    {
        NM_MOSRA_DATATYPE_INT = 0,
        NM_MOSRA_DATATYPE_DOUBLE,
        NM_MOSRA_DATATYPE_STRING
    };

    enum NMMosraDataSetType
    {
        NM_MOSRA_DS_VTKDS = 0,
        NM_MOSRA_DS_OTBTAB,
        NM_MOSRA_DS_NONE
    };

    NMMosraDataSetType getDataSetType()
        {return mType;}

    vtkSmartPointer<vtkTable> getDataSetAsVtkTable();

    void setDataSet(vtkDataSet* vtkds);
    void setDataSet(otb::AttributeTable::Pointer otbtab);

    otb::AttributeTable::Pointer getAttributeTable(void)
        {return mOtbTab;}
    vtkDataSet* getVtkDataSet(void)
        {return mVtkDS;}

    bool hasColumn(const QString& columnName);
    int  getColumnIndex(const QString& colName);
    void addColumn(const QString& colName,
                   NMMosraDataSetDataType type);

    int  getNumRecs();
    double getDblValue(const QString& columnName, int row);
    int  getIntValue(const QString& columnName, int row);
    QString getStrValue(const QString& columnName, int row);

    double getDblValue(int col, int row);
    int  getIntValue(int col, int row);
    QString getStrValue(int col, int row);


    void setIntValue(const QString& colname, int row, int value);
    void setDblValue(const QString& colname, int row, double value);
    void setStrValue(const QString& colname, int row, const QString& value);


protected:

    NMMosraDataSetType mType;

    vtkDataSet* mVtkDS;
    otb::AttributeTable::Pointer mOtbTab;

};


/////////////////////////////////////////////////////////////////
/// \brief The NMMosra class
///        the one dealing with the deatails of the optimisation
/////////////////////////////////////////////////////////////////

class NMMosra : public QObject
{
	Q_OBJECT
	Q_ENUMS(NMMosoDVType)
	Q_ENUMS(NMMOsoScalMeth)

friend class NMMosraDataSet;

public:
	NMMosra(QObject* parent=0);
	virtual ~NMMosra();

	// decision variable type
	enum NMMosoDVType {NM_MOSO_REAL, NM_MOSO_INT,
		NM_MOSO_BINARY};

	// scalarisation method
	enum NMMosoScalMeth {NM_MOSO_WSUM, NM_MOSO_INTERACTIVE};

	// area unit type
	enum AreaUnitType {NM_MOSO_MAP_UNITS, NM_MOSO_PERCENT_TOTAL,
		NM_MOSO_PERCENT_SELECTED, NM_MOSO_PERCENT_ZONE};

    /*! loads the settings from textfile (LUMASS Optimisation settings file, *.los)
     *  and passes a string to the parseStringSettings function for doing the
     *  grunt work of analysing the string.
     */
	int loadSettings(QString fileName);

    /*! parses the settings and makes them meaningful to NMMosra*/
    int parseStringSettings(QString strSettings);

	QString getLayerName(void);

    void setLogger(NMLogger* logger){mLogger = logger;}

    void setItkProcessObject(itk::ProcessObject* obj)
        {mProcObj = obj;}

	void setDataSet(const vtkDataSet* dataset);
    void setDataSet(otb::AttributeTable::Pointer otbtab);
    const NMMosraDataSet* getDataSet()
		{return this->mDataSet;}
	vtkSmartPointer<vtkTable> getDataSetAsTable();

    void cancelSolving(void) {this->mbCanceled = true;}
	int solveLp(void);
	int mapLp(void);
	HLpHelper* getLp();
    vtkSmartPointer<vtkTable> sumResults(vtkSmartPointer<vtkTable>& changeMatrix);

	// enquire batch processing set up
	bool doBatch(void);
	QString getDataPath(void)
		{return this->msDataPath;}
	QStringList getPerturbationItems()
		{return this->mslPerturbItems;}
    const QList< QList<float> >& getAllUncertaintyLevels()
        {return this->mlflPerturbUncertainties;}
    const QList<float>& getUncertaintyLevels(const int& perturbItemIdx)
        {return this->mlflPerturbUncertainties.at(perturbItemIdx);}
	long getNumberOfPerturbations()
		{return this->mlReps;}
	QString getLosFileName(void)
		{return this->msLosFileName;}
	unsigned int getTimeOut(void)
		{return this->muiTimeOut;}

	void writeReport(QString fileName);
	QString getReport(void);

	/* set a solver timeout -> maximum availalbe time for solving
	 * the problem
	 */
	void setTimeOut(int secs)
        {this->muiTimeOut = secs >= 0 ? secs : 0;}

    void setBreakAtFirst(bool breakAtFirst)
        {this->mbBreakAtFirst = breakAtFirst;}

	/*	\brief add uncertainty to performance scores
	 *
	 *  This function varies the individual performance scores by
	 *  the uncertainty given as 'percent'. It takes the scores for
	 *  each individual land use of the criterion, and randomly
	 *  (uniform distribution) adds an uncertainty value
	 *  of +/- 0 to percent.
	 */
    bool perturbCriterion(const QString& criterion,
                          const QList<float>& percent);

	/* \brief varies a constraint by a given percent
	 *
	 *  This function varies the named criterion by the given
	 *  percentage.
	 */
    bool varyConstraint(const QString& constraint,
                        float percent);

	/* lp_solve callback function to check for user abortion ->
	 * i.e. interactive cancellation of solving process rather
	 * than a time one
	 */
    static int callbackIsSolveCanceled(lprec* lp, void* userhandle);

    static void lpLogCallback(lprec* lp, void* userhandle, char* buf);



private:

    static const std::string ctxNMMosra;

	bool mbCanceled;

	HLpHelper* mLp;
    //vtkDataSet* mDataSet;
    NMMosraDataSet* mDataSet;
    NMLogger* mLogger;

    itk::ProcessObject* mProcObj;

	QString msReport;
	QString msSettingsReport;
	QString msLosFileName;

	QString msLandUseField;
	QString msAreaField;
	QString msLayerName;

	QString msDataPath;
	QStringList mslPerturbItems;
    QList< QList<float> > mlflPerturbUncertainties;
	QList<float> mflUncertainties;
	long mlReps;

	NMMosoDVType meDVType;
	NMMosoScalMeth meScalMeth;

    bool mbBreakAtFirst;
	unsigned int muiTimeOut;
	int miNumOptions;
	QStringList mslOptions;
    QStringList mslPerfSumZones;

	// <objective> <min | max> <weight>
	QMap<QString, QStringList> mmslObjectives;

	// maps criterion performance measures onto criterion name
	// <criterion name>, <performance of option 1> [ <performance of option 2> ...]
	// option performance fields have to be given in the same order
	// as in the mlsOptions list
	QMap<QString, QStringList> mmslCriteria;

	// as above but this map contains per criterion the data fields which
	// are going to be used to evaluate status quo and optimised
	// performance
	QMap<QString, QStringList> mmslEvalFields;

	// maps areal constraints onto options
    // <constr label>, <option[,option[,...]][:zonefield]> < >= | <= > < number > < percent_of_total | percent_of_selected | map_units >
	QMap<QString, QStringList> mmslAreaCons;
	QMap<QString, QStringList> mmslAreaZoneCons;
	QStringList mmslAreaConsLabel;

    QMap<QString, QStringList> mmslFeatCons;

	/*! nested map; holds for each zone field the maximum available area for each specified
	 *  (combination of) land use options */
	QMap<QString, QMap<QString, double > > mmslZoneAreas;
	QMap<QString, QMap<QString, int > > mmslZoneLength;


	// maps criterion constraints onto criteria
	// nested maps:
	// outer (multi-)map:
	// 		key:   <criterion label>   value: <land use field map>
	// inner map (land use field map):
	// 		key: <land use label | "total">  value: <if 'land use': land use performance field>
	//				        		                <if 'total': list of all land use performance fields>
	//		 										 < >= | <= | = > < criterion cap >
	QMultiMap<QString, QMap<QString, QStringList > > mmslCriCons;

	// maps objective constraints onto objectives
	// <objective>, < >= | <= > < number >
	QMap<QString, QStringList> mmslObjCons;

	long mlNumDVar;
	long mlNumArealDVar;
	long mlNumOptFeat;
	long mlCriCons;
	long mlLpCols;
	double mdAreaTotal;
	double mdAreaSelected;

	// reset internal variables
	void reset(void);
	void createReport(void);

	int checkSettings(void);

	int makeLp(void);
	int addObjFn(void);
	int addObjCons(void);
	int addExplicitAreaCons(void);
	int addImplicitAreaCons(void);
    int addFeatureCons(void);
	int addCriCons(void);

	int isSolveCanceled(void);
    void forwardLpLog(const char* log);

	/* converts the user specified area into area units
	  (same units as the specified area field) */
	double convertAreaUnits(double input, AreaUnitType otype);
	double convertAreaUnits(double input, const QString& otype, const QStringList& zoneSpec);

};

#endif /* NMMOSRA_H_ */
