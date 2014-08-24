#ifndef LEVEL_HPP
#define LEVEL_HPP

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

struct Link
{
    int r;
    int c;
    int to_r;
    int to_c;
    std::string to_name;
};

struct Switch
{
    enum Action
    {
        TOGGLEDOOR
    };

    Action action;
    int action_data;
    std::string to_name;
    int to_r;
    int to_c;

    int r;
    int c;
};

class Stage
{
    using Tile = int;
    using Row = int*;

    std::vector<Tile> data;
    int width = 0;
    int height = 0;

public:

    std::string name;
    std::vector<Link> links;
    std::vector<Switch> switches;

    void resize(int w, int h);
    Row operator[](std::vector<Tile>::size_type r);

    int getHeight() const;
    int getWidth() const;
};

class Level
{
public:
    std::string name;
    std::unordered_map<std::string, Stage> stages;

    void loadFile(std::string filename);
};

#endif // LEVEL_HPP
