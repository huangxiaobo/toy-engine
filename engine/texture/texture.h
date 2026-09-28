#ifndef __TEXTURE_H__

#include <string>

class Texture {
public:
    Texture();

    ~Texture();

public:
    unsigned int m_id;
    std::string m_type;
    std::string m_path;
};

#endif // # __TEXTURE_H__
