#pragma once
#ifndef __camera_header
#define __camera_header

#include <math.h>
struct vector2
{
    double x = 0, y = 0;
    constexpr vector2(double n)
    {
        x = n; y = n;
    }
    constexpr vector2(double a, double b)
    {
        x = a; y = b;
    }
    constexpr vector2()
    {
        x = y = 0;
    }
    constexpr vector2& operator+=(vector2 b)
    {
        x += b.x;
        y += b.y;
        return *this;
    }
    constexpr vector2& operator-=(vector2 b)
    {
        x -= b.x;
        y -= b.y;
        return *this;
    }
    constexpr vector2& operator*=(vector2 b)
    {
        x *= b.x;
        y *= b.y;
        return *this;
    }
    constexpr vector2& operator/=(vector2 b)
    {
        if (b.x == 0) x = NAN;
        else x /= b.x;
        if (b.y == 0) y = NAN;
        else y /= b.y;
        return *this;
    }
};

constexpr vector2 operator+(vector2 a, vector2 b)
{
    return a+=b;
}
constexpr vector2 operator-(vector2 a, vector2 b)
{
    return a-=b;
}
constexpr vector2 operator*(vector2 a, vector2 b)
{
    return a*=b;
}
constexpr vector2 operator/(vector2 a, vector2 b)
{
    return a/=b;
}
constexpr bool operator==(vector2 a, vector2 b)
{
    return a.x == b.x && a.y == b.y;
}

class camera
{
    public:
        vector2 cameraPosPX, cameraPosWX, cameraSizePX, cameraSizeWX, unitPerPixel, effectiveUnitPerPixel;
        double scrollFactor, slideDelta;

    camera();
    camera(vector2 cSize);
    camera(vector2 cPos, vector2 cSize);

    void initMembers();

    void resizeCamera(vector2 newSizePX);
    void moveCamera(vector2 newPosWX);
    int zoomCamera(vector2 zoomPosPX, int rotations);
    void slideCamera(vector2 direction);
    void resetCamera(int resetScale);

    vector2 pixelToWorld(vector2 pixel);
    vector2 worldToPixel(vector2 world);
};


#endif // __camera_header
