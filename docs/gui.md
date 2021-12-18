---
title: "Graphical User Interface (GUI)"
permalink: "/docs/gui"
--- 

[![Graphical_User_Interface]({{ "/assets/images/docs/gui_v0966_enumerated.png" | relative_url }})]({{ "/assets/images/docs/gui_v0966_enumerated.png" | relative_url }})<br>

1. **Main Tool Bar** (s. below)
2. **User Tools**: Tools created from LUMASS models
3. **Model Config & Search**: Model file load and save, model configuration, and component search
4. **Model View**: Visual modelling environment
5. **Map View**: 2D/3D Display area for raster and vector layers as well as for point clouds
6. **Map Layers**: Table of contents and legend configuration for layers displayed in the map view 
7. **Table Objects**: Table of contents for stand-alone tables (s. Table view)
8. **Model Components**: List of available model components in the modelling environment
9. **User Models**: List of models stored in the `UserModels` folder (Settings >> Configure Settings ...) 
10. **Notifications**: Info, warn, and error messages related to processing and modelling tasks
11. **Component Properties**: Properties of a given model component
12. **Layer Attributes**: List of map layer attribute values for a given point in the map (view) 
13. **Table View**: Table display and processing interface

LUMASS provides a typical desktop user-interface that embraces the use of drag and drop and context menus. If you want to accomplish a certain task and are in doubt of how to do it, try drag and drop, for example to import a table, image, or model into the respective view areas. If you want to perform an action on a particular object, e.g. map layer, table column, or model component, double check whether it provides a context menu (right click) offering object specific actions, such as table operations or executing model components.
The user interface can be adjusted to best suit the current task at hand (i.e. mapping, or modelling). For example, the main views (GUI: 1,2) can be individually hidden and displayed (Tool bar: 12,13), adjusted in their size (by using their separating slider), or stacked vertically or horizontally (Tool bar: 14, 15). The display areas left and right of the main views (Layers & Components and Attributes and Properties, GUI: 5-9), as well as the notification area (GUI: 10) are dockable windows and can be arbitrarily positioned around the centred main views or float on top of the main user interface. Layer attribute tables and stand-alone tables (GUI: 13) are displayed in their own top level window independent of the main user interface.
The individual content and property display areas (GUI: 5-9) can be collapsed and unfolded individually by clicking on their respective title button showing their name (e.g. Table Objects, GUI: 7). The View menu provides the presets 'Map View Mode' and 'Model View Mode', which configure the associated display areas for mapping and modelling tasks respectively. 

## Main Tool Bar

[![Tool_bar]({{ "/assets/images/docs/main_tool_bar_enumerated.png" | relative_url }})]({{ "/assets/images/docs/main_tool_bar_enumerated.png" | relative_url }})<br>

1. Zoom in map/model view
2. Zoom out map/model view
3. Zoom to map/model content
4. Pan map/model
5. Select model components
6. Clear selection (features/components)
7. Link model components
8. Reset model
9. Stop model execution
10. Execute model
11. Follow model execution (auto focus on executing component)
12. Display/hide map view
13. Display/hide model view
14. Stack main views horizontally
15. Stack main views vertically

## Model Config & Search

[![ModelConfigSearch]({{ "/assets/images/docs/model_config_search_enumerated.png" | relative_url }})]({{ "/assets/images/docs/model_config_search_enumerated.png" | relative_url }})<br>

1. Save active model (current filename)
2. Save model as ...
3. Active model filename
4. Configuration context selector (None, Model's YAML, \<UserTool\>, YAML File)
5. Model configuration file path (*.yaml, *.ldb)
6. Model component/parameter search

## Saving and Loading Models

Models can be saved under the current filename (Model Config: 1,3) or under a new filename (Model Config: 2). LUMASS models are stored in two separate files. The  structure of the model, including all configured parameters, is stored in a XML-based text file (\*.lmx). The visual representation of a model as seen in the modelling environment is stored in a binary file (\*.lmv). Both files are always saved automatically, regardless of whether the 'lmv' or 'lmx' file ending is used when saving a model. Equally, when loading a model into the modelling environment, either by using the context menu ('Load Into ...') or by dragging a model from a file explorer into the environment, using either the 'lmv' or 'lmx' file works. Any model components (GUI: 8), `UserModels` (GUI: 9), or externally saved model files (*.lmx, *.lmv) dragged or loaded (context menu) into the modelling environment are added (copied) into the currently edited model. 
