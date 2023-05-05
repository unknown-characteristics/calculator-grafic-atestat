#include "camera.h"
using namespace std;



camera::camera()
{
    initMembers();
}

camera::camera(vector2 cSize)
{
    initMembers();
    resizeCamera(cSize);
}

camera::camera(vector2 cPos, vector2 cSize)
{
    initMembers();
    resizeCamera(cSize);
    moveCamera(cPos);
}

void camera::initMembers()
{
    cameraPosPX = cameraPosWX = cameraSizePX = cameraSizeWX = 0;
    scrollFactor = pow(2, 0.25);
    effectiveUnitPerPixel = 1.f / 128; effectiveUnitPerPixel.y *= -1; unitPerPixel = effectiveUnitPerPixel / scrollFactor / scrollFactor;
    slideDelta = 7;
}

void camera::resizeCamera(vector2 newSizePX)
{
    cameraSizePX = newSizePX;
    cameraPosPX = cameraSizePX/2;
    cameraSizeWX = cameraSizePX * unitPerPixel;
}

void camera::moveCamera(vector2 newPosWX)
{
    cameraPosWX = newPosWX;
}

vector2 camera::pixelToWorld(vector2 pixel)
{
    vector2 delta = pixel - cameraPosPX;
    delta *= unitPerPixel;
    return cameraPosWX + delta;
}

vector2 camera::worldToPixel(vector2 world)
{
    vector2 delta = world - cameraPosWX;
    delta /= unitPerPixel;
    return cameraPosPX + delta;
}


int camera::zoomCamera(vector2 zoomPosPX, int rotations)
{
    double scrollDelta = pow(scrollFactor, rotations);
    vector2 delta = pixelToWorld(zoomPosPX) - cameraPosWX;
    vector2 zoomPosWX = pixelToWorld(zoomPosPX);
    if (unitPerPixel.x / scrollDelta <= 1e-15)
    {
        scrollDelta = unitPerPixel.x / 1e-15;
        unitPerPixel.x = 1e-15; unitPerPixel.y = -1e-15;
    }
    else
        unitPerPixel /= scrollDelta;
    delta /= scrollDelta;
    cameraSizeWX = cameraSizePX * unitPerPixel;
    moveCamera(zoomPosWX - delta);

    if (unitPerPixel.x >= effectiveUnitPerPixel.x) //zoomed out
    {
        while(unitPerPixel.x >= effectiveUnitPerPixel.x)
            effectiveUnitPerPixel *= 2; 
        return -1;
    }
    else if (unitPerPixel.x <= effectiveUnitPerPixel.x / 2) //zoomed in
    {
        while (unitPerPixel.x <= effectiveUnitPerPixel.x / 2)
            effectiveUnitPerPixel /= 2; 
        return 1;
    }
    else return 0;
}

void camera::slideCamera(vector2 direction)
{
    moveCamera(cameraPosWX + direction * slideDelta * unitPerPixel);
}

void camera::resetCamera(int resetScale = 0)
{
    cameraPosWX = { 0, 0 };
    if (!resetScale) return;
    effectiveUnitPerPixel = 1.f / 128; effectiveUnitPerPixel.y *= -1; unitPerPixel = effectiveUnitPerPixel / scrollFactor / scrollFactor;
    cameraSizeWX = cameraSizePX * unitPerPixel;
}