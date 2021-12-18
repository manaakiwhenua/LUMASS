---
title: "Creating Models"
permalink: "/docs/mod_createpipeline"
--- 


... to be completed ...


### Notes 
- Some processing components that accept multiple input components, rely on the order of inputs to distinguish semantically different input images (cf. [`SummarizeZones`]({{ "/docs/cref_sum_zones" | relative_url }})). 
- The order of input images can be changed using the Component Property Editor dialog by interactively moving the list entries in the left hand pane (`shift` + `left mouse button` down: move; `ctrl` + `left mouse button` down: copy).
- To specify the nth output image of a processing component as input image to a downstream component, append the `0`-based index of the desired image to the input component name, e.g. {% raw %}`{{{ImageReader}{MapAlgebra24:1}}}`{% endraw %}.
In this case, the processing compoments has two specified inputs, `ImageReader12` and the second output image of `MapAlgebra24`, as indicated by index `1` for the second output.