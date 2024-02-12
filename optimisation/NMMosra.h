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
#include <QSet>
#include <QStringList>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QTextStream>
#include <QFile>

#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "otbAttributeTable.h"
#include "otbSQLiteTable.h"

#include "LpHelper.h"

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

struct VarAdmin
{
    QString name;
    QStringList dimensions;
    QStringList bounds;

    // <dim id vector>, <varOffset>
    // note the dimension ids, identifying the
    // variable offset, are stored in the same
    // order as defined in the .los file
    //      dim id = as values in table
    // OPTIONS ids = 0-based
    // var_offset  = 0-based
    QMap<QVector<long long>, long long> dimOffsetMap;
    //QMap<long long, std::vector<long long> > offsetDimMap;

    VarAdmin(){}

    VarAdmin(const VarAdmin& va)
    {
        name = va.name;
        dimensions = va.dimensions;
        bounds = va.bounds;

        dimOffsetMap = va.dimOffsetMap;
    }
};

class NMOpenGA;

class NMMosra : public QObject
{
    Q_OBJECT
    Q_ENUMS(NMMosoDVType)
    Q_ENUMS(NMMOsoScalMeth)

//friend class NMMosraDataSet;

public:
    NMMosra(QObject* parent=0);
    virtual ~NMMosra();

    // solver type
    enum NMSolverType {NM_SOLVER_NONE,
                       NM_SOLVER_LPSOLVE,
                       NM_SOLVER_IPOPT,
                       NM_SOLVER_GA};

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

    void setSolFileName(const QString& filename) {msSolFileName = filename;}
    QString getSolFileName(void){return msSolFileName;}

    QString getNlFileName(void){return msNlFileName;}

    void setEnableLUCControl(bool blucc){mbEnableLUCControl = blucc;}
    bool getEnableLUCControl(void){return mbEnableLUCControl;}

    void setLUCControlField(const QString& lucctrlField){msLUCControlField = lucctrlField;}
    QString getLUCControlField(void){return msLUCControlField;}

    void setMaxOptAlloc(const int maxOptAlloc){miMaxOptAlloc = maxOptAlloc;}
    int getMaxOptAlloc(void){return miMaxOptAlloc;}

    int backupLUCControl(void);
    int restoreLUCControl(void);
    int getNumRecLUCChange(void){return miNumRecLUCChange;}
    void lockInLUGrps(bool bLockInLuGrps){mbLockInLuGrps = bLockInLuGrps;}

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

    NMSolverType getSolverType(void){return mSolverType;}

    void setDataSet(const vtkDataSet* dataset);
    void setDataSet(otb::AttributeTable::Pointer otbtab);
    void setDataSet(const QSqlTableModel* sqlmod);
    const NMMosraDataSet* getDataSet()
        {return this->mDataSet;}
    vtkSmartPointer<vtkTable> getDataSetAsTable();

    void setScenarioName(const QString& name)
        {mScenarioName = name;}
    QString getScenarioName(void) {return mScenarioName;}

    bool infixToPolishPrefix();

    void cancelSolving(void) {this->mbCanceled = true;}
    int configureProblem(void);
    bool solveProblem(void);
    int solveLp(void);
    int solveOpenGA(void);
    int mapLp(void);
    /*
     * proc : 0=landslide, 1=surficial, 2=earthflow, 3=gully
     */
    int makeSTECLp(int proc);
    int makeSTECLp2(int proc);
    int makeNL(void);
    int makeNL2(void);
    int mapLpTab(void);
    int mapLpDb(void);
    int mapLpQtSql(void);
    int mapNL(void);
    int calcBaseline(void);
    int calcBaselineTab(void);
    int calcBaselineDb(void);
    int calcBaselineQtSql(void);
    int calcOptPerformanceDb(void);
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

    void writeBaselineReductions(QString filename);

    void writeSTECReport(QString fileName, std::map<int, std::map<long long, double> > &valstore);
    void writeSTECReport2(QString fileName, std::map<int, std::map<long long, double> > &valstore);

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


