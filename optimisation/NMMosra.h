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

#include <QObject>
#include <QMap>
#include <QStringList>
#include <QSqlTableModel>
#include <QSqlQuery>

#include "LpHelper.h"

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

    void reset();

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
        NM_MOSRA_DS_QTSQL,
        NM_MOSRA_DS_NONE
    };

    NMMosraDataSetType getDataSetType()
        {return mType;}

    vtkSmartPointer<vtkTable> getDataSetAsVtkTable();

    void setDataSet(vtkDataSet* vtkds);
    void setDataSet(otb::AttributeTable::Pointer otbtab);
    void setDataSet(QSqlTableModel *sqlmod);

    otb::AttributeTable::Pointer getOtbAttributeTable(void)
        {return mOtbTab;}
    vtkDataSet* getVtkDataSet(void)
        {return mVtkDS;}
    QSqlTableModel* getQSqlTableModel(void)
        {return mSqlMod;}


    int  getColumnIndex(const QString& colName);
    QString getColumnName(const int& idx);
    bool hasColumn(const QString& columnName);
    void addColumn(const QString& colName,
                   NMMosraDataSetDataType type);
    NMMosraDataSetDataType getColumnType(const QString& colname);

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

    void setProcObj(itk::ProcessObject* procObj) {mProcObj = procObj;}
    void setLogger(NMLogger* logger) {mLogger = logger;}

    /*! for DB-based datasets */

    /*! for db-based datasets starts/ends a transaction */
    bool beginTransaction();
    bool endTransaction();
    void rollBack();

    bool prepareRowUpdate(const QStringList& colnames, bool bInsert);
    bool updateRow(const QVariantList& values, const int& row=-1);

    bool prepareRowGet(const QStringList& colnames);
    bool getRowValues(QVariantList& values, const int& row);

    void setTableName(const QString& name) {mTableName = name;}

protected:

    QString getNMPrimaryKey();
    QVariant getQSqlTableValue(const QString& column, int row);

    QMap<QString, NMMosraDataSetDataType> mColTypes;
    NMMosraDataSetType mType;

    vtkDataSet* mVtkDS;
    otb::AttributeTable::Pointer mOtbTab;
    QSqlTableModel* mSqlMod;

    itk::ProcessObject* mProcObj;
    NMLogger* mLogger;

    QList<int> mRowUpdateColumnIndices;
    QStringList mRowUpdateColumns;
    QStringList mRowGetColumns;
    QSqlQuery mRowUpdateQuery;
    QSqlQuery mRowGetQuery;

    QString mTableName;
    QString mPrimaryKey;

    QVector<QSqlQuery> mGetColValueQueries;

    bool mbTransaction;
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

//friend class NMMosraDataSet;

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

    // type of model to export
    enum NMMosoExportType {NM_MOSO_LP, NM_MOSO_MPS};

    /*! loads the settings from textfile (LUMASS Optimisation settings file, *.los)
     *  and passes a string to the parseStringSettings function for doing the
     *  grunt work of analysing the string.
     */
	int loadSettings(QString fileName);

    /*! parses the optimisation settings from a string and makes them meaningful to NMMosra*/
    int parseStringSettings(QString strSettings);

	QString getLayerName(void);

    void setLogger(NMLogger* logger);

    void setItkProcessObject(itk::ProcessObject* obj);

    /*! stores the filename of the settings file with this object,
        doesn't do anything else, i.e. doesn't parse of othewise checks
        the file for accessibility etc.
    */
    void setLosFileName(const QString& fileName) { msLosFileName = fileName; }

    /*! stores the content of an optimisation settings file as one big string */
    void setLosSettings(const QString& settings) { msLosSettings = settings; }

    /*! retreives the currently stored optimisation settings; use \ref parseStringSettings
     *  to parse the settings */
	QString getLosSettings(void) { return msLosSettings; }
 
	void setDataSet(const vtkDataSet* dataset);
    void setDataSet(otb::AttributeTable::Pointer otbtab);
    void setDataSet(const QSqlTableModel* sqlmod);
    const NMMosraDataSet* getDataSet()
		{return this->mDataSet;}
	vtkSmartPointer<vtkTable> getDataSetAsTable();

    void cancelSolving(void) {this->mbCanceled = true;}
	int solveLp(void);
	int mapLp(void);
    int mapLpTab(void);
    int mapLpDb(void);
    int mapLpQtSql(void);
    int calcBaseline(void);
    int calcBaselineTab(void);
    int calcBaselineDb(void);
    int calcBaselineQtSql(void);
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

    /*! writes the model after it has been build
     *  but before solving it
     *
     *  filename: the filename of the exported model
     *  type: NM_MOSO_LP || NM_MOSO_MPS
     */
    void writeProblem(QString filename, NMMosoExportType type);

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
	QString msLosSettings;

    QString mProblemFilename;
    NMMosoExportType mProblemType;

	QString msLandUseField;
	QString msAreaField;
	QString msLayerName;

    QString msOptFeatures;
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

    // map holding the incentive names (key) (comprised of to
    // valid criterion names from the CRITERIA section
    // concatenated by and underscore '_') and the associated
    // column name (value) storing the actual incentive
    // as per ha score for each feature
    QMap<QString, QStringList> mmslIncentives;

    // map holding the incentive names (key) (s. above) and
    // the 'split' name, i.e. the individual criteria
    // representing the benefit criterion (first) and the
    // cost criterion (second)
    QMap<QString, QStringList> mIncNamePair;

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

    /*! map containing (non-overlapping) generic zone constraints
     * key: ZONE_CONS_<x>
     * string list: 0: column name of unique value zone identifier
     *              1: <column name of resource spec, e.g. GullyMit0 SurfMit1 LandMit2 ...> | total
     *              2: criterion label
     *              3: operator: <= | >= | =
     *              4: column name of constraint value; note: if an individual zone
     *                 comprises more than one spatial option, all spatial options
     *                 within a particular zone should have the same value allocated
     *                 to them
     */
    QMap<QString, QStringList> mslZoneConstraints;



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

    int addIncentCons(void);
	int addObjCons(void);
	int addExplicitAreaCons(void);
	int addImplicitAreaCons(void);
    int addFeatureCons(void);
    int addZoneCons(void);
	int addCriCons(void);

	int isSolveCanceled(void);
    void forwardLpLog(const char* log);

	/* converts the user specified area into area units
	  (same units as the specified area field) */
	double convertAreaUnits(double input, AreaUnitType otype);
	double convertAreaUnits(double input, const QString& otype, const QStringList& zoneSpec);

};

#endif /* NMMOSRA_H_ */
