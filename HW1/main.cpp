
#include <iostream>
#include <math.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <cassert>
#include <exception>

class Image
{
public:

    struct Rgb
    {
        Rgb() : r(0), g(0), b(0)  {}
        Rgb(float c) : r(c), g(c), b(c) {}
        Rgb(float _r, float _g, float _b) : r(_r), g(_g), b(_b) {}
        bool operator != (const Rgb &c) const { return c.r != r && c.g != g && c.b != b; }
        Rgb& operator *= (const Rgb &rgb) { r *= rgb.r, g *= rgb.g, b *= rgb.b; return *this; }
        Rgb& operator *= (const float scale) { r *= scale, g *= scale, b *= scale; return *this; }
        Rgb& operator += (const Rgb &rgb) { r += rgb.r, g += rgb.g, b += rgb.b; return *this; }
        Rgb& operator -= (const Rgb &rgb) { r = (r > rgb.r) ? r - rgb.r : 0,
                                            g = (g > rgb.g) ? g - rgb.g : 0,
                                            b = (b > rgb.b) ? b - rgb.b : 0; return *this; }
        float r, g, b;
    };

    Image() : p(0), w(0), h(0), pixels(nullptr)
    { /* empty image */ }

    Image(const unsigned int &_w, const unsigned int &_h, const Rgb &c = kBlack) :
        w(_w), h(_h), pixels(nullptr)
    {
        pixels = new Rgb[w * h];
        for (unsigned int i = 0; i < w * h; ++i) pixels[i] = c;
    }
    // copy constructor
    Image(const Image &img) : w(img.w), h(img.h), pixels(nullptr)
    {
        pixels = new Rgb[w * h];
        memcpy(pixels, img.pixels, sizeof(Rgb) * w * h);
    }
     // move constructor
    Image(Image &&img) : w(0), h(0), pixels(nullptr)
    {
        w = img.w;
        h = img.h;
        pixels = img.pixels;
        img.pixels = nullptr;
        img.w = img.h = 0;
    }
    // move assignment operator
    Image& operator = (Image &&img)
    {
        if (this != &img) {
            if (pixels != nullptr) delete [] pixels;
            w = img.w, h = img.h;
            pixels = img.pixels;
            img.pixels = nullptr;
            img.w = img.h = 0;
        }
        return *this;
    }
    Rgb& operator () (const unsigned &x, const unsigned int &y) const
    {
        assert(x < w && y < h);
        return pixels[y * w + x];
    }

    Image& operator += (const Image &img)
    {
        for (int i = 0; i < w * h; ++i) {
            pixels[i] += img[i];
            pixels[i] *= 0.5; // take averange
        };
        return *this;
    }
    Image& operator -= (const Image &img)
    {
        for (int i = 0; i < w * h; ++i) pixels[i] -= img[i];
        return *this;
    }
    Image operator * (const float scale)
    {
        Image tmp(*this);
        // multiply pixels together
        for (int i = 0; i < w * h; ++i) tmp[i] *= scale;
        return tmp;
    }
    Image operator + (const Image &img)
    {
        Image tmp(*this);
        // add pixels to each other
        for (int i = 0; i < w * h; ++i)
            tmp[i] += img[i];
        return tmp;
    }
    Image operator - (const Image &img)
    {
        Image tmp(*this);
        // add pixels to each other
        for (int i = 0; i < w * h; ++i)
            tmp[i] -= img[i];
        return tmp;
    }
    static Image gammacorrect(const Image &img, const float gamma){
        Image tmp(img.w, img.h);
        int w = img.w, h = img.h;
        for (int i = 0; i < w * h; ++i) {
            float c = 1;
            tmp.pixels[i].r = 255.f * pow(img.pixels[i].r / 255.f, gamma);
            tmp.pixels[i].g = 255.f * pow(img.pixels[i].g / 255.f, gamma);
            tmp.pixels[i].b = 255.f * pow(img.pixels[i].b / 255.f, gamma);
        }
        return tmp;
    }
    static Image alphacompo( Image &img_F,  Image &img_B, const float alpha) {
        // assert
        Image tmp(img_F.w, img_F.h);
        int w = img_F.w, h = img_F.h;
        tmp = img_F * alpha + img_B * (1-alpha);
        return tmp;

    }

    const Rgb& operator [] (const unsigned int &i) const { return pixels[i]; }
    Rgb& operator [] (const unsigned int &i) { return pixels[i]; }
    ~Image() { if (pixels != nullptr) delete [] pixels; }
    unsigned int p, w, h;
    Rgb *pixels;
    static const Rgb kBlack, kWhite, kRed, kGreen, kBlue;
};

const Image::Rgb Image::kBlack = Image::Rgb(0);