    bool processExplicitArealCons(std::vector<QString>& vsConsLabel,
                                  std::vector<int>& vnZoneLength,
                                  std::vector<QString>& vsZoneField,
                                  std::vector<std::vector<unsigned int> >& vvnOptionIndex,
                                  std::vector<unsigned int>& vnConsType,
                                  std::vector<double>& vdRHS
                                  );

    bool processFeatureSetConstraints(std::vector<QString>& vsFSCZoneField,
                                std::vector<std::vector<unsigned int> >& vvnFSCOptionIndex);


    // fills mmvHighDimLoopIterLength
    bool determineHighDimLoopIterLength(
            const QString& eqn,
            otb::SQLiteTable::Pointer& sqltab
            );


private:

    static const std::string ctxNMMosra;

    bool mbCanceled;

    NMSolverType mSolverType;

    HLpHelper* mLp;
    //vtkDataSet* mDataSet;
    NMMosraDataSet* mDataSet;
    NMLogger* mLogger;

    itk::ProcessObject* mProcObj;

    QString msReport;
    QString msSettingsReport;
    QString msLosFileName;
    QString msLosSettings;
    QString mScenarioName;
    QString msSolFileName;
    QString msNlFileName;

    QString mProblemFilename;
    NMMosoExportType mProblemType;

    QString msAreaField;
    QString msLayerName;


    QString msLandUseField;
    QString msLUCControlField;
    QString msBkpLUCControlField;

    // openGA settings
    bool mbGAMultiThreading;
    double mGAConstraintTolerance;
    size_t mGAPopulation;
    size_t mGAMaxGeneration;

    // IpOpt specific settings
    bool mbEnableLUCControl;
    bool mbLockInLuGrps;
    int  miMaxOptAlloc;
    int  miNumRecLUCChange;
    double mdLuLbndFactor;

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
    // {lp_solve, ipopt}
    QString msSolver;
    QStringList mslOptions;
    QStringList mslPerfSumZones;

    QList<QStringList> mlslOptGrps;

    // <OPTION, OptGrp (pos in mlslOptGrps)>
    QMap<QString, int> mmOptGrpMap;

    // <objective> <min | max> <weight>
    QMap<QString, QStringList> mmslObjectives;

    // maps criterion performance measures onto criterion name
    // <criterion name>, <performance of option 1> [ <performance of option 2> ...]
    // option performance fields have to be given in the same order
    // as in the mlsOptions list
    QMap<QString, QStringList> mmslCriteria;

    // maps eqn parameters to columns (values) in the optimisation db
    // when a parameter is further qualifed by ':unique' it denotes
    // a dimension defined over the distinct set of values of the given
    // (single) column in the db
    // <para name>, <column1 [column2 [column3 ...]]>
    QMap<QString, QStringList> mmslParameters;
    QStringList mslDistinctParameters;

    // defines non-linear decision variables and thier dimension(s);
    // the latter are defined as (=distinct parameters, s. above) or
    // the special OPTIONS set of resources
    // <var name>, <dimension1 [dimension2 [dimension3 ...]]>
    QMap<QString, QStringList> mmslVariables;

    QMap<QString, QString> mmsEquations;

    // map holding the incentive names (key) (comprised of two
    // valid criterion names from the CRITERIA section
    // concatenated by an underscore '_') and the associated
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

    // <eqn name>, <comp operator, rhs>
    QMap<QString, QStringList> mmslLinearConstraints;


    // <eqn name>, <comp operator, rhs>
    QMap<QString, QStringList> mmslNonLinearConstraints;

    // <eqn name>, <comp operator, rhs>
    QMap<QString, QStringList> mmslLogicConstraints;

    // <parameter name>, < pair<scaling factor, affected eqns> >
    QMap<QString, std::pair<double, QStringList> > mmParameterScaling;

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


