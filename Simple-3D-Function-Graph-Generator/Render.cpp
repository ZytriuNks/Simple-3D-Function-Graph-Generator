#include<iostream>
#include<string>
#include<vector>
#include<cmath>
#include <Qt3DExtras/Qt3DWindow>
#include "ExpressionEvaluator.h"

//网格采样数据
class GridSampling {
public:
	GridSampling(int _rg, int _rsln, std::string _line) {
		rg = _rg; rsln = _rsln; line = _line;
		n = 1 + 2 * pow(10 , rsln - 1) * rg;
		length = pow(0.1 ,rsln - 1);
	};

	//返回
	std::vector<std::vector<double>> returnArray() {
		spawnSamplingGrid();
		evaluateSample();
		return z;
	}

	//测试
	void test() {

		std::cout << "n=" << n  << "\tlen=" << length << std::endl;

		for (auto i = 0; i < x.size(); i++) {
			std::cout << x[i] << " ";
		}

		std::cout << std::endl;

		for (auto i = 0; i <z.size(); i++) {
			for (int j = 0; j < z[i].size(); j++) {
				std::cout << z[j][i]<<"\t";
			}
			std::cout << std::endl;
		}
	}

private:
	int rg, rsln,n;
	double length;
	std::string line;
	std::vector<double> x, y,temp;
	std::vector<std::vector<double>> z;
	
	//生成采样网格
	void spawnSamplingGrid() {
		for (int i = 0; i < n; i++) {
			x.push_back(i * length - rg);
		}
		y = x;
	}
	
	//采样点求值
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

//测试用主函数
int main()
{
	std::string line;
	int rg, rsln;
	std::cout << "line:";
	std::getline(std::cin, line);
	std::cout << "rg:";
	std::cin >> rg;
	std::cout << "rsln:";
	std::cin >> rsln;
	GridSampling gdsp(rg, rsln, line);
	gdsp.returnArray();
	gdsp.test();
}