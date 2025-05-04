#include <iostream>
#include <fstream>
#include <chrono>
#include <openacc.h>  // OpenACC header

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

        // Read the header
        inFile.read(reinterpret_cast<char*>(&header), sizeof(BMPFileHeader));
        if (header.id[0] != 'B' || header.id[1] != 'M') {
            cerr << "Error: File is not a valid BMP." << endl;
            return false;
        }

        int totalPixels = header.width * header.height;
        int dataSize = totalPixels * 3;  // 3 bytes per pixel

        colorPixels = new unsigned char[dataSize];
        grayPixels  = new unsigned char[totalPixels];

        // Move to the pixel data offset and read the image
        inFile.seekg(header.dataOffset, ios::beg);
        inFile.read(reinterpret_cast<char*>(colorPixels), dataSize);
        inFile.close();
        return true;
    }

    // Convert the loaded image to grayscale using OpenACC for acceleration
    void convertImage() {
        int totalPixels = header.width * header.height;
        int dataSize = totalPixels * 3;

        // Copy member pointers to local variables to avoid capturing 'this' on device.
        unsigned char* localColor = colorPixels;
        unsigned char* localGray  = grayPixels;

        // Transfer the image data to the device, run the conversion in parallel, and copy results back
        #pragma acc data copyin(localColor[0:dataSize]) copyout(localGray[0:totalPixels])
        {
            #pragma acc parallel loop
            for (int i = 0; i < totalPixels; i++) {
                // BMP stores pixel data in BGR order
                unsigned char blue  = localColor[i * 3];
                unsigned char green = localColor[i * 3 + 1];
                unsigned char red   = localColor[i * 3 + 2];

                localGray[i] = static_cast<unsigned char>(0.299 * red + 0.587 * green + 0.114 * blue);
            }
        }
    }

    // Save the grayscale image as a new BMP file
    void saveImage(const char* filename) {
        // Update header for a 24-bit grayscale image (R=G=B)
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
    // Initialize the OpenACC runtime
    acc_init(acc_device_default);

    Grayscaler converter;
    if (!converter.loadImage("nasa_image.bmp"))
        return 1;

    cout << "Running grayscale conversion with OpenACC...\n";
    auto startTime = high_resolution_clock::now();

    converter.convertImage();

    auto endTime = high_resolution_clock::now();
    cout << "OpenACC Execution Time: "
         << duration_cast<milliseconds>(endTime - startTime).count() << " ms" << endl;

    converter.saveImage("gray_output_acc.bmp");

    // Shutdown the OpenACC runtime
    acc_shutdown(acc_device_default);
    
    return 0;
}