    /*! FEATURE SET CONSTRAINTS
     *
     *  key: FEATSET_CONS_<X>
     *  purpose: constrain the performance of a given set of land uses regarding a specific
     *           criterion
     *  string list: 0: land use option(s): <land_use_1[+land_use_2[+land_use_3[+...]]] | total>
     *               1: column name containing the unique identifiers of sets of features: <column_name>
     *               2: criterion the performance of land-use options on is to be constrained
     *                  within the specified regions
     *               3: operator: < <= | = | >= >
     *               4: column name of right-hand-side values (thresholds) specified for individual
     *                  sets of features (zones);
     *
     *  Example:
     *  OPTIONS=Dai FOR SNB
     *
     *  FEATSET_CONS_1=Dai:cat_n   Nloss <= TN_MAL
     *
     *  FEATSET_CONS_2=total:cat_p Ploss <= TP_MAL
     *
     *
     *  ID | n_dai | p_dai | p_snb | cat_n |  TN_MAL | cat_p |  TP_MAL |
     *  ---|-------|-------|-------|-------|---------|-------|---------|
     *  0  | 30    | 1.3   | 0.5   |       |         | 101   |  3.4    |
     *  1  | 45    | 1.0   | 0.4   | 303   |  1343   | 303   |  7.5    |
     *  2  | 80    | 0.7   | 0.3   | 303   |  1343   | 303   |  7.5    |
     *  3  | 56    | 1.2   | 0.5   | 404   |   454   |       |         |
     *  4  | 40    | 1.8   | 0.8   | 303   |  1343   | 303   |  7.5    |
     *  5  | 103   | 1.3   | 0.7   | 303   |  1343   | 303   |  7.5    |
     *  6  | 60    | 1.5   | 0.4   | 404   |   454   |       |         |
     *  7  | 30    | 0.6   | 0.5   |       |         | 101   |  3.4    |
     *
     *  FEATSET_CONS_1_Dai_cat_n_303_lower: 45 X_1_1 + 80 X_2_1 + 40 X_4_1 + 103 X_5_1 <= 1343
     *  FEATSET_CONS_1_Dai_cat_n_404_lower: 56 X_3_1 + 60 X_6_1 <= 454
     *
     *  FEATSET_CONS_2_total_cat_p_101_lower: 1.3 X_0_1 + 0.5 X_0_3 + 0.6 X_7_1 + 0.5 X_7_3 <= 3.4
     *  FEATSET_CONS_2_total_cat_p_303_lower: 1.0 X_1_1 + 0.4 X_1_3 + 0.7 X_2_1 + 0.3 X_2_3 + 1.8 X_4_1 + 0.8 X_4_3 + 1.3 X_5_1 + 0.7 X_5_3 <= 7.5
     *
     */

    QMap<QString, QStringList> mslFeatSetCons;
    QMap<QString, QString> msFeatureSetConsLabel;

    // ----- FeatureSet Constraint value structure --------------
    // - the order of constraints in those vectors below id governed
    //   bye the order of constraints in the mslFeatSetCons map
    // for each feature set constraint, we need a vector of unique ids
    //               uid      0: fsc_j_seg, 1: fsc_rhs
    std::vector<QMap<size_t, QStringList> > mvvFSCUniqueIDSegmentRHSMap;
    std::vector<QMap<size_t, unsigned int> > mvvFSCUniqueIDCounterMap;



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
    double mdAreaScalingFactor;

    double mdMinAlloc;

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
    int addFeatureSetConsDb(void);
    int addZoneCons(void);
    int addCriCons(void);

    int isSolveCanceled(void);
    void forwardLpLog(const char* log);

    /* converts the user specified area into area units
      (same units as the specified area field) */
    double convertAreaUnits(double input, AreaUnitType otype);
    double convertAreaUnits(double input, const QString& otype, const QStringList& zoneSpec);

    // ===========================================================
    //                      NL PROCESSING
    // ============================================================

    enum NLSegment {
        NL_SEG_UNKNOWN = 0,
        NL_SEG_C = 1,
        NL_SEG_L = 2,
        NL_SEG_O = 3,
        NL_SEG_x = 4,
        NL_SEG_r = 5,
        NL_SEG_b = 6,
        NL_SEG_k = 7,
        NL_SEG_J = 8,
        NL_SEG_G = 9,
        NL_SEG_NL = 10
    };

