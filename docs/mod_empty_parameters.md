---
title: "Empty Model Parameters"
permalink: "/docs/mod_empty_parameters"
--- 
<link rel="shortcut icon" type="image/x-icon" href="../LUMASS_icon_64.ico">

Where applicable, missing or omitted parameter values in LUMASS are indicated by an 'empty' component property value (i.e. an empty string). Please note that empty strings are provided by leaving the specific parameter value in the property value editor blank (cf. image below). Do not enter `""` or `''`! For example, you want to use the [Image2Table]({{ "/docs/cref_image2table" | relative_url }}) component to export a single column of image values from a 3D image into a table. Since the x and z-coordinates are constant for all exported column values, you only want to write out the pixel coordinates for the y-dimension (i.e. the row indices) into an output table column called `row_id`. To achieve that, you specify empty paramters for the x and z-dimension and `row` for the y-dimension of the `DimVarNames` property (s. image below). Note that LUMASS automatically appends `_id` to each non-empty `DimVarNames` parameter.

[![Empty_nD_Parameter]({{ "/assets/images/docs/empty_nd_parameter.png" | relative_url }}){: width="70%", height="70%"}]({{ "/assets/images/docs/empty_nd_parameter.png" | relative_url }})<br>
**Empty nD model parameter**