MODELLING README
Author: Alexander Herzig
Copyright 2012 Landcare Research New Zealand Ltd 

++++++++++++++++++++++++++++++++++++++++++
HOW TO USE THE LUMASS MODELLING FRAMEWORK
++++++++++++++++++++++++++++++++++++++++++

1. Principles
2. Building a model
   2.1 Component order counts
   2.2 
3. Executing a model


===========================================
1. PRINCIPLES
===========================================

1.1 Data and algorithms

The modelling framework is built upon the ITK/OTB image
processing pipeline. It consists of two fundamental
components: data and algorithms. The algorithms  are
working on the data to produce new data, which,
in turn, can feed into other algorithms. Many data
objects and algorithms, connected via their in-/output
relationships, constitutes the ITK/OTB processing pipeline.

1.2 


===========================================
2. Building a model
===========================================

2.1 Component order counts


2.2 Repetitive execution of a model component

- the number of parameters recognised by the 
  modelling framework depens on the number of
  input components: e.g. if you want to 
  iteratively process one image applying different
  equations, you have to repeat the input parameter
  as often as the number of different equations 
  you want to process.  
