#ifndef __TEXTURE_H__

#include <string>

using namespace std;

class Texture
{
public:
    Texture();
    ~Texture();

public:
    unsigned int id;
    string type;
    string path;
};

#endif // !__TEXTURE_H__