    bool createSegmentFile(QFile*& file, NMMosra::NLSegment segType, const QString& abbr="");

    long mConsCounter;
    long mObjCounter;
    long mLogicConsCounter;

    QMap<size_t, size_t> mvNonLinearConsVarCounter;
    QMap<size_t, size_t> mvNonLinearObjVarCounter;


    //QSet<QString> mSNonLinearConsVarCounter;
    //QSet<QString> mSNonLinearObjVarCounter;

    static const QString msOPTIONS;
    static const QString msSDU;
    static const QStringList mslLowDims;

    QStringList mslDecisionVars;
    QStringList mslProcessVars;
    QStringList msBinaryVars;
    QStringList msIntVars;
    QMap<QString, QString> mmLookupColsFstParam;


    QMap<QString, QStringList > mmslVarBoundsMap;
    QMap<QString, VarAdmin> mmVarAdminMap;
    QMap<QString, long> mmDimLengthMap;
    QMap<QString, QSet<QString> > mmDimEqnMap;
    QMap<QString, QStringList> mmHighDimEqnMap;
    QMap<QString, QSet<QVector<long long> > > mmslHighDimValComboTracker;

    QMap<QVector<long long>, long long> mmvHighDimLoopIterLength;

    // segment stores
    //std::vector<QString> mvC_seg;
    //std::vector<QString> mvL_seg;
    //std::vector<std::vector<QString> > mvO_seg;
    //std::vector<QString> mvJ_seg;
    //std::vector<QString> mvG_seg;
    //static const size_t miOsegSizeLimit;

    std::vector<QFile*> mvfC_seg;
    std::vector<QFile*> mvfL_seg;
    std::vector<QFile*> mvfO_seg;
    std::vector<QFile*> mvfJ_seg;
    std::vector<QFile*> mvfG_seg;

    size_t mJAccessCounter;
    size_t mCAccessCounter;

    QString mx_seg;
    QString mb_seg;
    QString mr_seg;
    QString mk_seg;

    QFile* mpNLFile;

    // for each explict area constraint, vector of var offsets
    std::vector<std::vector<size_t> > mvExplicitAreaConsVarOffsets;
    // for each implict area constraint, vector of var offsets
    std::vector<std::vector<QString> > mvvImplicitAreaConsSegments;

    // <eqn name, < <high dims>,<all eqn's dimensions> > >
    QMap<QString, std::pair<QStringList, QStringList> > mmslHighDimLoopEquations;

    // < column, i.e. varoffset >, <non-zero count>
    QMap<size_t, size_t> mNonZeroJColCount;

    bool processVariables(QString &b_seg, QString& x_seg, long *n_vars, NMOpenGA* oga=nullptr);
    bool processLoopDimensions(void);
    void identifyHighDimLoopEquations(
            const QString& rootEqnName,
            const QString& subEqnName);


    double getRecordValue(
            QMap<QString, std::vector<size_t> >& nameValPosMap,
            std::vector<otb::AttributeTable::ColumnValue>& getvalues,
            const QString& paramName,
            const size_t optionsIdx = 0
            );

    bool populateEquations(
            const QString& eqnName,
            QMap<QString, QStack<std::vector<int> > > &activeSegment,
            int nestingLevel,
            const NLSegment& nlSeg,
            long& segCounter,
            QMap<QString, QMap<QString, int> > &eqnLoopCounterMap,
            QMap<QString, double>& dimValueMap,
            QMap<QString, std::vector<size_t> >& nameValPosMap,
            std::vector<otb::AttributeTable::ColumnValue>& getvalues
            );

    void removeLoopCounter(
            QMap<QString, QMap<QString, int> > &eqnLoopCounterMap,
            const QString& dimName,
            const QString& eqnName
            );

    void removeNLFiles(void);
    void clearInternalNLDataStructures(void);
    //bool processNLSegmentData(void);
    bool processNLSegmentData2(void);

