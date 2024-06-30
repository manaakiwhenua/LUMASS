---
title: "Model Components"
permalink: "/docs/mod_structure"
---
<link rel="shortcut icon" type="image/x-icon" href="../LUMASS_icon_64.ico">

[![Hierarchical_Model_Structure]({{ "/assets/images/docs/model_structure_exec_order.png" | relative_url }})]({{ "/assets/images/docs/model_structure_exec_order.png" | relative_url }})<br>
**Model structure and execution order**

LUMASS’ spatial modelling framework is built around two core components: i) data and ii) processes. The process components work on their input data to produce output data. Each process component represents a self-sufficient basic (spatial) algorithm that only depends on appropriate input data and a set of parameters. For example, the process component labelled `extract majorcat` (s. bottom left corner in the figure above), extracts higher order catchment identifiers from a given attribute of the input layer’s attribute table. The new image data created by this process (i.e. its output), is passed on as input data to the next processing component `create RAT`. This component summarises the aggregated catchment data it receives and creates an attribute table containing a record for each higher order catchment. The received input data together with the newly created attribute table constitutes this component’s output data and is passed on to the `ImageWriter` component that stores the image together with its associated attribute table as image file on disk. Such a sequence of process components, concatenated by their respective output and input data, is referred to as a **processing pipeline**.<br>

Some components can only sit either at the start (**source** components, e.g. [ImageReader]({{ "/docs/cref_image_reader" | relative_url }})) or at the end (**sink** components, e.g. [ImageWriter]({{ "/docs/cref_image_writer" | relative_url }})) of a pipeline. Only the **data** components [DataBuffer]({{ "/docs/cref_databuffer" | relative_url }}) and [DataBufferReference]({{ "/docs/cref_databuffer" | relative_url }}) can sit at either end of a processing-pipeline. **Stand-alone** components, such as [CostDistanceBuffer]({{ "/docs/cref_costdistancebuffer" | relative_url }}) and [ExternalExec]({{ "/docs/cref_external_exec" | relative_url }}) cannot be connected to a processing-piepline at all. All their required inputs and produced outputs are configured via their component properties. The reference page ([Model Component Reference]({{ "/docs/mod_component_reference" | relative_url }})) for each model component provides detailed information as to how a particular component can be used and whether it could be linked into a processing-piepline.<br> 

Stand-alone process components and processing pipelines can be combined to **aggregate components**. This might be simply done to group several pipelines contributing to the same higher level process, or to enable the repetitive execution of a group of components. For example, the aggregate component `MarkCat` (top right in the figure above) is executed three times in succession, which is indicated by the number `3` next to the circular arrow symbol in its title bar. It means that the processing pipeline hosted by the component is executed three times in a row. During each iteration, it reads the same sub-catchment image file and its associated raster attribute table (`cat`) and identifies for a different catchment in each iteration all of its upstream catchments and writes a new higher order catchment identifier into the raster attribute table (`mark major catchments`). Aggregate components may be nested inside each other to construct complex hierarchical processing workflows. Together with the capability of repetitive execution of components ([Branching and looping]({{ "/docs/mod_execution_control#branching-and-looping" | relative_url}})), it additionally enables the development of models operating on multiple temporal scales. 

## Properties and Parameters

In the following sections we will often refer to parameters and properties. To avoid any confusion, we here define their meaning in the context of the LUMASS modelling framework.

### Parameters

In a LUMASS model, a parameter denotes a constant value over a single execution of a process component. It may be a text (string) or numeric value and references specific data or a specific mode of computation. For example, a parameter may specify a particular characteristic of an image, such as its number of bands, a particular table, column, or row in a SQL expression, a constant value in a mathematical equation, or a particular mode of computation, e.g. stripped versus tiled streaming.

### Properties

A property refers to a particular characteristic of a model component. It ties a model parameter value to a specific model component and represents the technical means by which a model parameter is supplied to an aggregate or process component. The table below provides a list of properties shared across model and process components respectively. In addition to these general properties, individual process components are characterised by further properties depending on their specific functionality. Please refer to the [Process component reference]({{ "/docs/mod_component_reference" | relative_url }}) section for a comprehensive overview.

