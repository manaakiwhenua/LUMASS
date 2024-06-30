---
title: "Creating and Executing Models"
permalink: "/docs/mod_createpipeline"
--- 
<link rel="shortcut icon" type="image/x-icon" href="../LUMASS_icon_64.ico">

## Adding Model Components to a Model

1. Make sure the [Model View]({{ "docs/gui" | relative_url }}) (#4) and the [Model Components]({{ "docs/gui" | relative_url }}) (#8) list are visible (Main Menu >> View >> Model View Mode).
2. Drag the model component from the [Model Components]({{ "docs/gui" | relative_url }}) (#8) list into the aggregate component you want to add it to.


## Deleting Model Components From a Model

1. [Select]({{ "docs/mod_createpipeline#selecting-model-components" | relative_url }}) the model components you want to delete. 
2. Right-click on the component or link you want to delete and select `Delete <ComponentName | Link>` or `Delete <X> Components` if you selected one or more component.

## Building a Processing Pipeline

1. [Add]({{ "docs/mod_createpipeline#adding-model-components-to-a-model" | relative_url }}) the processing components to the model you want to link to a processing pipeline.
2. Select the [Link Components]({{ "docs/gui#main-toolbar" | relative_url}}) (#7) tool on the Main Toolbar. 
3. Establish the output-input links between components: 
   - Left-click on the component producing a given output and hold the mouse button down.
   - Drag the link line onto the target component receiving the produced image/RAT as input. 
   - Release the left mouse button.

    **Note**: Two processing components may share more than one output-input-link, e.g. `MapAlgebra` and `ImageWriter`. To establish multiple links between two components, just create as many links as required. However, while you will see the component providing the input images being listed multiple times in the `Inputs` list of the receiving component, only one link-line will be visible between the components. Please refer to [Multiple Inputs and Outputs] for information on how to reference multiple output/input correctly. 
    {: .notice--info}

4. Repeat step 3 until all output-input links between the pipeline components are created.
5. Deselect the Link Components tool.
6. Define/double check the components’ properties: For the error-free execution of  the processing pipeline, it is important to double check the components’ properties. 

    **Important**: An important concept of the modelling framework is that it requires an image’s data type, dimension, and number of bands (image characteristics) to be explicitly specified. Furthermore, the image characteristics of an output have to match the image characteristics of the associated downstream input. If these characteristics do not match, the pipeline will not execute properly and produce an error. 
    {: .notice--warning}

## Selecting Model Components

### Select Components Without Any Tool Being Selected

1. Make sure all tools on the Main Toolbar are deselected.
2. Press and hold the `Ctrl` key.
3. Left-click the unselected components you want to select.

### Select Components Using the Selection Tool

1. Select the [Selection Tool]({{ "docs/gui#main-toolbar" | relative_url}}) (#5) on the Main Toolbar.
2. Draw a box around the components you want to select:
    - Left-click and hold on a specific location in the model to mark a corner of the 'selection box'.
    - Drag the mouse to draw a selection box around the components you want to select.
    - Release the left mouse button.

**Important**: If the Selection Tool is activated (has been selected) on the Main Toolbar, a simple left-click anywhere in the model clears the current selection. However, if you press and hold the `Ctrl` key, the component clicked will be removed from or added to the current selection depending on whether it is currently selected or not. 
{: .notice--warning}
  
### Add Components to the Current Selection

Regardless of which of the above methods you are using to select components, just press and hold the `Ctrl` button while using the given method to add additional components to your selection.

### Remove Components From the Current Selection

Selected components can be removed from the current selection by simply selecting them again with any of the two described methods above.

### Clear the Current Selection

Press the [Clear Selection]({{ "docs/gui#main-toolbar" | relative_url }}) button (#6) on the Main Toolbar.

## Grouping Pipelines into Aggregate Components

1. [Select]({{ "docs/mod_createpipeline#selecting-model-components" | relative_url }}) the components you want to group into an aggregate component. 
2. Open the context menu (i.e. right-click anywhere) and select `Create Sequential Group`.  

**Note**: LUMASS only allows you to group model components that share the same host (parent) component, i.e. that are inside the same aggregate component. Note that the 'root' component is an aggregate component.
{: .notice--info}

## Moving and Copying Model Components

If you want to move or copy more than one model component, [select]({{ "docs/mod_createpipeline#selecting-model-components" | relative_url }}) the components you want to move or copy before following the below steps. 

### Drag and Drop

1. Left-click on (one of) the component(s) you want to copy or move.
2. Press and hold either the `Shift` or `Ctrl` key, depending on whether you want to move or copy the component(s).
3. Drag the comoponent(s) to its (their) target location (aggregate component) in the model.
4. Release the left mouse button and the `Shift` or `Ctrl` key.

### Cut / Copy and Paste

1. Open the context menu (i.e. right-click anywhere).
2. Select either `Cut ...` or `Copy ...` from the menu depending on whether you want to move or copy the component(s).
3. Right-click at the target location (aggregate component) to open the context menu.
4. Select `Paste ...` from the menu.

## Saving Model Component

If you want to save more than one independent model components, [select]({{ "doc/../mod_createpipeline#selecting-model-components"}}) them.

### Saving Model Components to Disk

1. Right click on (one of) the (selected) component(s) to open the context menu.
2. Select `Save ... As ...`.
3. Specify the filename under which to save the (selected) model component(s). 
4. Click `Save` on the FileDialog.

### Adding a Model Component to the User Models List

The [User Models]({{ "docs/gui" | relative_url }}) list (#9) shows models stored in a specific folder configured by the user (Main Menu >> Settings >> Configure Settings >> UserModels). In addition to saving a model component in that folder using the [context menu]({{ "docs/mod_createpipeline#saving-model-components-to-disk" | relative_url }}), users can simply drag and drop a model component onto the User Models list: 

1. Press and hold the `Ctrl` key.
2. Left-click and hold on the component you want to add to the User Models. 
3. Drag the component onto the [User Models]({{ "docs/gui#main-toolbar" | relative_url }}) list (#9).
4. Release the left mouse button and the `Ctrl` key.

**Note**: Since a model is expected to be encapsulated in a single aggregate component, you may only drag a single component onto the User Models list.
{: .notice--info}

## Loading Model Components

### Drag and Drop from the User Models List

1. Scroll to the user model you want to add to the [Model View]({{ "docs/gui" | relative_url }}) (#4) and the [Model Components]({{ "docs/gui" | relative_url }}) (#8).  
2. Drag the name of the User Model into the Model View and drop it onto an empty space of the aggregate component you want to add it to.

### Drag and Drop from the Filesystem Browser

1. Open your filesystem browser and navigate to the LUMASS model file (*.lmx or *.lmv) you want to load.
2. Drag either the *.lmx or *.lmv file into the Model View. 
3. Drop the file onto an empty space within the target aggregate component you want to add the model component to. 

### Context Menu from a Specific Model Component

1. Right-click on the aggregate component you want to add a saved model component to.
2. Select `Load into ...`
3. Use the filesystem browser to navigate to the model file you want to add to the given component.
4. Select either the *.lmx or *.lmv file of the model and confirm by clicking the `Open` button.

## Executing a Model

### Execute the Entire Model

To execute the entire model assembled in the Model View, simply press the [Play]({{ "docs/gui#main-toolbar" | relative_url }}) button (#10) on the Main Toolbar.  

### Execute a Specific Model Component

Users can execute individual aggregate components or individual processing pipelines inside a given aggregate component by executing the aggregate component or the sink component at the bottom or downstream end of the processing pipeline:

1. Right-click on the model component you want to execute to open the context menu.
2. Select `Execute ...` to invoke the component's `Update` function.

    **Note**: In case your processing pipeline ends in a `DataBuffer` or `DataBufferReference` component, you would be selecting `Update ...` in step 2.  
    {: .notice--info}

**Important**: Before you execute a selected model component, make sure that all potentially required data that is produced by upstream components and that are not part of the given aggregate component or processing pipeline, have been produced!
{: .notice--warning}


<!-- 

## Building a Simple Processing Pipeline

1. **Add a process component to the model**: Select the `ImageReader` entry in the [Model Components]({{ "gui" | realtive_url }}) (#8) list and drag it into the [Model View]({{ "gui" | realtive_url }}) (#4).

2. **Add the [`ImageWriter`]({{ "cref_image_writer" | realtive_url }}) component to the Model View** using drag and drop.

    **Note**: If none of the tools in the [main toolbar]({{ "docs/gui#main-toolbar" | relative_url}}) is selected (cf. #1-5 and #7), you can reposition the individual components using the mouse. Clicking with the left mouse button on a component shows its properties in the [Component Properties]({{ "gui" | realtive_url}}) (#11) window. A process component comprises the [general model component properties]({{ "mod_structure#general-model-and-aggregate-component-properties" | realtive_url }}) (at the top) and the specific process component properties at the bottom (below the `ProcessName` property). Refer to the [component reference]({{ "docs/mod_component_reference" | relative_url }}) for a description of a component's properties.  
    {: .notice--info}

3. **Link the process components**:
   - Select the [Link Components]({{ "docs/gui#main-toolbar" | relative_url}}) (#7) tool on the main tool bar. 
   - Left click on the `ImageReader` component and hold the left mouse button down.
   - Move the mouse pointer onto the `ImageWriter` component and release the left mouse button.
   - Deselect the Link Components tool.
   - Click on the `ImageWriter` component and inspect its `Inputs` property. The `ImageReader` Component is now listed as an input component to the `ImageWriter`. 

    **Note**: Always link components starting at the source (output) and ending at the target (input). 
    {: .notice--info} 

4. **Define/double check the components’ properties**: For the error-free execution of  the processing pipeline, it is important to double check the components’ properties. 

    **Important**: An important concept of the modelling framework is that it requires an image’s data type, dimension, and number of bands (image characteristics) to be explicitly specified. Furthermore, the image characteristics of an output (image) have to match the image characteristics of an associated input (image). If these characteristics do not match, the pipeline will not execute properly and produce an error. 
    {: .notice--warning}

    - **Define the input file name**: Use your filesystem browser to navigate to SampleData/data folder. Select the image file `LUMASS_icon_2048.kea` and drag it onto the `ImageReader` Component. This adds the absolute file name to the `FileNames` property of the `ImageReader` component. 
    - **Define the `ImageReader`’s image properties**: Apply the following settings to the component’s corresponding input and output properties: 
      - `PixelType: uchar`
      - `NumBands: 4`
    
    - **Define the output filename**: Click on the `ImageWriter` to display its properties in the [Component Properties]({{ "gui" | realtive_url}}) (#11) window. Click on its `FileNames` property to open the parameter editor (Fig. 9). Point into the left hand area of the dialog below the text #Iteration. From the context menu (right mouse click) select Insert parameter here. This inserts an editable file name parameter into the parameter list. To facilitate editing the output file name, drag the input file, as defined for the ImageReader, into the Edit parameter window. This inserts the file’s absolute file name into the editor window. Delete the leading sequence of characters (‘file:///’) and change the image file name to output_exercise1.img. Click the Apply button to transfer the parameter to the component’s FileNames property. 
Note: LUMASS uses a forward slash ‘/’ as path separator independent of the operating system. Furthermore, the ImageWriter component recognises the output image format from the defined file name suffix. Hence, this pipeline can be used to convert images between different formats as long as they are supported by the underlying GDAL library. 
    d. Define the ImageWriter’s image properties: Apply the following settings to the component’s corresponding input and output properties: 
ComponentType: uchar
NumBands: 4
    1. Save the pipeline as LUMASS model: Select the ImageReader and ImageWriter components by holding the CTRL key and a left mouse click on each component. Now point the mouse on one of the components and right click with the mouse to open the context menu. Select Save 2 Components As … Select a file name and folder for the model file and save the model. 
Note: LUMASS models are comprised of two different files differentiated by their file name suffix. The *.lmx file saves the actual model as XML representation, whereas the *.lmv file stores binary version of the the visual representation displayed in the Model View. While saving or reading a LUMASS model file, only one file needs to be explicitly specified. The corresponding other file is read/saved automatically by LUMASS. To execute a LUMASS model with the lumassengine commandline application, only the *.lmx version of the file is required. However, to edit the file in the LUMASS user interface, both files need to be available.
    6. Display the Notifications window: Select the Notifications option from the View menu to open the Notifications window. It displays information, warnings, and error messages occurring during a model run. 
    7. Execute the model: To execute the model either click on the Execute Model button on the tool bar (Fig. 2-10) or open the context menu of the ImageWriter component and select Execute ImageWriter. If the notifications window does not show any error messages, you should find a newly created image file at its specified output location.




### Notes 
- Some processing components that accept multiple input components, rely on the order of inputs to distinguish semantically different input images (cf. [`SummarizeZones`]({{ "/docs/cref_sum_zones" | relative_url }})). 
- The order of input images can be changed using the Component Property Editor dialog by interactively moving the list entries in the left hand pane (`shift` + `left mouse button` down: move; `ctrl` + `left mouse button` down: copy).
- To specify the nth output image of a processing component as input image to a downstream component, append the `0`-based index of the desired image to the input component name, e.g. {% raw %}`{{{ImageReader}{MapAlgebra24:1}}}`{% endraw %}.
In this case, the processing compoments has two specified inputs, `ImageReader12` and the second output image of `MapAlgebra24`, as indicated by index `1` for the second output.

-->