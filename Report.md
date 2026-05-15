# 项目报告：简易3D函数图像生成器(by Deepseek)

## 1. 项目概述

**项目名称**：Simple-3D-Function-Graph-Generator  
**开发环境**：Visual Studio 2022 + Qt 5.15.2 (MSVC 2019 64-bit)  
**主要依赖**：Qt Core, Qt Widgets, Qt Data Visualization  
**项目类型**：桌面应用程序（基于Qt框架）

**功能描述**：  
用户输入关于变量 `x` 和 `y` 的数学表达式（例如 `sin(x) * cos(y)`、`x^2 + y^2`），选择坐标轴范围（±5, ±10, ±20）和网格分辨率（1~3），点击“渲染”按钮后，程序动态生成3D曲面图并显示在窗口中，支持鼠标交互（旋转、缩放）。

---

## 2. 整体架构设计

项目采用 **MVC 风格的模块化设计**，分为三个核心部分：

1. **表达式解析与求值模块**（`ExpressionEvaluator.h`）  
   - 独立于GUI，提供字符串表达式到数学函数的转换。
2. **3D曲面生成与渲染模块**（`Simple3DFunctionGraphGenerator` 类）  
   - 负责UI交互、数据采样、曲面构建及Qt Data Visualization渲染。
3. **主程序入口**（`main.cpp`）  
   - 初始化Qt应用，创建主窗口并启动事件循环。

各模块通过清晰的接口协作，实现了关注点分离。

---

## 3. 模块详细设计与实现

### 3.1 表达式解析与求值模块 (`ExpressionEvaluator.h`)

**设计目标**：  
提供一个轻量级、可嵌入的表达式求值器，支持常见数学函数、二元运算符（`+ - * / ^`）、一元负号、变量 `x` 和 `y`。

**实现方式**：  
- **词法分析（Lexer）**：  
  将输入字符串解析为Token序列，支持数字、变量（x/y）、函数名（sin/cos等）、运算符、括号。  
- **语法分析（Parser）**：  
  采用递归下降算法，按优先级构建抽象语法树（AST），节点类型包括：`NumberNode`、`VariableNode`、`BinaryOpNode`、`UnaryNode`、`FunctionNode`。  
- **求值（Evaluate）**：  
  遍历AST，传入变量映射表（`std::map<std::string, double>`），动态计算表达式的值。

**特点**：  
- 纯头文件实现（所有成员函数在类内定义），无需单独编译 `.cpp`，方便集成。  
- 所有类位于 `ExprEval` 命名空间，避免命名污染。  
- 对外提供简洁的 `Expression` 类，仅需两行代码即可使用：
  ```cpp
  Expression expr("sin(x)+cos(y)");
  double result = expr.evaluate(1.2, 3.4);
  ```

### 3.2 主窗口与3D曲面渲染模块 (`Simple3DFunctionGraphGenerator`)

**设计目标**：  
提供友好的图形界面，接收用户输入，动态生成3D曲面并支持视角交互。

**UI设计**（基于Qt Designer生成 `.ui` 文件）：  
- 顶部输入栏：`z= ` + `QLineEdit` 用于输入表达式。  
- 控制栏：范围下拉框（`QComboBox`）、分辨率下拉框、渲染按钮（`QPushButton`）。  
- 中央区域：`QWidget` 容器，用于嵌入 `Q3DSurface` 窗口。

**核心类成员**：  
- `Ui::Simple3DFunctionGraphGeneratorClass ui`：自动生成的UI对象。  
- `QtDataVisualization::Q3DSurface* m_surface`：3D曲面窗口指针。  
- 槽函数：`onDrawButtonClicked()` 处理渲染按钮点击事件。  
- 辅助函数：`updateSurface(z, rg, rsln)` 根据采样数据更新曲面。

