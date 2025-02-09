#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <QVector3D>

enum CameraMoveType
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera
{
public:
    QVector3D m_position; // 摄像机位置
    QVector3D m_target;

    QVector3D m_up; 
    QVector3D m_front; // 摄像机前方
    QVector3D m_right; // 摄像机右边
    QVector3D m_up; // 摄像机上方
    QVector3D m_world_up; // 世界空间的上方
};

#endif