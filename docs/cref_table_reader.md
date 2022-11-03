---
title: "TableReader"
permalink: "/docs/cref_table_reader"
--- 

## Properties

 Property | Characteristic | Dimension | Description 
----------|----------------|:-----------:|-------------
CreateTable | optional,<br>user-defined, default: `False`  | 0 | Whether or not the table specified by `FileName` shall be created by the `TableReader`. If `Create Table` it set to `False`, `TableReader` assumes the specified table (`FileName`) already exists.  
FileName | compulsory<br>user-defined, default: `<empty>` | 1 | The name of the table to be (created and) opened (read/write). 
TableName | optional,<br>user-defined, default: `<empty>` | 1 | In case `FileName` specifies an SQLite database file, `TableName` specifies the table within the database to be opened (read/write). 


## Supported Image & Pipeline Features

Characteristic | Details 
---------------|---------------
Image dimensions | N/A, 1D, 2D, 3D  
Multi-band images | yes
Pipeline role | source
Sequential processing | yes
Parallel processing | no

## Overview

The `TableReader` process component reads a 

## Supported Table Formats


*.dbf, *.xls, *.csv, *.db, *.ldb