**工作流程**：  
1. **初始化**：构造函数中设置UI布局，创建 `Q3DSurface` 容器，绑定按钮信号。  
2. **参数获取**：用户点击渲染按钮后，读取表达式字符串、范围 `rg`、分辨率 `rsln`。  
3. **数据采样**：  
   - 使用内部类 `GridSampling` 生成均匀网格点（`x` 和 `y` 坐标）。  
   - 对每个网格点调用 `Expression::evaluate(x, y)` 计算 `z` 值，存储为二维 `vector<vector<double>>`。  
4. **曲面构建**：  
   - 根据 `z` 数据创建 `QSurfaceDataArray`，每个数据点用 `QSurfaceDataItem` 设置三维坐标 `(x, y, z)`。  
   - 构造 `QSurfaceDataProxy` 并传入数据数组，再创建 `QSurface3DSeries` 关联该代理。  
   - 设置曲面样式（着色模式、线框、基础颜色、阴影质量）。  
5. **场景更新**：  
   - 移除旧的 `QSurface3DSeries`，添加新系列。  
   - 调整坐标轴范围：X、Z 轴使用用户设定的 `[-rg, rg]`；Y 轴范围当前固定为 `[-rg, rg]`（代码中注释了动态Y轴计算，可作为后续优化点）。  
   - 隐藏背景（通过 `activeTheme()->setBackgroundEnabled(false)` 获得干净视图）。  
6. **错误处理**：捕获表达式求值抛出的异常，通过 `QMessageBox` 提示用户。

### 3.3 主程序入口 (`main.cpp`)

```cpp
int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    Simple3DFunctionGraphGenerator w;
    w.show();
    return a.exec();
}
```

**职责**：  
- 创建 Qt 应用对象，实例化主窗口并显示，进入消息循环。  
- 不包含任何业务逻辑，保持简洁。

---

## 4. 关键技术点

| 方面 | 技术实现 |
|------|----------|
| **表达式解析** | 递归下降解析器 + 抽象语法树 (AST) |
| **数学函数** | 使用 `std::sin`、`std::cos`、`std::pow` 等标准库函数 |
| **3D可视化** | Qt Data Visualization 模块 (`Q3DSurface`, `QSurfaceDataProxy`, `QSurface3DSeries`) |
| **UI布局** | Qt Designer 可视化设计 + 生成的 `ui_*.h` 文件 |
| **信号与槽** | 使用 `QPushButton::clicked` 触发自定义槽函数 `onDrawButtonClicked` |
| **动态数据更新** | 重建 `QSurfaceDataArray` 和 `QSurface3DSeries` 替换旧系列 |

---

## 5. 项目文件结构说明

| 文件 | 作用 |
|------|------|
| `main.cpp` | 程序入口 |
| `Simple3DFunctionGraphGenerator.h/.cpp` | 主窗口类声明与实现，核心渲染逻辑 |
| `Simple3DFunctionGraphGenerator.ui` | UI布局文件（XML格式） |
| `ExpressionEvaluator.h` | 表达式求值器（纯头文件） |
| `Simple3DFunctionGraphGenerator.qrc` | Qt资源文件（未使用实质资源，可为空） |
| `.vcxproj` / `.filters` | Visual Studio项目配置，已正确设置Qt模块（`core;gui;widgets;datavisualization`） |

---

## 6. 编译与运行

- 使用 Visual Studio 2022 打开 `.vcxproj` 文件，确保 Qt 5.15.2 已安装并配置好 `QtMsBuild`。  
- 选择 `Debug|x64` 或 `Release|x64` 配置，生成解决方案。  
- 运行生成的可执行文件，输入表达式（例如 `sin(x)*cos(y)`），选择范围和分辨率，点击“渲染”即可看到3D曲面。

---

## 7. AI使用

使用了Deepseek,用途如下
- 将'Inputer.cpp'转化为'ExpressionEvaluator.h'头文件
- 提供'ExpressionEvaluator.h'和'Simple3DFunctionGraphGenerator.cpp'编写思路
- 提供错误排查指导
- 生成此文本