void savePPM(const Image &img, const char *filename)
{
    if (img.w == 0 || img.h == 0) { fprintf(stderr, "Can't save an empty image\n"); return; }
    std::ofstream ofs;
    try {
        ofs.open(filename, std::ios::binary); // need to spec. binary mode for Windows users
        if (ofs.fail()) throw("Can't open output file");
        ofs << "P6\n" << img.w << " " << img.h << "\n255\n";
        unsigned char r, g, b;
        // loop over each pixel in the image, clamp and convert to byte format
        for (int i = 0; i < img.w * img.h; ++i) {
            r = static_cast<unsigned char> (img.pixels[i].r) % 255;// (std::min(1.f, img.pixels[i].r) );
            g = static_cast<unsigned char> (img.pixels[i].g) % 255;//(std::min(1.f, img.pixels[i].g) );
            b = static_cast<unsigned char> (img.pixels[i].b) % 255;//(std::min(1.f, img.pixels[i].b) );
            ofs << r << g << b;
        }
        ofs.close();
    }
    catch (const char *err) {
        fprintf(stderr, "%s\n", err);
        ofs.close();
    }
}

Image readPPM(const char *filename)
{
    std::ifstream ifs;
    ifs.open(filename, std::ios::binary);
    Image img;
    try {
        if (ifs.fail()) { throw("Can't open input file"); }
        std::string header;
        int w, h, b;
        ifs >> header;
        if (strcmp(header.c_str(), "P6") == 0) {
            ifs >> w >> h >> b;
            img.w = w; img.h = h;
            img.pixels = new Image::Rgb[w * h];
            ifs.ignore(256, '\n'); // skip empty lines in necessary until we get to the binary data
            unsigned char pix[3];
            // read each pixel one by one and convert bytes to floats
            for (unsigned int i = 0; i < w * h; ++i) {
                ifs.read(reinterpret_cast<char *>(pix), 3);
                img.pixels[i].r = pix[0];
                img.pixels[i].g = pix[1];
                img.pixels[i].b = pix[2];
            }
        } else if (strcmp(header.c_str(), "P5") == 0) {
            std::cout << "P5" << "\n";
            ifs >> w >> h >> b;
            img.w = w; img.h = h;
            img.pixels = new Image::Rgb[w * h];
            ifs.ignore(256, '\n'); // skip empty lines in necessary until we get to the binary data
            unsigned char pix;
            // read each pixel one by one and convert bytes to floats
            for (unsigned int i = 0; i < w * h; ++i) {
                // ifs.read(reinterpret_cast<char *>(pix), 3);
                ifs >> pix;
                img.pixels[i].r = pix;
                img.pixels[i].g = pix;
                img.pixels[i].b = pix;
            }
        } else if (strcmp(header.c_str(), "P3") == 0) {
            std::string l1, l2, l3, line;
            ifs >> l1; // we assume there is at least a space in the comment
            while (l1.substr(0,1) == "#") {
                getline(ifs, line);
                ifs >> l1;
            }
            ifs >> l2 >> l3;
            img.w = atoi(l1.c_str()); img.h = atoi(l1.c_str());
            img.pixels = new Image::Rgb[img.w * img.h];
            unsigned char pix[3];
            for (unsigned int i = 0; i < img.w * img.h; ++i) {
                for (unsigned char & j : pix) {
                    ifs >> line;
                    j = atoi(line.c_str());
                }
                img.pixels[i].r = pix[0];
                img.pixels[i].g = pix[1];
                img.pixels[i].b = pix[2];
            }
        } else if (strcmp(header.c_str(), "P2") == 0) {
            std::string l1, l2, l3, line;
            ifs >> l1; // we assume there is at least a space in the comment
            while (l1.substr(0,1) == "#") {
                getline(ifs, line);
                ifs >> l1;
            }
            ifs >> l2 >> l3;
            img.w = atoi(l1.c_str()); img.h = atoi(l1.c_str());
            img.pixels = new Image::Rgb[img.w * img.h];
            unsigned char pix;
            unsigned char temp_v;
            for (unsigned int i = 0; i < img.w * img.h; ++i) {
                ifs >> line;
                pix = atoi(line.c_str());
                img.pixels[i].r = pix;
                img.pixels[i].g = pix;
                img.pixels[i].b = pix;
            }
        } else throw("Can't read input file: unknow P identifier.");
        ifs.close();
    }
    catch (const char *err) {
        fprintf(stderr, "%s\n", err);
        ifs.close();
    }
    return img;
}

int main(int argc, char **argv)
{
    try {
        printf("Start program\n");
        Image I = readPPM("./images/Mandrill.ppm");
        Image J = readPPM("./images/tandon_stacked_color.ppm");


        Image K = I + J;
        Image S = I - J;
        Image M = I * 1.3;
        Image G = Image::gammacorrect(K, 0.5);
        Image A85 = Image::alphacompo(I, J, 0.85);
        Image A50 = Image::alphacompo(I, J, 0.5);
        I += J;
        printf("start save\n");
        savePPM(K, "./Add.ppm");
        savePPM(I, "./AddAssign.ppm");
        savePPM(S, "./subtract.ppm");
        savePPM(M, "./times130.ppm");
        savePPM(G, "./gamma.ppm");
        savePPM(A85, "./alpha85.ppm");
        savePPM(A50, "./alpha50.ppm");
        savePPM(S, "./images/out.ppm");


    }
    catch (const std::exception &e) {
        fprintf(stderr, "Error: %s\n", e.what());
    }

    return 0;
}
