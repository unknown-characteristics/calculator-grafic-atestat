#include "../graphContainerWindow.h"

int pointsPerPixel = 1;

graficFunctie::graficFunctie(functie* ptr, camera& cam)
{
    changeFunction(ptr);
    cameraResized(cam);
    generateGraph(cam);
}

void graficFunctie::changeFunction(functie* ptr)
{
    pFunc = ptr;
}

void graficFunctie::initPoints(camera& cam)
{
    pointList.clear();
    if (pFunc->state.funcLetter == L'x')
    {
        //vertical 
        double value = pFunc->evalFunc(NULL);
        point funcPoint; funcPoint.posWX = { value, 0 };
        funcPoint.posPX = cam.worldToPixel(funcPoint.posWX);
        pointList.push_back(funcPoint);
        return;
    }
    else if (pFunc->state.funcLetter == L'y'&&pFunc->state.variables.size()==0)
    {
        //horizontal
        double value = pFunc->evalFunc(NULL);
        point funcPoint; funcPoint.posWX = { 0, value };
        funcPoint.posPX = cam.worldToPixel(funcPoint.posWX);
        pointList.push_back(funcPoint);
        return;
    }
    else if (pFunc->state.variables.size() != 1) return;

    double increment = cam.effectiveUnitPerPixel.x/pointsPerPixel;

    double leftmostPos = cam.cameraPosWX.x - cam.cameraSizeWX.x / 2;
    leftmostPos = floor(leftmostPos / increment) * increment;
    double rightmostPos = cam.cameraPosWX.x + cam.cameraSizeWX.x / 2;
    rightmostPos = ceil(rightmostPos / increment) * increment;
    
    int pointListSize = ceil((rightmostPos - leftmostPos) / increment);
    pointList.resize(pointListSize, point(vector2(NAN, NAN), vector2(NAN, NAN), 0, 0));

    point currPoint; currPoint.posWX = { leftmostPos };
    double* tempVal = new double;
    
    int i = 0;
    while (currPoint.posWX.x <= rightmostPos)
    {
        *tempVal = currPoint.posWX.x;
        currPoint.posWX.y = pFunc->evalFunc(tempVal);
        if (i < pointListSize)
            pointList[i++] = currPoint;
        else pointList.emplace_back(currPoint);
        currPoint.posWX.x += increment;
    }
    delete tempVal;
    trimNAN();
}

void graficFunctie::trimNAN()
{
    int lastpos = pointList.size() - 1;
    while (lastpos >= 0 && isnan(pointList[lastpos].posWX.x)) lastpos--;
    lastpos++;
    pointList.erase(pointList.begin()+lastpos, pointList.end());
}

void graficFunctie::cameraResized(camera& cam)
{
    int newPointsArrSize = pointsPerPixel * cam.cameraSizePX.x + 2;
    if (newPointsArrSize <= pointsArrSize) return;

    pointsArrSize = newPointsArrSize;
    points.resize(pointsArrSize);
}

int graficFunctie::fixPoints(camera& cam)
{
    if (pFunc->isSpecialFunction())
        return generateGraph(cam);

    if (!canDrawGraph) return -1;

    double increment = cam.effectiveUnitPerPixel.x/pointsPerPixel;
    double leftmostPos = cam.cameraPosWX.x - cam.cameraSizeWX.x / 2;
    leftmostPos = floor(leftmostPos / increment) * increment;
    double rightmostPos = cam.cameraPosWX.x + cam.cameraSizeWX.x / 2;
    rightmostPos = ceil(rightmostPos / increment) * increment;

    point& firstElem = pointList.front(), & lastElem = pointList.back();
    int leftdif = (leftmostPos - firstElem.posWX.x) / increment, rightdif = (lastElem.posWX.x - rightmostPos) / increment;
    if (leftdif > (int)pointList.size()) leftdif = pointList.size();
    if (rightdif > (int)pointList.size()) rightdif = pointList.size();
    //handle deletions first
    if (leftdif > 0)
        pointList.erase(pointList.begin(), pointList.begin() + leftdif);
    if (rightdif > 0)
        pointList.erase(std::next(pointList.end(), -rightdif), pointList.end());

    //handle insertions
    if (leftdif < 0)
    {
        point currPoint; currPoint.posWX = firstElem.posWX.x - increment;
        double* tempVal = new double;
        while (currPoint.posWX.x >= leftmostPos)
        {
            *tempVal = currPoint.posWX.x;
            currPoint.posWX.y = pFunc->evalFunc(tempVal);
            pointList.emplace_front(currPoint);
            currPoint.posWX.x -= increment;
        }
        delete tempVal;
    }
    if (rightdif < 0)
    {
        point currPoint; currPoint.posWX = lastElem.posWX.x + increment;
        double* tempVal = new double;
        while (currPoint.posWX.x <= rightmostPos)
        {
            *tempVal = currPoint.posWX.x;
            currPoint.posWX.y = pFunc->evalFunc(tempVal);
            pointList.emplace_back(currPoint);
            currPoint.posWX.x += increment;
        }
        delete tempVal;
    }
    trimNAN();

    return generateGeometry(cam);
}



