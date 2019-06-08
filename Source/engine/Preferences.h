#pragma once

#include <string>
#include <cmath>
#include <yaml-cpp/yaml.h>

class Preferences
{
public:
    struct {
        int resolutionX;
        int resolutionY;
        bool maximized;
        bool fullscreen;
    } window;

    struct {
        int resolution;
        int samples;
    } shadows;

    struct {
        bool music;
    } sound;

    struct {
        int startup;
        std::vector<std::string> list;
    } scenes;

    Preferences(std::string prefsFile);
};