    bool populateHighDimLoopEquations(
            const QString& hdeqn,
            otb::SQLiteTable::Pointer& sqltab,
            QMap<QString, std::vector<size_t> >& nameValPosMap,
            std::vector<std::string>& getnames,
            std::vector<otb::AttributeTable::ColumnValue>& getvalues
            );

    bool populateHDLE(
            const QString& hdeqn,
            otb::SQLiteTable::Pointer& sqltab,
            QMap<QString, std::vector<size_t> >& nameValPosMap,
            std::vector<std::string>& getnames,
            std::vector<otb::AttributeTable::ColumnValue>& getvalues
            );


    bool populateLowDimAndOtherEquations(
            otb::SQLiteTable::Pointer& sqltab,
            QMap<QString, std::vector<size_t> >& nameValPosMap,
            std::vector<std::string>& getnames,
            std::vector<otb::AttributeTable::ColumnValue>& getvalues
            );


    bool writeNLSegment(const QString &sseg, QFile *&fseg, const NLSegment& segType, int writeFile = -1);

    bool bkprestoreLUCControl(bool bBackup);

    //bool

    // ===========================================================
    //                      EQUATION PARSING
    // ============================================================

    enum EqnElement {
        EQN_LOOP=1,
        EQN_FUNCTION=2,
        EQN_PARAMETER=3,
        EQN_EQUATION=4,
        EQN_NUMBER=5,
        EQN_OPERATOR=6,
        EQN_LBRACE=7,
        EQN_RBRACE=8,
        EQN_UNKNOWN=0
    };

    // eqn obj type : 1
    struct Loop
    {
        static const EqnElement type = EQN_LOOP;
        QString name;
        QString dim;
        int level = -1;
        int bodyStart = -1;
        int bodyEnd = -1;

        std::vector<int> sections;
    };

    // eqn obj type : 2
    struct Func
    {
        static const EqnElement type = EQN_FUNCTION;
        QString name;
        std::vector<int> sep;
        int level = -1;
        int bodyStart = -1;
        int bodyEnd = -1;
    };

    // eqn obj type : 3
    // NOTE: a 'parameter' is here defined as
    // a named numerical value, which, in the
    // context of the optimisation problem at
    // hand, could be either a descision variable
    // or a pre-defined parameter
    struct Param
    {
        static const EqnElement type = EQN_PARAMETER;
        QString name;
        QStringList dimensions;
        int paramStart = -1;
        int paramEnd = -1;
    };

    // eqn obj type : 4
    struct Equation
    {
        static const EqnElement type = EQN_EQUATION;
        QString eqn;
        int eqnStart = -1;
        int eqnEnd = -1;
    };

    // eqn obj type : 5
    struct Number
    {
        static const EqnElement type = EQN_NUMBER;
        int numStart = -1;
        int numEnd = -1;
        double value = 0;
    };

    // eqn obj type : 6
    struct Operator
    {
        static const EqnElement type = EQN_OPERATOR;
        QString op;
        int opStart = -1;
        int opEnd = -2;
    };


    struct EquationAdmin {
        QString name;
        // <pos in revPolPrefix list>,
        //      <eqn element type, offset into type list>
        QMap<int, std::pair<EqnElement, int> > elemMap;
        //QMultiMap<int, Loop> loopMap;
        QList<Loop> loopList;
        QList<Func> funcList;
        QList<Param> paramList;
        QList<Equation> eqnList;
        QList<Number> numList;
        QList<Operator> opList;
    };

    // <name>, <parsed and itemized equation stored in a handy object>
    QMap<QString, EquationAdmin> mmEquationAdmins;

    // <name>, <EquationAdmin>, <polish prefix notation eqn elements>
    QMap<QString, std::pair<EquationAdmin, QStringList> > mmPrefixEquations;


    bool parseEquation(const QString eqn,
            EquationAdmin& ea,
            bool blogEndPos = false
            );