int graficFunctie::reduceResolution(camera& cam)
{
    resolutionChanged = 1;
    if (pFunc->isSpecialFunction())
        return generateGraph(cam);

    if (!canDrawGraph) return -1;

    double removalIncrement = cam.effectiveUnitPerPixel.x / pointsPerPixel;
    double oldIncrement = pointList[1].posWX.x - pointList[0].posWX.x;
    int ratio = round(removalIncrement / oldIncrement);

    double leftmostPos = floor((cam.cameraPosWX.x - cam.cameraSizeWX.x / 2) / removalIncrement) * removalIncrement;
    int index = int((pointList[0].posWX.x - leftmostPos) / oldIncrement) % ratio;

    int cnt = std::erase_if(pointList, [&index, ratio](const point& pt)
    {
        return (index++)%ratio!=0;
    });
    trimNAN();
    return fixPoints(cam);
}

int graficFunctie::increaseResolution(camera& cam)
{
    resolutionChanged = 1;
    if (pFunc->isSpecialFunction())
        return generateGraph(cam);

    if (!canDrawGraph) return -1;

    tempPointList.clear();
    
    double increment = cam.effectiveUnitPerPixel.x / pointsPerPixel;

    double leftmostPos = cam.cameraPosWX.x - cam.cameraSizeWX.x / 2;
    leftmostPos = floor(leftmostPos / increment) * increment;
    double rightmostPos = cam.cameraPosWX.x + cam.cameraSizeWX.x / 2;
    rightmostPos = ceil(rightmostPos / increment) * increment;
    //if (rightmostPos <= leftmostPos + cam.cameraSizeWX.x) rightmostPos += increment;
    int pointListSize = ceil((rightmostPos - leftmostPos) / increment);
    tempPointList.resize(pointListSize, point(vector2(NAN, NAN), vector2(NAN, NAN), 0, 0));

    point currPoint; currPoint.posWX = { leftmostPos };
    double* tempVal = new double;

    int i = 0, j = 0;
    while (currPoint.posWX.x <= rightmostPos)
    {
        while (pointList[j].posWX.x < currPoint.posWX.x && j < pointList.size()) j++;
        if (j < pointList.size() && pointList[j].posWX.x == currPoint.posWX.x)
        {
            if(i < pointListSize)
                tempPointList[i++] = pointList[j];
            else tempPointList.emplace_back(pointList[j]); //imprecision might lead to incorrect ammount of points in resize()
        }
        else
        {
            *tempVal = currPoint.posWX.x;
            currPoint.posWX.y = pFunc->evalFunc(tempVal);
            if (i < pointListSize)
                tempPointList[i++] = currPoint;
            else tempPointList.emplace_back(currPoint); //imprecision might lead to incorrect ammount of points in resize()
        }
        currPoint.posWX.x += increment;
    }
    delete tempVal;

    pointList.swap(tempPointList);
    trimNAN();
    return fixPoints(cam);
}

