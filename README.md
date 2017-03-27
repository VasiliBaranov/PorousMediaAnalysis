# Welcome to the PorousMediaAnalysis project

The program allows morphological processing of three-dimensional binary images, comprised of solid (black) and void (non-black) pixels. It can 
* detect pores (large connected parts of void pixels) and throats between these pores. This procedure is based on, but not equivalent to, the work of [Dong and Blunt, 2009](http://journals.aps.org/pre/abstract/10.1103/PhysRevE.80.036307)
* compute shortest paths (geodesic distance) between the center void pixel and all the other void pixels in the sample

It is a console program, which doesn't require any preinstalled libraries, is multiplatform (Windows/nix) and supports in some steps OpenMP.

It was developed by me (Vasili Baranau) while doing research in the group of
Prof. Dr. Ulrich Tallarek in Marburg, Germany, in 2014. It is distributed under the [MIT license](https://github.com/VasiliBaranov/PorousMediaAnalysis/blob/master/LICENSE). If you use this program in a scientific project, **please cite this paper**: *K. Hormann, V. Baranau, D. Hlushkou, A. Höltzel, and U. Tallarek; Topological analysis of non-granular, disordered porous media: determination of pore connectivity, pore coordination, and geometric tortuosity in physically reconstructed silica monoliths; New Journal of Chemistry, 40, 4187–4199 (2015)*; [doi:10.1039/C5NJ02814K](http://pubs.rsc.org/en/content/articlelanding/2016/nj/c5nj02814k).

Overview of operation
=======
![Overview of operation](https://github.com/VasiliBaranov/PorousMediaAnalysis/blob/master/Docs/Wiki/Images/FrontPageImage.png)

The image above represents detecting pores in the solid-void structure. From left to right: original geometry (white—solid); Euclidean Distance Transform; "containing ball radii" (in the sense of [local thickness](https://imagej.net/Local_Thickness)); separation of the void space into pores (pore boundaries are black). Similar to Fig. 3 in [Hormann et al., 2015](http://pubs.rsc.org/en/content/articlelanding/2016/nj/c5nj02814k). For details, see  [Short algorithm description](https://github.com/VasiliBaranov/PorousMediaAnalysis#short-algorithm-description) below (the image depicts stages 1, 2, and 5) or the [full pore-throat detection description](https://github.com/VasiliBaranov/PorousMediaAnalysis/wiki/Pore-throat-analysis-description).

Main links in the project
=======
1. This page, below : a short program description and links to important wiki pages
2. [Latest release](https://github.com/VasiliBaranov/PorousMediaAnalysis/releases/latest)
3. [Wiki](https://github.com/VasiliBaranov/PorousMediaAnalysis/wiki)
4. [Example](https://github.com/VasiliBaranov/PorousMediaAnalysis/tree/master/Docs/Example)
5. [Matlab scripts for reading the results](https://github.com/VasiliBaranov/PorousMediaAnalysis/tree/master/Docs/Matlab%20scripts%20for%20tiff%20reinterpreting)

___
Below is a very short program description on a conceptual level and some links to important wiki pages. For a very detailed algorithm and program API description, please see the project [wiki](https://github.com/VasiliBaranov/PorousMediaAnalysis/wiki). Please read the wiki page [Compilation](https://github.com/VasiliBaranov/PorousMediaAnalysis/wiki/Compilation) prior to building the project, because compilation on nix systems may require some precaution due to a [bug](https://bugs.eclipse.org/bugs/show_bug.cgi?id=340300) in Eclipse.

Table of contents:

1. [Short algorithm description](https://github.com/VasiliBaranov/PorousMediaAnalysis#short-algorithm-description)
  1. [Pore-throat detection](https://github.com/VasiliBaranov/PorousMediaAnalysis#pore-throat-detection)
  2. [Shortest paths computation](https://github.com/VasiliBaranov/PorousMediaAnalysis#shortest-paths-computation)
2. [Program execution](https://github.com/VasiliBaranov/PorousMediaAnalysis#program-execution)
3. [Other](https://github.com/VasiliBaranov/PorousMediaAnalysis#other)

Short algorithm description
=======

Pore-throat detection
-----------

To detect pores and throats, we do the following steps (inspired by the work of [Dong and Blunt, 2009](http://journals.aps.org/pre/abstract/10.1103/PhysRevE.80.036307)):

1.	Do Euclidean distance transform (EDT)
2.	Find containing spheres for each pixel
3.	Find starting pore voxels (pore seeds)
4.	Do pore propagation through containing ball radii starting at the pore seeds
5.	Do watershed propagation through containing ball radii, using the pore seeds as water sources

The image above depicts stages 1, 2, and 5. This procedure is described in slightly more detail just below. For a very detailed description, please read [this wiki page](https://github.com/VasiliBaranov/PorousMediaAnalysis/wiki/Pore-throat-analysis-description).

### 1. Euclidean distance transform
We do a standard Euclidean distance transform. In other words, we determine for each voxel a sphere that can be inscribed around this voxel and just touches the solid boundary (the maximum inscribed sphere).

### 2. Finding containing sphere radii
Each void voxel is thus covered by many maximum inscribed spheres. For each void voxel, we determine the radius of the largest sphere that coveres the voxel (the maximum containing sphere).

### 3. Finding pore seeds
After step 2, we can detect areas of local maxima in the map of maximum containing radii. I.e., some maximum containing radii form plateaus surrounded by voxels with lower maximum containing radii. Such plateaus can be formed either by one maximum containing sphere or by several maximum containing spheres with equal radii. We identify each plateau with a separate pore. For each plateau, we arbitrary select a center of one of the forming spheres as a pore center (pore seed).

### 4. Initial pore propagation
For each pore seed, we conduct pore propagation. Pore propagation is an iterative process that expands pore boundary at each iteration. At each iteration, we check all 26 neighbors of each pore boundary pixel. If a neighbor (i) is void, (ii) does not belong to the current pore yet, and (iii) its maximum inscribed radius is not larger than the maximum inscribed radius of a current boundary pixel, we mark the neighbor as belonging to the current pore and include it in the list of boundary pixels for the next iteration. Pixels that form the boundary at a current iteration are not included in the boundary for the next iteration. We stop iterations if there are no pixels in the boundary for the next step. After all pores finish propagating, each void pixel can belong to several pores. The areas of shared pixels may be quite large. Please note that the actual program execution determines pore seeds during the initial pore propagation (as described [here](https://github.com/VasiliBaranov/PorousMediaAnalysis/wiki/Pore-throat-analysis-description#14-initial-pore-propagation)). A separate step 3 (finding pore seeds) is added here for simplicity.

### 5. Watershed segmentation
Finally, watershed segmentation is used to define smaller pore boundaries. We use maximum containing radii as the depth of the “relief” where watershed happens. For the segmentation, we need to maintain “water level” and a set of boundary pixels for each pore. Initially, pore boundaries are formed only by pore seeds. Water level is initially set at the largest maximum containing radius among pore seeds (the deepest point in the relief) and then increased gradually. For each water level, we conduct pore propagation, which is an iterative process that expands boundaries of each pore. At each iteration, we process for each pore those boundary pixels that are at a current water level. For each boundary pixel, we check its 26 neighbors. If a neighbor is (i) void and (ii) its maximum containing radius is not larger than the maximum containing radius of a current boundary pixel, the boundary can propagate to it. If the neighbor does not belong to any pore yet, we mark the neighbor as belonging to the current pore and add it to the pore boundary (to be processed on the next iteration). If the neighbor is already marked as belonging to one or more pores, we specify that the neighbor belongs to the current pore as well, do not add it to the pore boundary, and remove it from the boundaries of other pores which he belongs to (whether it has to be processed on the current or future iterations). When there are no more boundary pixels on a current water level, the water level is increased by one and iterations start again. When there are no more boundary pixels to be processed at all, the segmentation stops. At this point all void voxels shall belong to one or more pores.

For a very detailed description of the steps above, please read [this wiki page](https://github.com/VasiliBaranov/PorousMediaAnalysis/wiki/Pore-throat-analysis-description).

Shortest paths computation
-----------

The program can calculate shortest paths between a single starting void voxel and all other void voxel of a reconstruction. As a starting voxel, we use a void voxel closest to the geometric center of the sample. To calculate shortest paths, we considered each void voxel as a vertex in a graph. We consider each voxel connected only with its 26 nearest neighbors, excluding solid voxels. Euclidean distance between voxels is used as a weight of edges in this graph. It can thus have only values of 1, √2, and √3. Thus, we calculate shortest paths between the starting voxel and other void voxels inside this graph. We employ Dijkstra’s shortest paths algorithm.

For a very detailed description of this step, please read [this wiki page](https://github.com/VasiliBaranov/PorousMediaAnalysis/wiki/Shortest-path-computation-description).


Program execution
=======

Pore-throat detection
-----------

For a detailed description of console parameters, file formats, memory requirements, etc., please read [this wiki page](https://github.com/VasiliBaranov/PorousMediaAnalysis/wiki/Pore-throat-analysis-execution).


Shortest paths computation
-----------

For a detailed description of console parameters, file formats, memory requirements, etc., please read [this wiki page](https://github.com/VasiliBaranov/PorousMediaAnalysis/wiki/Shortest-path-computation-execution).

Other
=======

[Architecture](https://github.com/VasiliBaranov/PorousMediaAnalysis/wiki/Architecture) describes a high-level architecture of the project.

[Compilation](https://github.com/VasiliBaranov/PorousMediaAnalysis/wiki/Compilation) describes how to edit and build the PorousMediaAnalysis project. **Please read** it because compilation on nix systems may require some precaution due to a [bug](https://bugs.eclipse.org/bugs/show_bug.cgi?id=340300) in Eclipse.