    bool polishingEqn(EquationAdmin& ea, EquationAdmin &pa,
                      QStringList& polishPrefix);
    QString reverseEqn(const QString& eqn, EquationAdmin& ea,
                       int offset, int end);

    bool shuntyYard(const QString& revInEqn,
                    QStringList &polishPrefixList,
                    EquationAdmin& ea,
                    EquationAdmin& pa
            );


    QStringList shuntyCoreRev(const QString& revEqn,
                       EquationAdmin& ea,
                       EquationAdmin& pa,
                       int offset, int outlistoffset//,
                       //QMap<QString, std::pair<Loop, QString> > *pRevLoopMap=nullptr
                       );
    QStringList shuntyCorePref(const QStringList& revList,
                               EquationAdmin& inEa,
                               EquationAdmin& outEa);

    void PrintElemMap(const EquationAdmin& admin);

    bool createEqnElemAdminMap(
            QMap<QString, QSet<QString> > &dimEqnMap,
            QMap<QString, std::vector<size_t> >& nameValPosMap,
            QMap<QString, int>& procVarInObjConsMap,
            std::vector<std::string>& getnames,
            std::vector<otb::AttributeTable::ColumnValue>& getvalues,
            const QString& eqnname);

    bool getParameterValue(
            double& pVal,
            const QString& eqnName,
            const NMMosra::Param& pa,
            std::vector<otb::AttributeTable::ColumnValue>& getvalues,
            QMap<QString, std::vector<size_t> >& nameValPosMap,
            QMap<QString, QMap<QString, int> > &eqnLoopCounterMap,
            QMap<QString, double>& dimValueMap
            );

    static QList<QChar> numchar(){
        QList<QChar> nchlist;
        nchlist << '0' << '1' << '2'
                << '3' << '4' << '5'
                << '6' << '7' << '8'
                << '9' << 'e' << '+'
                << '-' << '.';
        return nchlist;
    }
    static const QList<QChar> mNumCharList;

    static QList<QChar> ws(){
        QList<QChar> lws;
        lws << ' ' << '\n' << '\r'
            << '\t' << '\t' << '\b'
            << '\v' << '\f';
        return lws;
    }
    static const QList<QChar> mWhitespaces;

    static QStringList lnames(){
        QStringList lnam;
        lnam << "for" << "sum" << "mult";
        return lnam;
    }
    static const QStringList mLoopNames;

    static QList<QChar> opchar(){
        QList<QChar> opcharList;
        opcharList << '+';
        opcharList << '^';
        opcharList << '<';
        opcharList << '=';
        opcharList << '-';
        opcharList << '!';
        opcharList << '>';
        opcharList << '*';
        opcharList << '&';
        opcharList << '<';
        opcharList << '/';
        opcharList << '|';
        opcharList << '>';
        return opcharList;
    }
    static const QList<QChar> mOpCharList;

    static QMap<QString, int> aov(){
        QMap<QString, int> mapov;
        mapov.insert("and" , 21);
        mapov.insert("or"  , 20);
        mapov.insert("lt"  , 22);
        mapov.insert("le"  , 23);
        mapov.insert("ge"  , 28);
        mapov.insert("gt"  , 29);
        mapov.insert("eq"  , 24);
        mapov.insert("ne"  , 30);
        return mapov;
     }
    static const QMap<QString, int> mAMPLOperators;

    static QMap<QChar, QChar> rl(){
        QMap<QChar, QChar> maprl;
        maprl.insert('(', ')');
        maprl.insert('{', '}');
        maprl.insert('[', ']');
        return maprl;
    }
    static const QMap<QChar, QChar> mReverseLeft;// = rl();

    static QMap<QChar, QChar> rr(){
        QMap<QChar, QChar> maprr;
        maprr.insert(')', '(');
        maprr.insert('}', '{');
        maprr.insert(']', '[');
        return maprr;
    }
    static const QMap<QChar, QChar> mReverseRight;// = rr();