#### Property Dimensions

Component properties have different dimensions depending on the dimensionality of the parameter they represent and whether or not the parameter is *dynamic*, i.e. can be changed when the component is executed repetitively. 

- **0-D** properties represent either simple numbers, e.g. `TimeLevel` or `NumIterations`, strings, e.g. `UserID` or `Description`, boolean values, e.g. `WriteTable`, or values of pre-defined lists, e.g. `NMInputComponentType` or other specific process component properties. Most of the represented parameters are static and do not change during a model run, e.g. `Description`, `NMInputComponentType`, or `OutputNumDimensions`. The only 0-D parameter that can be changed dynamically at model runtime through [parameter expressions]({{ "docs/mod_dynamic_parameters#parameter-expressions" | relative_url }}) is `UserID`.
- **1-D** properties represent [dynamic lists]({{ "docs/mod_dynamic_parameters#parameter-lists" | relative_url }}) of single parameter values of type string, e.g. `NumIterationsExpression` or `SQLStatement` ([SQLProcessor]({{ "/docs/cref_sql_processor" | relative_url }})), or number, e.g. `Nodata` ([MapKernelScript2]({{ "/docs/cref_map_kernel_script_2" | relative_url }})). During the [iterative execution]({{ "/docs/mod_execution_control#branching-and-looping" | relative_url}}) of a model component, at each iteration step an individual parameter value is taken from the list and passed to the underlying process or aggregate component. For more detailed information please refer to the [Parameter lists]({{ "docs/mod_dynamic_parameters#parameter-lists" | relative_url }}) section. In the [Component Properties]({{ "/docs/gui" | relative_url }}) pane of the graphical user interface, 1-D properties are referred to as `QStringList` when hovering over the value of a property.    
- **2-D** properties represent [dynamic lists]({{ "docs/mod_dynamic_parameters#parameter-lists" | relative_url }}) of multi-valued parameters, i.e. parameter lists. They are used, for example, to specify the size of an image (`Size`) or image origin coordinates (`OutputOrigin`) for each image dimension for a given iteration step. In the [Component Properties]({{ "/docs/gui" | relative_url }}) pane of the graphical user interface, they are referred to as `QList<QStringList>` when hovering over the value of a property.


#### General Model and Aggregate Component Properties

| Property | Characteristic | Dimension | Description |
|----------|----------------|:-----------:|-------------|
| ComponentName            | compulsory,<br>system-defined  | 0 | unique model component name; LUMASS ensures that each model component can be uniquely identified by its ComponentName |
| UserID                   | optional,<br>user-defined      | 0 | a non-unique user-defined short name, which is used to refer to a specific model component in map algebra expressions, map kernel scripts, SQL statements, and parameter expressions |
| Description              | optional,<br>User-defined      | 0 | a short user-defined description of the component; it defaults to the ComponentName  | 
| TimeLevel                | compulsory,<br>user-defined    | 0 | the user editable time level of the model component; time levels are used to define the control flow of a model |
| Inputs                   | optional,<br>user-defined      | 2 | a list of the ComponentNames of the input components |
| IterationStep            | compulsory,<br>user-defined    | 0 | the start iteration step for the next execution of the component (default: 1), or the actual iteration step, if the component is currently being executed  |
| NumIterations            | compulsory,<br>user-defined    | 0 | the number of times the component is executed (default: 1)  |
| NumIterationsExpression  | optional,<br>user-defined      | 1 | a list of parameter expressions to dynamically define the number of times a component is executed (e.g. used for conditional iteration); this list is empty by default  |

**Note**: Only **aggregate** and **process** components possess iteration related properties (`IterationStep`, `NumIterations`, `NumIterationsExpression`). **Data** components (`DataBuffer`, `DataBufferReference`) do not possess these. However, they may still feature in repeatedly executed pipelines and receive or provide data that is produced in a given interation.
{: .notice--info}