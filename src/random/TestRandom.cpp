#include <iostream>
#include <cstdlib>
#include <ctime>
#include <random>
#include <string>

// C风格的写法
void initRandomNumByCStyle() {
    std::srand(std::time(nullptr));

    int random1;
    random1 = std::rand(); // 0 到 RAND_MAX 之间的随机数
    std::cout << "initRandomNumByCStyle 随机数: " << random1 << std::endl; 

    int random2;
    random2 = std::rand() % 100; // 0~99之间的随机数
    std::cout << "initRandomNumByCStyle 0~99之间的随机数: " << random2 << std::endl; 
}

// C++11 random库
void initRandomNumByCpp(std::string str) {
    // 随机数引擎（使用硬件熵源初始化）
    std::random_device rd;

    // Mersenne Twister 19937 生成器
    std::mt19937 gen(rd());

    if (str == "int") {
        // 均匀分布随机数 0~99
        std::uniform_int_distribution<> dis(0, 99);        
        std::cout << "initRandomNumByCpp 均匀分布随机数: " << dis(gen) << "\n";
    } else if (str == "real") {
        // 浮点数随机数 0.0~3.3
        std::uniform_real_distribution<> dis(0.0, 3.3);        
        std::cout << "initRandomNumByCpp 浮点数随机数: " << dis(gen) << "\n";
    } else {
        // 正太分布随机数 均值0.0 标准差1.0
        std::normal_distribution<> dis(0.0, 1.0);        
        std::cout << "initRandomNumByCpp 正太分布随机数: " << dis(gen) << "\n";
    }
}