    static QMap<QString, int> po(){
        QMap<QString, int> mappo;
        mappo.insert("+", 0);
        mappo.insert("-", 1);
        mappo.insert("*", 2);
        mappo.insert("/", 3);
        mappo.insert("^", 5);
        mappo.insert("<", 6);
        mappo.insert("||", 20);
        mappo.insert("&&", 21);
        mappo.insert("<=", 23);
        mappo.insert("=", 24);
        mappo.insert(">=", 28);
        mappo.insert(">", 29);
        mappo.insert("<>", 30);
        mappo.insert("!=", 30);
        mappo.insert("!", 34);
        return mappo;
    }
    static const QMap<QString, int> mParseOperators;// = po();

    static QMap<QString, int> opLevel(){
        //6 ^
        //5 * /
        //4 + -
        //3 < > = <= >= <> !=
        //2 ! not
        //1 && and
        //0 || or
        QMap<QString, int> lvlmap;
        lvlmap.insert("+", 4);
        lvlmap.insert("^", 6);
        lvlmap.insert("<", 3);
        lvlmap.insert("=", 3);
        lvlmap.insert("-", 4);
        lvlmap.insert("!", 2);
        lvlmap.insert(">", 3);
        lvlmap.insert("*", 5);
        lvlmap.insert("&&", 1);
        lvlmap.insert("<=", 3);
        lvlmap.insert("/", 5);
        lvlmap.insert("||", 0);
        lvlmap.insert(">=", 3);
        lvlmap.insert("<>", 3);
        lvlmap.insert("!=", 3);
        return lvlmap;
    }
    static const QMap<QString, int> mmOpLevel;

    static QMap<QString, int> amplfunc(){
        QMap<QString, int> amfuncmap;
        amfuncmap.insert("floor", 13);
        amfuncmap.insert("neg"  , 16);
        amfuncmap.insert("tan"  , 38);
        amfuncmap.insert("sin"  , 41);
        amfuncmap.insert("exp"  , 44);
        amfuncmap.insert("atanh", 47);
        amfuncmap.insert("asin" , 51);

        amfuncmap.insert("ceil" , 14);
        amfuncmap.insert("not"  , 34);
        amfuncmap.insert("sqrt" , 39);
        amfuncmap.insert("log10", 42);
        amfuncmap.insert("cosh" , 45);
        amfuncmap.insert("atan" , 49);
        amfuncmap.insert("acosh", 52);

        amfuncmap.insert("abs"   , 15);
        amfuncmap.insert("tanh"  , 37);
        amfuncmap.insert("sinh"  , 40);
        amfuncmap.insert("log"   , 43);
        amfuncmap.insert("cos"   , 46);
        amfuncmap.insert("asinh" , 50);
        amfuncmap.insert("acos"  , 53);

        amfuncmap.insert("plus"   , 0 );
        amfuncmap.insert("div"    , 3 );
        amfuncmap.insert("less"   , 6 );
        amfuncmap.insert("atan2"  , 48);
        amfuncmap.insert("round"  , 57);

        amfuncmap.insert("minus"  , 1 );
        amfuncmap.insert("rem"    , 4 );
        amfuncmap.insert("intdiv" , 55);
        amfuncmap.insert("trunc"  , 58);

        amfuncmap.insert("mult"      , 2 );
        amfuncmap.insert("pow"       , 5 );
        amfuncmap.insert("precision" , 56);
        amfuncmap.insert("iff"       , 73);

        amfuncmap.insert("max"  , 12);
        amfuncmap.insert("min"  , 11);
        amfuncmap.insert("sum"  , 54);
        amfuncmap.insert("count", 59);

        amfuncmap.insert("and"  , 70);
        amfuncmap.insert("or"   , 71);

        amfuncmap.insert("numberof", 60);
        amfuncmap.insert("numberofs", 61);

        amfuncmap.insert("if", 35);
        amfuncmap.insert("ifs", 65);
        amfuncmap.insert("implies", 72);

        return amfuncmap;
    }
    static const QMap<QString, int> mAMPLFunctions;

};

#endif /* NMMOSRA_H_ */
