PROJECT(NMQt)

set(NMQSQLite_SRC_DIR ${NMQt_SOURCE_DIR}/drivers/sqlite
    CACHE FILEPATH "Path to LUMASS internal copy of Qt's QSQLiteDriver code")
mark_as_advanced(FORCE NMQSQLite_SRC_DIR)