int graficFunctie::generateGeometry(camera& cam)
{
    SafeRelease(&pPathGeom);
    HRESULT hr = pFactory->CreatePathGeometry(&pPathGeom);
    if (FAILED(hr)) return -1;

    ID2D1GeometrySink* pSink = NULL;
    hr = pPathGeom->Open(&pSink);
    if (FAILED(hr)) return -1;
    pSink->SetFillMode(D2D1_FILL_MODE_WINDING);

    if (pFunc->state.funcLetter == L'x')
    {
        canDrawGraph = 1;
        point& firstElem = *pointList.begin();
        if (firstElem.posPX.x >= 0 && firstElem.posPX.x <= cam.cameraSizePX.x)
        {
            pSink->BeginFigure({ (float)firstElem.posPX.x, 0 }, D2D1_FIGURE_BEGIN_HOLLOW);
            pSink->AddLine({ (float)firstElem.posPX.x, (float)cam.cameraSizePX.y });
            pSink->EndFigure(D2D1_FIGURE_END_OPEN);
        }
    }
    else if (pFunc->state.funcLetter == L'y' && pFunc->state.variables.size() == 0)
    {
        canDrawGraph = 1;
        point& firstElem = *pointList.begin();
        if (firstElem.posPX.y >= 0 && firstElem.posPX.y <= cam.cameraSizePX.y)
        {
            pSink->BeginFigure({ 0, (float)firstElem.posPX.y }, D2D1_FIGURE_BEGIN_HOLLOW);
            pSink->AddLine({  (float)cam.cameraSizePX.x, (float)firstElem.posPX.y });
            pSink->EndFigure(D2D1_FIGURE_END_OPEN);
        }
    }
    else
    {
        int index = 0, buildingFigure = 0, shouldDrawLine = 0;

        pointList[0].posPX = cam.worldToPixel(pointList[0].posWX);
        for (int i = 0; i < pointList.size()-1; i++)
        {
            point& currElem = pointList[i], & nextElem = pointList[i+1];
            nextElem.posPX = cam.worldToPixel(nextElem.posWX); //currElem.posPX was calculated at previous step

            if (resolutionChanged) currElem.continuityChecked = 0; //changing resolution changes the following point so the continuity check must be discarded (not always but not worth optimisation)

            if (currElem.posPX.y < 0 && nextElem.posPX.y < 0) shouldDrawLine = 0;
            else if (currElem.posPX.y > cam.cameraSizePX.y && nextElem.posPX.y > cam.cameraSizePX.y) shouldDrawLine = 0;
            else if (isnan(currElem.posPX.y) || isnan(nextElem.posPX.y) || !isfinite(currElem.posPX.y) || !isfinite(nextElem.posPX.y)) shouldDrawLine = 0;
            else if (abs(currElem.posPX.y - nextElem.posPX.y) >= 3)
            {
                if (currElem.continuityChecked)
                    shouldDrawLine = currElem.continuous;
                else
                {
                    int rez = checkContinuity(pFunc, currElem, nextElem);
                    currElem.continuityChecked = 1;
                    currElem.continuous = shouldDrawLine = rez;
                }
            }
            else shouldDrawLine = 1;

            if (shouldDrawLine)
            {
                if (!buildingFigure)
                {
                    buildingFigure = 1;
                    index = 0;
                    pSink->BeginFigure({ (float)currElem.posPX.x, (float)currElem.posPX.y }, D2D1_FIGURE_BEGIN_HOLLOW);
                }
                points[index++] = { (float)nextElem.posPX.x, (float)nextElem.posPX.y };
            }
            else
            {
                if (buildingFigure)
                {
                    buildingFigure = 0;
                    pSink->AddLines(points.data(), index); index = 0;
                    pSink->EndFigure(D2D1_FIGURE_END_OPEN);
                    
                }
            }
        }

        if(buildingFigure)
        {
            pSink->AddLines(points.data(), index);
            pSink->EndFigure(D2D1_FIGURE_END_OPEN);
        }
    }
    pSink->Close();
    SafeRelease(&pSink);
    canDrawGraph = 1; resolutionChanged = 0;
    return 0;
}

int graficFunctie::checkContinuity(functie* ptr, point a, point b)
{
    double ydistance = abs(a.posWX.y - b.posWX.y), xdistance = b.posWX.x - a.posWX.x;
    double midY = (a.posWX.y + b.posWX.y) / 2;

    double x1 = a.posWX.x + xdistance / 3, x2 = b.posWX.x - xdistance / 3;
    double y1 = ptr->evalFunc(&x1), y2 = ptr->evalFunc(&x2);

    if (isnan(y1) || isnan(y2) || !isfinite(y1) || !isfinite(y1)) return 0;
    if (abs(y1 - midY) / ydistance <= 0.45) return 1;
    if (abs(y2 - midY) / ydistance <= 0.45) return 1;
    return 0;
}

int graficFunctie::generateGraph(camera& cam)
{
    canDrawGraph = 0;
    if (pFunc==NULL) return -1;
    eroare err = pFunc->state.eroareFunctie;
    if (err.errorType != ErrorType::NoError || pFunc->state.variables.size() > 1) return -1;
    if (pFunc->state.variables.size() == 0 && !(pFunc->state.funcLetter == L'x' || pFunc->state.funcLetter == L'y')) return -1;
    initPoints(cam);
    return generateGeometry(cam);
}
