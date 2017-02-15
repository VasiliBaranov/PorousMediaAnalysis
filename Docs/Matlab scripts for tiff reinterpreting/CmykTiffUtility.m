classdef CmykTiffUtility < handle
    methods (Static)
        % A function to reinterpret a CMYK tiff file as a matrix with either uint32 or float values.
        % pixelType shall be either 'uint32' or 'single'. TODO: add an assert or use a logical parameter.
        function outputMatrix = ReadCmykImage(cmykTiffImagePath, pixelType)
            cmykImage = imread(cmykTiffImagePath);

            outputMatrixInt = zeros(size(cmykImage, 1), size(cmykImage, 2), 'uint32');
            outputMatrixIntTemp = zeros(size(cmykImage, 1), size(cmykImage, 2), 'uint32');
            for i = 1 : 4
                outputMatrixIntTemp = uint32(cmykImage(:, :, i));
                outputMatrixIntTemp = bitshift(outputMatrixIntTemp, 8 * (i - 1));
                outputMatrixInt = bitor(outputMatrixInt, outputMatrixIntTemp);
            end

            outputMatrix = typecast(outputMatrixInt(:), pixelType);
            outputMatrix = reshape(outputMatrix, size(outputMatrixInt));
            outputMatrix = double(outputMatrix);
        end
        
        % Save the image as pcolor, but without the colorbar and changes in resolution
        % The arguments are the same as for the function imwrite. format is optional
        % But pcolor displays the NaN value as white, this function--as the darkest value. TODO: update
        function [] = SaveAsPcolor(X, map, filePath, format)
            colormapLength = size(map, 1);
            maxValue = max(max(X));
            minValue = min(min(X));
            interpolatedValuesColors = (X - minValue) / (maxValue - minValue) * colormapLength + 1;
            if (nargin == 3)
                imwrite(interpolatedValuesColors, map, filePath);
            else
                imwrite(interpolatedValuesColors, map, filePath, format);
            end
        end
    end
end
