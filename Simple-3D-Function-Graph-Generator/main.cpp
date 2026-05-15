#include<iostream>
#include<string>
#include<vector>
#include<cmath>
#include <QtWidgets/QApplication>
#include <Qt3DExtras/Qt3DWindow>
#include <QtDataVisualization/Q3DSurface>
#include <QtDataVisualization/QSurfaceDataProxy>
#include <QtDataVisualization/QSurface3DSeries>
#include <QComboBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QtWidgets>
#include "ExpressionEvaluator.h"
#include "Simple3DFunctionGraphGenerator.h"



int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	Simple3DFunctionGraphGenerator w;
	w.show();
	return a.exec();
}