#include <QtWidgets/QMessageBox>
#include <QtWidgets/QVBoxLayout>
#include <QtDataVisualization/QSurfaceDataProxy>
#include <QtDataVisualization/QSurface3DSeries>
#include <QtDataVisualization/QValue3DAxis>
#include <cmath>
#include <vector>
#include <string>
#include <iostream>
#include "Simple3DFunctionGraphGenerator.h"
#include "ExpressionEvaluator.h"

using namespace QtDataVisualization;

// 网格采样数据类（原样保留）
class GridSampling {
public:
    GridSampling(int _rg, int _rsln, std::string _line) {
        rg = _rg; rsln = _rsln; line = _line;
        n = 1 + 2 * pow(10, rsln - 1) * rg;
        length = pow(0.1, rsln - 1);
    };

    std::vector<std::vector<double>> returnArray() {
        spawnSamplingGrid();
        evaluateSample();
        return z;
    }

private:
    int rg, rsln, n;
    double length;
    std::string line;
    std::vector<double> x, y, temp;
    std::vector<std::vector<double>> z;

    void spawnSamplingGrid() {
        for (int i = 0; i < n; i++) {
            x.push_back(i * length - rg);
        }
        y = x;
    }

    void evaluateSample() {
        Expression expr(line);
        for (int i = 0; i < n; i++) {
            temp.clear();
            for (int j = 0; j < n; j++) {
                temp.push_back(expr.evaluate(x[i], y[j]));
            }
            z.push_back(temp);
        }
    }
};

// 构造函数
Simple3DFunctionGraphGenerator::Simple3DFunctionGraphGenerator(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    // 为 ui.widget 设置布局（该 widget 在 .ui 中专门用于显示 3D 曲面）
    QVBoxLayout* surfaceLayout = new QVBoxLayout(ui.widget);
    ui.widget->setLayout(surfaceLayout);

    // 创建 3D 曲面窗口
    m_surface = new Q3DSurface();
    QWidget* container = QWidget::createWindowContainer(m_surface);
    surfaceLayout->addWidget(container);

    // 填充下拉框
    ui.comboBox_rg->addItems({ "5", "10", "20" });
    ui.comboBox_rsln->addItems({ "1", "2", "3" });

    // 连接按钮信号
    connect(ui.pushButton, &QPushButton::clicked,
        this, &Simple3DFunctionGraphGenerator::onDrawButtonClicked);
}

// 析构函数（无需特殊处理，Qt 会自动清理父子对象）
Simple3DFunctionGraphGenerator::~Simple3DFunctionGraphGenerator() {}

// 渲染按钮槽函数
void Simple3DFunctionGraphGenerator::onDrawButtonClicked()
{
    // 获取界面上的参数
    QString expr = ui.lineEdit_expr->text();
    int rg = ui.comboBox_rg->currentText().toInt();
    int rsln = ui.comboBox_rsln->currentText().toInt();

    // 检查表达式是否为空
    if (expr.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入表达式");
        return;
    }

    try {
        // 生成网格数据
        GridSampling gdsp(rg, rsln, expr.toStdString());
        std::vector<std::vector<double>> z = gdsp.returnArray();

        if (z.empty() || z[0].empty()) {
            QMessageBox::warning(this, "错误", "网格数据为空，请检查参数或表达式");
            return;
        }

        // 更新曲面
        updateSurface(z, rg, rsln);
    }
    catch (const std::exception& e) {
        QMessageBox::critical(this, "表达式错误", e.what());
    }
}

// 更新 3D 曲面
void Simple3DFunctionGraphGenerator::updateSurface(const std::vector<std::vector<double>>& z, int rg, int rsln)
{
    if (!m_surface) {
        QMessageBox::critical(this, "错误", "3D 曲面对象未初始化");
        return;
    }

    // 计算网格维度
    int rows = static_cast<int>(z.size());      // Z 方向点数
    int cols = static_cast<int>(z[0].size());   // X 方向点数

    // 创建数据数组
    QSurfaceDataArray* dataArray = new QSurfaceDataArray;
    dataArray->reserve(rows);

    float rangeMin = -static_cast<float>(rg);
    float rangeMax = static_cast<float>(rg);
    float stepX = (rangeMax - rangeMin) / (cols - 1);
    float stepZ = (rangeMax - rangeMin) / (rows - 1);

    // 遍历 Z 值，构造曲面点
    for (int i = 0; i < rows; ++i) {
        QSurfaceDataRow* newRow = new QSurfaceDataRow(cols);
        float zVal = rangeMin + i * stepZ;
        for (int j = 0; j < cols; ++j) {
            float xVal = rangeMin + j * stepX;
            float yVal = static_cast<float>(z[i][j]);
            (*newRow)[j].setPosition(QVector3D(xVal, yVal, zVal));
        }
        dataArray->append(newRow);
    }

    // 创建数据代理和序列
    QSurfaceDataProxy* proxy = new QSurfaceDataProxy();
    proxy->resetArray(dataArray);   // proxy 接管 dataArray
    QSurface3DSeries* series = new QSurface3DSeries(proxy);
    series->setDrawMode(QSurface3DSeries::DrawSurfaceAndWireframe);
    series->setFlatShadingEnabled(false);

    series->setBaseColor(QColor(173, 216, 230));   // 浅蓝色
    series->setDrawMode(QSurface3DSeries::DrawSurface);
    series->setFlatShadingEnabled(false);

    // 设置阴影（投影）
    m_surface->setShadowQuality(QAbstract3DGraph::ShadowQualitySoftLow);

    // 移除旧序列，添加新序列
    QList<QSurface3DSeries*> oldSeriesList = m_surface->seriesList();
    for (QSurface3DSeries* oldSeries : oldSeriesList) {
        m_surface->removeSeries(oldSeries);
        delete oldSeries;
    }
    m_surface->addSeries(series);

    // 获取当前的活动主题
    Q3DTheme* theme = m_surface->activeTheme();

    // 隐藏面板背景
    theme->setBackgroundEnabled(false);

    // 设置坐标轴范围
    QValue3DAxis* axisX = m_surface->axisX();
    QValue3DAxis* axisY = m_surface->axisY();
    QValue3DAxis* axisZ = m_surface->axisZ();

    axisX->setRange(rangeMin, rangeMax);
    axisZ->setRange(rangeMin, rangeMax);
    axisY->setRange(rangeMin, rangeMax);

    // 计算 Y 轴范围（函数值的范围）
    /*
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            float val = static_cast<float>(z[i][j]);
            if (val < minY) minY = val;
            if (val > maxY) maxY = val;
        }
    }
    float yPadding = (maxY - minY) * 0.1f;
    axisY->setRange(minY - yPadding, maxY + yPadding);
    */

}