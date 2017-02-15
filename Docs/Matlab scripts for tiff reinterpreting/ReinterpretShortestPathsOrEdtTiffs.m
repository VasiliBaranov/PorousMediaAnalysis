clear all;

% A script to reinterpret CMYK tiff files as matrices with either uint32 or float values
% NOTE: Before trying this code out, run the program in "../Example" with parameters "-all" and "-spd"!!!
% OR: copy the results from "../Example/Expected results from -all" and "../../Example/Expected results from -spd" to "../Example".

folder = '../Example';
shouldAnalyzeEuclideanDistanceSquares = true; % EuclideanDistanceSquares
% shouldAnalyzeEuclideanDistanceSquares = false; % ShortestPathDistances


if (shouldAnalyzeEuclideanDistanceSquares)
    %%%%%%%%%%%
    % Parameters for EuclideanDistanceSquares
    imagesFolder = fullfile(folder, 'EuclideanDistanceSquares'); targetType = 'uint32';
    unknownShortestPathValue = intmax('uint32'); unknownValueToDisplay = Inf; % this value is never encountered for EDT
    solidShortestPathValue = 0; solidValueToDisplay = NaN; % white colour in pcolor
else
    %%%%%%%%%%%
    % Parameters for ShortestPathDistances
    imagesFolder = fullfile(folder, 'ShortestPathDistances'); targetType = 'single';
    unknownShortestPathValue = realmax('single'); unknownValueToDisplay = NaN; % white colour
    solidShortestPathValue = -1; solidValueToDisplay = 0; % the darkest color
end

imageIndex = 1;

imagesList = dir(fullfile(imagesFolder, '*.tif'));
imagePath = fullfile(imagesFolder, imagesList(imageIndex).name);

%%%%%%%%%%%
% Reading the image
shortestPathsOrEdtSquares = CmykTiffUtility.ReadCmykImage(imagePath, targetType);

%%%%%%%%%%%
% Changing some solid and unknown values for better visualization
shortestPathsOrEdtSquares(shortestPathsOrEdtSquares == solidShortestPathValue) = solidValueToDisplay;
shortestPathsOrEdtSquares(shortestPathsOrEdtSquares == unknownShortestPathValue) = unknownValueToDisplay;

%%%%%%%%%%%
% Displaying the image

% For some reason 'pcolor' flips the image vertically. 'image' doesn't do it
shortestPathsOrEdtSquares = flipud(shortestPathsOrEdtSquares);

figure;
currentColormap = jet(256);
colormap(currentColormap);
h = pcolor(shortestPathsOrEdtSquares);
set(gca, 'DataAspectRatio', [1 1 1]);
set(h, 'EdgeAlpha', 0); % this is essential, it removes lines between cells 
colorbar;

% Saving the image as pcolor, but without the colorbar and changes in resolution
outputFilePath = 'output.tiff';
CmykTiffUtility.SaveAsPcolor(shortestPathsOrEdtSquares, currentColormap, outputFilePath);


