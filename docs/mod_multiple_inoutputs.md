---
title: "Multiple Inputs and Outputs"
permalink: "/docs/mod_multiple_inoutputs"
--- 

While most processing components that can be included into a processing pipeline only accept one input component or only produce one output component, a few components either accept more than one input and/or produce more than one output:

- [BMIModel]({{ "docs/cref_bmi_model" | relative_url }})
- [ImageWriter]({{ "docs/cref_image_writer" | relative_url }})
- [JSMapKernelScript]({{ "docs/cref_js_map_kernel" | relative_url }})
- [MapAlgebra]({{ "docs/cref_map_algebra" | relative_url }})
- [MapKernelScript2]({{ "docs/cref_map_kernel_script_2" | relative_url }})
- [RAMFlowAcc]({{ "docs/cref_ram_flowacc" | relative_url }})
- [SQLProcessor]({{ "docs/cref_sql_processor" | relative_url }})
- [SummarizeZones]({{ "docs/cref_sum_zones" | relative_url }})
- [UniqueCombination]({{ "docs/cref_unique_combination" | relative_url }})

## Linking Multiple Input Images to a Component

Components that accept more than one input, refer to different input images using the input compnents' `UserID`s. Components that expect inputs of a specific type or content, e.g. `SummarizeZones` or `RAMFlowAcc`, expect different inputs to be indicated by specific names (`UserID`), e.g. `ZoneImage` or `DEM`. Please refer to the individual component's reference for more information.


## Linking Multiple SQLite Databases to a SQLProcessor

When multiple SQLite databases are linked as input to a `SQLProcessor` component, the first input becomes the main database. All subsequent databases linked to that component are attached to the main database. For example, in the pipeline depicted  below, `ImageReader116` provides the `main` database as it is listed as the first input component to `SQLProcessor89`. The database provided by `ImageReader118` is then attached to the database of `ImageReader116`. Thereby its `UserID` (`blackhole`) is registered as the [schema name](https://www.sqlite.org/lang_attach.html) for its database. This way all database tables from all attached databases can be accessed by the `SQLProcessor` via `schema-name.table-name`. 

[![MultipleInputsToSqlProcessor]({{ "/assets/images/docs/sqlprocessor_multiple_inputs.png" | relative_url }})]({{ "/assets/images/docs/sqlprocessor_multiple_inputs.png" | relative_url }})<br>
**Multiple `SQLProcessor` inputs**


## Assigning Names to Multiple Outputs

Since each component only has a single `UserID`, components that produce more than one output image provide other means of specifying individual output names. For example, the `MapAlgebra` component provides the property `OutputNames` (s. image [Image Access by Index or Name]({{ "/assets/images/docs/outputs_access_index_name.png" | relative_url }})) for users to specify a name for each individual output image produced. Whether or not a `BMIModel` provides multiple outputs and how they're named, can be determined using the model's API or consulting the model's documentation. 

## Linking Specific Outputs as Input to Another Component 

To link a specific output image of a processing component that produces more than one ouput, e.g. `MapAlgebra`, as input to another component, the user needs to append the name of the output image to the component's name using a colon `:` (s. 'access by name' in image below). Alternatively, the output image can also be referred to by its output index (s. 'access by index'). 

[![AccessByIndexVSName]({{ "/assets/images/docs/outputs_access_index_name.png" | relative_url }})]({{ "/assets/images/docs/outputs_access_index_name.png" | relative_url }})<br>
**Image access by index or name**
