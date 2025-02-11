#include "utils.h"

#include <iostream>

void Utils::DebugMatrix(const QMatrix4x4 &mat)
{
    std::cout << "[" << std::endl;

    for (int i = 0; i < 4; i++)
    {
        std::cout << "  [ " << mat.row(i).x() << "," << mat.row(i).y() << "," << mat.row(i).z() << "," << mat.row(i).w() << " ]" << std::endl;
    }
    std::cout << "]" << std::endl;
}