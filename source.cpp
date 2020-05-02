#include <iostream>
#include <vector>
#include <fstream>
#include "Header.h"

#define PATH "C:\\Users\\Ростислав\\Downloads\\UCgV01xupB4.bmp"

int main(int argc, char* argv[]){
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " file_name" << std::endl;
        return 0;
    }

    char* fileName = argv[1];

    // открываем файл
    std::ifstream fileStream(fileName, std::ifstream::binary);
    if (!fileStream) {
        std::cout << "Error opening file '" << fileName << "'." << std::endl;
        return 0;
    }

    // заголовк изображения
    BITMAPFILEHEADER fileHeader;
    read(fileStream, fileHeader.bfType, sizeof(fileHeader.bfType));
    read(fileStream, fileHeader.bfSize, sizeof(fileHeader.bfSize));
    read(fileStream, fileHeader.bfReserved1, sizeof(fileHeader.bfReserved1));
    read(fileStream, fileHeader.bfReserved2, sizeof(fileHeader.bfReserved2));
    read(fileStream, fileHeader.bfOffBits, sizeof(fileHeader.bfOffBits));

    if (fileHeader.bfType != 0x4D42) {
        std::cout << "Error: '" << fileName << "' is not BMP file." << std::endl;
        return 0;
    }

    // информация изображения
    BITMAPINFOHEADER fileInfoHeader;
    read(fileStream, fileInfoHeader.biSize, sizeof(fileInfoHeader.biSize));

    // bmp core
    if (fileInfoHeader.biSize >= 12) {
        read(fileStream, fileInfoHeader.biWidth, sizeof(fileInfoHeader.biWidth));
        read(fileStream, fileInfoHeader.biHeight, sizeof(fileInfoHeader.biHeight));
        read(fileStream, fileInfoHeader.biPlanes, sizeof(fileInfoHeader.biPlanes));
        read(fileStream, fileInfoHeader.biBitCount, sizeof(fileInfoHeader.biBitCount));
    }

    // получаем информацию о битности
    int colorsCount = fileInfoHeader.biBitCount >> 3;
    if (colorsCount < 3) {
        colorsCount = 3;
    }

    int bitsOnColor = fileInfoHeader.biBitCount / colorsCount;
    int maskValue = (1 << bitsOnColor) - 1;

    // bmp v1
    if (fileInfoHeader.biSize >= 40) {
        read(fileStream, fileInfoHeader.biCompression, sizeof(fileInfoHeader.biCompression));
        read(fileStream, fileInfoHeader.biSizeImage, sizeof(fileInfoHeader.biSizeImage));
        read(fileStream, fileInfoHeader.biXPelsPerMeter, sizeof(fileInfoHeader.biXPelsPerMeter));
        read(fileStream, fileInfoHeader.biYPelsPerMeter, sizeof(fileInfoHeader.biYPelsPerMeter));
        read(fileStream, fileInfoHeader.biClrUsed, sizeof(fileInfoHeader.biClrUsed));
        read(fileStream, fileInfoHeader.biClrImportant, sizeof(fileInfoHeader.biClrImportant));
    }

    // bmp v2
    fileInfoHeader.biRedMask = 0;
    fileInfoHeader.biGreenMask = 0;
    fileInfoHeader.biBlueMask = 0;

    if (fileInfoHeader.biSize >= 52) {
        read(fileStream, fileInfoHeader.biRedMask, sizeof(fileInfoHeader.biRedMask));
        read(fileStream, fileInfoHeader.biGreenMask, sizeof(fileInfoHeader.biGreenMask));
        read(fileStream, fileInfoHeader.biBlueMask, sizeof(fileInfoHeader.biBlueMask));
    }

    // если маска не задана, то ставим маску по умолчанию
    if (fileInfoHeader.biRedMask == 0 || fileInfoHeader.biGreenMask == 0 || fileInfoHeader.biBlueMask == 0) {
        fileInfoHeader.biRedMask = maskValue << (bitsOnColor * 2);
        fileInfoHeader.biGreenMask = maskValue << bitsOnColor;
        fileInfoHeader.biBlueMask = maskValue;
    }

    // bmp v3
    if (fileInfoHeader.biSize >= 56) {
        read(fileStream, fileInfoHeader.biAlphaMask, sizeof(fileInfoHeader.biAlphaMask));
    }
    else {
        fileInfoHeader.biAlphaMask = maskValue << (bitsOnColor * 3);
    }

    // bmp v4
    if (fileInfoHeader.biSize >= 108) {
        read(fileStream, fileInfoHeader.biCSType, sizeof(fileInfoHeader.biCSType));
        read(fileStream, fileInfoHeader.biEndpoints, sizeof(fileInfoHeader.biEndpoints));
        read(fileStream, fileInfoHeader.biGammaRed, sizeof(fileInfoHeader.biGammaRed));
        read(fileStream, fileInfoHeader.biGammaGreen, sizeof(fileInfoHeader.biGammaGreen));
        read(fileStream, fileInfoHeader.biGammaBlue, sizeof(fileInfoHeader.biGammaBlue));
    }

    // bmp v5
    if (fileInfoHeader.biSize >= 124) {
        read(fileStream, fileInfoHeader.biIntent, sizeof(fileInfoHeader.biIntent));
        read(fileStream, fileInfoHeader.biProfileData, sizeof(fileInfoHeader.biProfileData));
        read(fileStream, fileInfoHeader.biProfileSize, sizeof(fileInfoHeader.biProfileSize));
        read(fileStream, fileInfoHeader.biReserved, sizeof(fileInfoHeader.biReserved));
    }

    // проверка на поддерку этой версии формата
    if (fileInfoHeader.biSize != 12 && fileInfoHeader.biSize != 40 && fileInfoHeader.biSize != 52 &&
        fileInfoHeader.biSize != 56 && fileInfoHeader.biSize != 108 && fileInfoHeader.biSize != 124) {
        std::cout << "Error: Unsupported BMP format." << std::endl;
        return 0;
    }

    if (fileInfoHeader.biBitCount != 16 && fileInfoHeader.biBitCount != 24 && fileInfoHeader.biBitCount != 32) {
        std::cout << "Error: Unsupported BMP bit count." << std::endl;
        return 0;
    }

    if (fileInfoHeader.biCompression != 0 && fileInfoHeader.biCompression != 3) {
        std::cout << "Error: Unsupported BMP compression." << std::endl;
        return 0;
    }

    // rgb info
    RGBQUAD** rgbInfo = new RGBQUAD * [fileInfoHeader.biHeight];

    for (unsigned int i = 0; i < fileInfoHeader.biHeight; i++) {
        rgbInfo[i] = new RGBQUAD[fileInfoHeader.biWidth];
    }

    // определение размера отступа в конце каждой строки
    int linePadding = ((fileInfoHeader.biWidth * (fileInfoHeader.biBitCount / 8)) % 4) & 3;

    // чтение
    unsigned int bufer;

    for (unsigned int i = 0; i < fileInfoHeader.biHeight; i++) {
        for (unsigned int j = 0; j < fileInfoHeader.biWidth; j++) {
            read(fileStream, bufer, fileInfoHeader.biBitCount / 8);

            rgbInfo[i][j].rgbRed = bitextract(bufer, fileInfoHeader.biRedMask);
            rgbInfo[i][j].rgbGreen = bitextract(bufer, fileInfoHeader.biGreenMask);
            rgbInfo[i][j].rgbBlue = bitextract(bufer, fileInfoHeader.biBlueMask);
            rgbInfo[i][j].rgbReserved = bitextract(bufer, fileInfoHeader.biAlphaMask);
        }
        fileStream.seekg(linePadding, std::ios_base::cur);
    }

    // вывод
    /*
    for (unsigned int i = 0; i < fileInfoHeader.biHeight; i++) {
        for (unsigned int j = 0; j < fileInfoHeader.biWidth; j++) {
            std::cout << std::dec
                << +rgbInfo[i][j].rgbRed << " "
                << +rgbInfo[i][j].rgbGreen << " "
                << +rgbInfo[i][j].rgbBlue << " "
                << (rgbInfo[i][j].rgbRed + rgbInfo[i][j].rgbGreen + rgbInfo[i][j].rgbBlue) / 3 << std::endl;
        }
        std::cout << std::endl;
    }
    */
    

    for (auto y = fileInfoHeader.biWidth - 1; y > 0; y--) {
        for (unsigned int x = 0; x < fileInfoHeader.biHeight; x++) {
            //std::cout << std::dec
            //    << (rgbInfo[x][y].rgbRed + rgbInfo[x][y].rgbGreen + rgbInfo[x][y].rgbBlue) / 3 << std::endl;
            unsigned int average = (rgbInfo[y][x].rgbRed + rgbInfo[y][x].rgbGreen + rgbInfo[y][x].rgbBlue) / 3;
            
            if (0 < average and average <= 10) std::cout            << "@";
            else if (10 < average and average <= 20) std::cout      << "@";
            else if (20 < average and average <= 30) std::cout      << "@";
            else if (30 < average and average <= 40) std::cout      << "@";
            else if (40 < average and average <= 50) std::cout      << "@";
            else if (50 < average and average <= 60) std::cout      << "@";
            else if (60 < average and average <= 70) std::cout      << "@";
            else if (70 < average and average <= 80) std::cout      << "@";
            else if (80 < average and average <= 90) std::cout      << "@";
            else if (90 < average and average <= 100) std::cout     << "@";
            else if (100 < average and average <= 105) std::cout    << "@";
            else if (105 < average and average <= 110) std::cout    << "$";
            else if (110 < average and average <= 120) std::cout    << "S";
            else if (120 < average and average <= 130) std::cout    << "x";
            else if (130 < average and average <= 140) std::cout    << "+";
            else if (140 < average and average <= 150) std::cout    << ";";
            else if (150 < average and average <= 160) std::cout    << "-";
            else if (160 < average and average <= 170) std::cout    << "-";
            else if (170 < average and average <= 180) std::cout    << "-";
            else if (180 < average and average <= 190) std::cout    << ",";
            else if (190 < average and average <= 200) std::cout    << ".";
            else if (200 < average and average <= 210) std::cout    << ".";
            else if (210 < average and average <= 220) std::cout    << " ";
            else if (220 < average and average <= 230) std::cout    << " ";
            else if (230 < average and average <= 240) std::cout    << " ";
            else if (240 < average and average <= 250) std::cout    << " ";
            else if (250 < average and average <= 255) std::cout    << " ";



            else std::cout << "?";

            


            std::cout << " ";
        }
        std::cout << std::endl;
    }

    return 1;
}

unsigned char bitextract(const unsigned int byte, const unsigned int mask) {
    if (mask == 0) {
        return 0;
    }

    // определение количества нулевых бит справа от маски
    int
        maskBufer = mask,
        maskPadding = 0;

    while (!(maskBufer & 1)) {
        maskBufer >>= 1;
        maskPadding++;
    }

    // применение маски и смещение
    return (byte & mask) >> maskPadding;
}
