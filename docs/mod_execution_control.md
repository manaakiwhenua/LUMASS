---
title: "Execution Order and Control Flow"
permalink: "/docs/mod_execution_control"
---

## Execution Sequence

[![Hierarchical_Model_Structure]({{ "/assets/images/docs/model_structure_exec_order.png" | relative_url }})]({{ "/assets/images/docs/model_structure_exec_order.png" | relative_url }})<br>
**Model structure and execution order**

The model component `create major cat file` (s. figure above) extracts three aggregated higher order catchments from a provided sub-catchment file and writes them together with an associated attribute table into a new image file. The overall functionality is broken down into smaller processing steps, which, executed in the right order, provide the desired result. To control the execution sequence of process and aggregate components, LUMASS uses **time levels** that are assigned to each individual model component. The time level is shown next to a clock symbol in the top left corner of each component. Execution flows from higher time levels to lower time levels and from higher aggregation levels to lower aggregation levels, i.e. from the outside to the inside. For example, the execution sequence of the `create major cat file` component is as follows (with time levels given as TL): 

- **TL 9**: `create major cat file`
    - **TL 15**: `PrepareCatMarking`
      - **TL 15**: Pipeline to prepare the raster attribute table (RAT) (e.g. add columns, etc.)
    - **TL 12**: `MarkMajorCatchments`
      - **TL 13**: `MarkCat` (*note: executed 3 times in a row*)
        - **TL 13**: Pipeline to mark major catchments
      - **TL 12**: Pipeline to mark other catchments
    - **TL 9**: `WriteMajorCatchmentFile`
        - **TL 9**: Pipeline to extract majorcat and create RAT

Process components that are part of a processing pipeline and that share the same host component (i.e. aggregate component), have to sit on the same time level to be properly initialised prior to pipeline execution. Note that, counter intuitively to the overall down-stream execution flow, pipeline execution always starts at the bottom end of each pipeline (pull model). That means the position of a pipeline’s bottom end component in the model hierarchy determines when the pipeline is executed. Consequently, the number of iterations of a pipeline-end’s host component determines the number of times the whole pipeline is repeated in sequence. This implies that a processing pipeline may reach across different aggregate components as long as it follows the general rule of down-stream execution (and thus data) flow. In other words, individual processing components that are linked into a processing pipeline, may only provide input to other process components that sit either on the same time level and share the same host component or that are positioned down-stream with regard to the overall model hierarchy. Components that are not part of a processing pipeline but share the same host component and time level, may be executed in an arbitrary order. 
Time levels are user-defined ([General model and aggregate component properties]({{ "docs/mod_structure#general-model-and-aggregate-component-properties" | relative_url }})) and do not necessarily have to be strictly sequential, i.e. the sequence may omit individual numbers. For example, the internal execution sequence inside the create major cat file component starts at time level 15, which is followed by time level 13. The only rule LUMASS enforces is that the minimum time level of child components inside an aggregate component must not be smaller than the time level of the component itself (i.e. their host or parent component). For example, when the hierarchy of a model is changed by moving or cloning components from one component to another, if required, LUMASS automatically adjusts the time levels of the inserted components according to this rule. Additionally, LUMASS provides efficient means to change and adjust time levels for multiple components at a time. 

## Branching and Looping

### Iterative Execution

Process and aggregate components can be executed iteratively to facilitate batch-processing, scenario modelling, and sensitivity or uncertainty analyses. How often an aggregate or a stand-alone processing component is executed when its position in the [execution sequence]({{ "/docs/mod_execution_control#execution-sequence" | relative_url}}) is reached, is controlled by its properties `IterationStep`, `NumIterations`, and `NumIterationsExpression`. `IterationStep` controls at what iteration step the execution should be started. It is automatically increased during model execution after each iteration and controls which parameter on its contained individual process components' parameter lists to apply at the given iteration step. 

While all process components provide the above mentioned properties to control iterations, they should not be used for individual components that are part of a processing pipeline. Best practise is to embed processing-pipelines and stand-alone process compoments, e.g. `ExternalExec` and `CostDistanceBuffer`, into aggregate components that are executed iteratively.

### Conditional Execution

Conditional execution in a LUMASS model is possible at two levels: i) the pixel or record level, and ii) the component level. Conditional statements at the pixel level are provided by the `MapAlgebra`, `MapKernelScript2`, and `JSMapKernelScript` components and at the record level by the `SQLProcessor` component. While these enable the conditional computation of individual pixel or table record values, conditional execution at the component level enables the controlled execution of aggregated components representing higher level processes. This is implemented through the `NumIterationsExpression` property ([General model and aggregate component properties]({{ "docs/mod_structure#general-model-and-aggregate-component-properties" | relative_url }})) that enables the dynamic definition (s. [Parameter expressions]({{ "/docs/mod_dynamic_parameters#parameter-expressions" | relative_url}})) of the number of times an aggregated component is executed. If that expression evaluates to zero the respective component is not executed all. However, if no `NumIterationsExpression` is provided, the number of iterations of an aggregate component is defined statically using its `NumIterations` property ([General model and aggregate component properties]({{ "docs/mod_structure#general-model-and-aggregate-component-properties" | relative_url }})). In that case a model component can be manually disabled, i.e. excluded from a model run, by setting the `IterationStep` property such that its value is greater than that of the `NumIterations` property (cf. [Parameter lists]({{ "/docs/mod_dynamic_parameters#parameter-lists" | relative_url }})). For aggregate components this will be indicated in the model view by making its background colour transparent. 

**Note**: While `NumIterationsExpression` is mostly used with aggregate components to control the execution of a set of processing pipelines or sub-components, it can also be used with source (process) components sitting at the top of a processing pipeline. This is useful to configure an optional input component to another process component. For example a `MapAlgreba` component could have an optional input component depending on the availability of a specific input image file. The `ImageReader` component, responsible for making this image available to the `MapAlgebra` component, could use its `NumIterationsExpression` to check the availability of that specific input file and evaluate to zero, if the file is not available.  
{: .notice--info}
