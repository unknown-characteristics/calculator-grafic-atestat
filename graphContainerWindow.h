#pragma once
#include "baseWindow.h"
#include "mate/mate.h"
#include <deque>
#include <vector>
enum class targetType;

struct point
{
	vector2 posWX, posPX;
	int continuityChecked = 0, continuous = 0; //these refer to the line whose left point is this point only!
};


class graficFunctie
{
public:
	functie* pFunc;

	graficFunctie() : pFunc(NULL), pPathGeom(NULL) {};
	graficFunctie(functie* ptr, camera& cam);
	void changeFunction(functie* ptr);
	int generateGraph(camera& cam);
	void initPoints(camera& cam);
	void cameraResized(camera& cam);
	void trimNAN();
	int fixPoints(camera& cam);
	int reduceResolution(camera& cam);
	int increaseResolution(camera& cam);
	int generateGeometry(camera& cam);
	static int checkContinuity(functie* ptr, point a, point b);

	int pointsDone = 0, geometryDone = 0, updatePostponed = 0, canDrawGraph = 0, resolutionChanged = 0;

	std::deque<point> pointList, tempPointList;
	ID2D1PathGeometry* pPathGeom;
	std::vector<D2D1_POINT_2F> points;
	int pointsArrSize = 0;
};

class graphContainerWindow : public baseWindowDrawable<graphContainerWindow>
{
public:
	LRESULT procMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	virtual HRESULT CreateGraphicsResources();
	virtual void DiscardGraphicsResources();
	void handleMainUnitSize();
	void RecreateGraphs();
	void GraphAccuracyChanged();
	void RecreateGraphsAfterMove();
	void RecreateGraphsAfterZoom(int mode);
	int RecreateGraph(int i);
	int RecreateGraph(graficFunctie* ptr);
	int hitTestGraphs(int x, int y);
	void itemChanged(targetType target, int index1, int index2);

	int capturing = 0, listLength = 0;
	ID2D1SolidColorBrush* pBrush = NULL, *pWhiteBrush = NULL;
	ID2D1StrokeStyle* pGraphStroke = NULL;
	IDWriteTextFormat* pSmallFormat = NULL, * pPointFormat = NULL;
	IDWriteTextLayout* pPointLayout = NULL;
	camera cam;
	graficFunctie* graf[101] = { NULL };
	graficFunctie* axaOX, *axaOY;
};


extern int pointsPerPixel;

extern graphContainerWindow* globalGraphContainerWindow;
