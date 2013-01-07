MODELLING README
Author: Alexander Herzig
Copyright 2012, 2013 Landcare Research New Zealand Ltd 

++++++++++++++++++++++++++++++++++++++++++
HOW TO USE THE LUMASS MODELLING FRAMEWORK
++++++++++++++++++++++++++++++++++++++++++

1. Principles
2. Building a model
   2.1 Model building blocks
   2.2 Creating model components
   2.3 Rules to rule
3. Executing a model

===========================================
1. PRINCIPLES
===========================================

The modelling framework is built upon the ITK/OTB image
processing pipeline. It consists of two fundamental
conceptual components: data and algorithms. The algorithms
are working on the data to produce new data, which,
in turn, can feed into other algorithms. Many data
objects and algorithms, connected via their in-/output
relationships, constitute the ITK/OTB processing pipeline.


===========================================
2. BUILDING A MODEL
===========================================

2.1 Model building blocks
~~~~~~~~~~~~~~~~~~~~~~~~~

LUMASS models are built from two main type of components:
* Process components, and
* Aggregate components.
Process components are the real working horses and actually
process data. They can be plugged together via input-/output
relationships (s. below) to build ITK/OTB-based processing
pipelines, which support multi-threading as well as 
streamed processing of very large data sets. Aggregate
components are used to organise model components. 
They represent a group of process and/or aggregate components
and are useful to build larger processing units encapsulating
a specific task or sub model. 
Both component types support repetitive execution and the
notion of time levels, thus enabling the implementation 
of dynamic process models operating on different time 
scales. 


2.2 Creating model components
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Process components are created by dragging entries from
the list of model components into the model view window.
They can be edited by double clicking them and changing 
their individual parameters on the component property dialog.
Model components can also be processed by using the tools 
associated with the model view (top tool bar). It provides 
classic tools such as zoom-in, zoom-out, pan, and select.
Additionally, the link tool can be used to plug process components
together to form a processing pipeline. Further processing
options are available from the context menu of the model view
window. For example, two or more selected model components
can be grouped together to build an aggregate component, thus
allowing for building a hierarchy of components and sub components.
Model components grouped together are automatically put onto
the next higher time level (i.e. host's time level plus one). 
However, this can later be changed to any level equal to or 
higher than the host's time level. Selected components sharing
a time level can be 'un-grouped' via the context menu. It
moves them from their current group (or host component) to the
group's host component and puts them on the same time level as 
the new host's time level. 
Component properties can be edited by double clicking a model
component. 


2.3 Rules to rule
~~~~~~~~~~~~~~~~~

* A (connected) processing pipeline (i.e. a streamable pipeline)
  cannot cross time levels. Processing pipelines across time
  levels are at least partly disconnected, which means that the
  higher level part of the pipeline is executed first and then,
  according to the order of time levels, any lower parts of 
  the pipeline. Data streaming is not possible within disconnected
  pipelines. For processing large data sets, you should consider
  writing the data at the end of a (connected) pipeline to disk and
  re-read the data at the beginning of a new time level, if required.  

* Model components are executed in order of their (relative) time levels.
  Components sitting on the same time level don't have a particular
  order of execution, which is guaranteed by the system (though
  this should be in the order they're listed as sub components
  by their host). 

* Data providing components either have to be part of the
  processing pipeline or have to sit on a higher time level
  than the component they're feeding into.


===========================================
3. EXECUTING A MODEL
===========================================
  
Each and every individual model component (process as well as aggregate components)
can be executed individually. If the model component requires input data
from other, possibly disconnected components, it tries to fetch the data
upon execution. Hence, in most cases it only makes sense to execute  
aggregate components since they ensure their sub-components to be initialised
and linked properly with other components prior to model execution. Executing
individual process components only makes sense, when they are 'self contained'
(e.g. the CostDistanceBuffer component) and don't depend on any other component.
Model runs can be aborted by clicking the STOP EXECUTION button below the model
view window. Note that pressing this button only triggers a request with the 
model controller to stop the currently running model. This may not happen 
immediately, and depends on the implementation of the individual process components,
however, model execution should be stopped at the latest before the execution of
the next process component.    
