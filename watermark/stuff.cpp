/**
* Name : Simone
* Surname : Schirinzi
* Matriculation number : 490209
* Software : Image Watermark
*
* I declare that the content of this file is in its entirety the original work of the author.
*
* Simone Schirinzi
*/

#ifndef SPM_WATERMARK_STUFF_H
#define SPM_WATERMARK_STUFF_H

#define cimg_display 0
#define cimg_use_jpeg 1

#include <string>
#include <iostream>
#include "CImg.h"

class Image {
private:
public:
    std::vector<std::pair<const unsigned int, const unsigned int>>* mark;
    cimg_library::CImg<std::uint8_t> img;
    std::string inPath;
    std::string outPath;

    Image(std::string inPath, std::string outPath, std::vector<std::pair<const unsigned int, const unsigned int>>* mark){
        this->inPath = inPath;
        this->outPath = outPath;
        this->img = cimg_library::CImg<std::uint8_t>((const unsigned int)0);
        this->mark = mark;
    }
};

inline std::vector<std::pair<const unsigned int, const unsigned int>> parseWatermark(std::string f){
    cimg_library::CImg<std::uint8_t> mrk(f.data());
    std::vector<std::pair<const unsigned int, const unsigned int>> v;
    for(unsigned int y = 0; y < mrk.height(); y++)
        for(unsigned int x = 0; x < mrk.width(); x++)
            if(mrk(x,y) < (std::uint8_t)128)
                v.push_back(std::pair<const unsigned int, const unsigned int>(x,y));
    return v;
};

inline int dirToJpgList(std::string dir, std::vector<std::string> &files){
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        std::cout << "Error(" << errno << ") opening " << dir << std::endl;
        return errno;
    }

    while ((dirp = readdir(dp)) != NULL) {
        std::string str = std::string(dirp->d_name);
        if(str.length() > 4 && str.find("jpg") == str.length() - 3)
            files.push_back(std::string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}

/**
 * Rewrite of offset CImg function:
 * longT offset(const int x, const int y=0, const int z=0, const int c=0) const {
 *     return x + (longT)y*_width + (longT)z*_width*_height + (longT)c*_width*_height*_depth;
 * }
 */
inline cimg_library::CImg<std::uint8_t> filter (cimg_library::CImg<std::uint8_t> img,
                      std::vector<std::pair<const unsigned int, const unsigned int>> mark){
    cimg_library::CImg<std::uint8_t>::ulongT off, imgSize = img.size(), s;
    cimg_library::CImg<std::uint8_t>::ulongT c1 = (cimg_library::CImg<std::uint8_t>::ulongT) ((cimg_library::CImg<std::uint8_t>::longT)img._width*img._height*img._depth);
    cimg_library::CImg<std::uint8_t>::ulongT c2 = 2*c1;

    auto w = img._width;
    auto markSize = mark.size();
    for(unsigned int p = 0; p < markSize; p++){
        //precompute offset
        off = (cimg_library::CImg<std::uint8_t>::ulongT)(mark[p].first + (cimg_library::CImg<std::uint8_t>::longT) mark[p].second*w);

        //avoid null pointer exception
        if (off < imgSize)img._data[off] = (std::uint8_t) 0;
        s = off + c1; if (s < imgSize) img._data[s] = (std::uint8_t) 0;
        s = off + c2; if (s < imgSize) img._data[s] = (std::uint8_t) 0;

    }
    return img;
}

inline cimg_library::CImg<std::uint8_t> open(std::string path){
    return cimg_library::CImg<std::uint8_t>(path.data());
}

inline void close(cimg_library::CImg<std::uint8_t> img, std::string path){
    img.save_jpeg(path.data());
    img.assign();
}

#endif