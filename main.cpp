#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>
#include <set>
#include <math.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#define INPUT "input.txt"
#define MAX_ITERATION 1e3
#define MAX_SAMPLE 2e4

float p; // Величина допустимого отклонения точек от плоскости
float size; // Количество точек

// Структура для хранения точек
struct Point {
    float x, y, z;
};

std::vector<Point> OriginPoints; // Список заданных точек
std::set<size_t> planeHashSet; // Множество для хранения хэшей случайно сгенерированных плоскостей

// Структура для хранения плоскости
struct Plane {
    float a, b, c, d;
    // Функция вычисления расстояния от точки до плоскости
    inline float distance(Point p) {
        return (a*p.x + b*p.y + c*p.z + d) / sqrt(a*a + b*b + c*c);
    }
};

void ReadPoints()
{
    std::string line;
    std::ifstream inputStream(INPUT); // Открываем файл для чтения

    if (inputStream.is_open()) {
        // Считывание параметра p
        std::getline(inputStream, line);
        p = std::stod(line);
        // Считывание количества точек
        std::getline(inputStream, line);
        size = std::stod(line);

        Point currentPoint;

        // Считывание точек
        for (int i = 0; i < size; i++) {
            std::getline(inputStream, line); // Считываем всю строку

            std::string separator("\t"); // Определение разделителя
            int prev = 0, next = 0;

            // Ищем в строке разделитель
            while ((next = line.find(separator, prev)) != std::string::npos) {
                // Вырезаем из строки кусок до найденного разделителя
                std::string tmp = line.substr(prev, next-prev);
                if (prev == 0)
                    currentPoint.x = std::stod(tmp); // Преобразуем и считываем координату x
                else
                    currentPoint.y = std::stod(tmp); // Преобразуем и считываем координату y
                prev = next + separator.length(); // Запоминаем положение, где был найден разделитель
            }
            // Вырезаем из строки оставшуюся после поиска часть
            std::string tmp = line.substr(prev, line.length());
            currentPoint.z = std::stod(tmp); // Преобразуем и считываем координату z

            OriginPoints.push_back(currentPoint); // Добавляем в массив точку со считанными координатами
        }
        inputStream.close(); // Закрываем поток чтения

        // Начало диагностики
        /*
        std::cout << "p == " << p << std::endl;
        std::cout << "size == " << size << std::endl;

        for (int i = 0; i < OriginPoints.size(); i++)
            std::cout << OriginPoints[i].x << "\t" << OriginPoints[i].y << "\t" <<  OriginPoints[i].z << std::endl;
        */
        // Конец диагностики
    }
    else {
        std::cout << "Can't open input file" << std::endl;
    }
}

Plane createRandomPlane()
{
    Plane randomPlane;
    float normal_length = 0; // Величина нормали сгенерированной плоскости

    // Цикл генерации случайной плоскости
    while (true) {
        // Вектор для хранения сгенерированных индексов
        std::vector<int> index(3);

        // Генерация случайных индексов точек из списка исходных точек
        for (uint8_t i = 0; i < 3; i++) {
            index[i] = rand() % OriginPoints.size();
        }

        // Упорядочивание вектора со случайно сгенерированными индексами
        std::sort(index.begin(), index.end());
        // Получаем случайные точки по сгенерированным индексам
        const Point p1 = OriginPoints[index[0]];
        const Point p2 = OriginPoints[index[1]];
        const Point p3 = OriginPoints[index[2]];

        // Вычисляем параметры для сгенерированной плоскости
        randomPlane.a = p1.y * (p2.z - p3.z) + p2.y * (p3.z - p1.z) + p3.y * (p1.z - p2.z);
        randomPlane.b = p1.z * (p2.x - p3.x) + p2.z * (p3.x - p1.x) + p3.z * (p1.x - p2.x);
        randomPlane.c = p1.x * (p2.y - p3.y) + p2.x * (p3.y - p1.y) + p3.x * (p1.y - p2.y);
        randomPlane.d = -p1.x * (p2.y * p3.z - p3.y * p2.z) - p2.x * (p3.y * p1.z - p1.y * p3.z) - p3.x * (p1.y * p2.z - p2.y * p1.z);
        // Вычисляем длину нормали сгенерированной плоскости
        normal_length = sqrt(randomPlane.a * randomPlane.a + randomPlane.b * randomPlane.b + randomPlane.c * randomPlane.c);

        // Проверка на вырожденность плоскости
        if (normal_length < 1e-6)
            continue;

        break;
    }

    // Нормализуем коэффициенты сгенерированной плоскости
    randomPlane.a /= normal_length;
    randomPlane.b /= normal_length;
    randomPlane.c /= normal_length;
    randomPlane.d /= normal_length;

    return randomPlane;
}

Plane CalculatePlane()
{

    Plane result; // Найденная плоскость
    unsigned long max = 0; // Максимальное количество точек, достаточно близко расположенных к плоскости
    uint16_t sample = OriginPoints.size() / 2;

    if (sample > MAX_SAMPLE) {
        sample = MAX_SAMPLE;
    }

    //for (int j = 0; j < MAX_ITERATION; j++) {
    Plane candidatePlane;

    uint16_t goodPoints = 0;
    uint16_t counter = 0;

    // Проверка половины исходных точек (50% по условию принадлежит дороге)

    while (counter < ceil(OriginPoints.size()/2)) {
        // генерация плоскости и точек для проверки
        candidatePlane = createRandomPlane();

        std::vector<uint16_t> distanceIndex; // Вектор для хранения проверенных точек

        for (uint16_t i = 0; i < sample; i++) {
            // Генерация случайного индекса точки для проверки расстояния до неё
            uint16_t index = std::rand() % OriginPoints.size();
            // Проверка на дублирование точек
            if (fabs(candidatePlane.distance(OriginPoints[index])) < p) {
                goodPoints++;
            }
        } // end for
        // Проверка, что найденная плоскость - оптимальная по количеству точек, принадлежащих ей
        if (goodPoints > max) {
            result = candidatePlane;
            max = goodPoints;
        }
        goodPoints = 0;
        counter++;
    } // end while

    return result;
}

int main(int, const char**)
{
    // Считывание параметров и точек из файла
    ReadPoints();
    // Инициализация генератора рандомных чисел
    srand(time(NULL));

    Plane result = CalculatePlane();

    // std::cout << result.a << " " << result.b << " " << result.c << " " << result.d << std::endl;
    printf("%10.6f\t%10.6f\t%10.6f\t%10.6f\n", result.a, result.b, result.c, result.d);

    return 0;
}
