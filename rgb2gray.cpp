#include <iostream>
#include <fstream>
#include <chrono>

using namespace std;
using namespace std::chrono;

#pragma pack(push, 1)
struct BMPFileHeader {
    char id[2];         // "BM"
    int fileSize;
    int reserved;
    int dataOffset;
    int headerSize;
    int width;
    int height;
    short planes;
    short bitsPerPixel;
    int compression;
    int imageDataSize;
    int horizontalRes;
    int verticalRes;
    int totalColors;
    int importantColors;
};
#pragma pack(pop)

class Grayscaler {
public:
    Grayscaler() : colorPixels(nullptr), grayPixels(nullptr) {}
    
    ~Grayscaler() {
        delete[] colorPixels;
        delete[] grayPixels;
    }
    
    // Load BMP image from file
    bool loadImage(const char* filename) {
        ifstream inFile(filename, ios::binary);
        if (!inFile) {
            cerr << "Error: Unable to open file " << filename << endl;
            return false;
        }
        
        // Read header
        inFile.read(reinterpret_cast<char*>(&header), sizeof(BMPFileHeader));
        if (header.id[0] != 'B' || header.id[1] != 'M') {
            cerr << "Error: Not a valid BMP file." << endl;
            return false;
        }
        
        int totalPixels = header.width * header.height;
        int dataSize = totalPixels * 3;  // 3 bytes per pixel (BGR)
        
        colorPixels = new unsigned char[dataSize];
        grayPixels  = new unsigned char[totalPixels];
        
        // Jump to pixel data and read image data (BMP data is stored bottom-up)
        inFile.seekg(header.dataOffset, ios::beg);
        inFile.read(reinterpret_cast<char*>(colorPixels), dataSize);
        inFile.close();
        
        return true;
    }
    
    // Convert loaded image data to grayscale
    void convertImage() {
        int totalPixels = header.width * header.height;
        for (int i = 0; i < totalPixels; i++) {
            // BMP pixel data is stored in BGR order
            unsigned char blue  = colorPixels[i * 3];
            unsigned char green = colorPixels[i * 3 + 1];
            unsigned char red   = colorPixels[i * 3 + 2];
            
            grayPixels[i] = static_cast<unsigned char>(0.299 * red + 0.587 * green + 0.114 * blue);
        }
    }
    
    // Save the grayscale image as a BMP file
    void saveImage(const char* filename) {
        // Update header values for a 24-bit grayscale image (R=G=B)
        header.bitsPerPixel = 24;
        int totalPixels = header.width * header.height;
        header.imageDataSize = totalPixels * 3;
        header.fileSize = header.dataOffset + header.imageDataSize;
        
        ofstream outFile(filename, ios::binary);
        if (!outFile) {
            cerr << "Error: Unable to save file " << filename << endl;
            return;
        }
        
        outFile.write(reinterpret_cast<char*>(&header), sizeof(BMPFileHeader));
        for (int i = 0; i < totalPixels; i++) {
            unsigned char gray = grayPixels[i];
            outFile.put(gray); // Red
            outFile.put(gray); // Green
            outFile.put(gray); // Blue
        }
        outFile.close();
    }
    
private:
    BMPFileHeader header;
    unsigned char* colorPixels;
    unsigned char* grayPixels;
};

int main() {
    Grayscaler converter;
    if (!converter.loadImage("nasa_image.bmp"))
        return 1;
    
    auto startTime = high_resolution_clock::now();
    converter.convertImage();
    auto endTime = high_resolution_clock::now();
    
    cout << "Grayscale conversion time: "
         << duration_cast<milliseconds>(endTime - startTime).count() << " ms" << endl;
    
    converter.saveImage("gray_output.bmp");
    return 0;
}
