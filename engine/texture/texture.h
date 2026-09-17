#ifndef __TEXTURE_H__

#include <string>

class Texture {
public:
    Texture();

    ~Texture();

public:
    unsigned int id;
    std::string type;
    std::string path;
};

#endif // # __TEXTURE_H__
