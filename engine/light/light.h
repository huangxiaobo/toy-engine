#ifndef __LIGHT_H__
#define __LIGHT_H__


enum LightType
{
    LightTypePoint, // 点光源
    LightTypeDirection, // 方向光源
    LightTypeSpot // 聚光灯
};


class Light
{
public:
    Light();

    Light(const LightType &light_type) : light_type(light_type) {}
    virtual const LightType& GetLightType() const { return light_type; };

    virtual ~Light() {}

private:
    LightType light_type;
};

#endif // __LIGHT_H__