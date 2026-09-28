#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include <memory>

class Config;

// 全局配置实例。启动时先落一份默认构造的 Config，保证 world.yaml 缺失或解析失败时
// gConfig 依然非空 —— Renderer::init 会无条件解引用它取裁剪面、地形、天空穹参数。
extern std::unique_ptr<Config> gConfig;

#endif // __GLOBALS_H__
