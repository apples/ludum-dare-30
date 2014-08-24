#include "level.hpp"

#include "meta.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

using namespace std;

void Stage::resize(int w, int h)
{
    data.resize(w*h);
    width = w;
    height = h;
}

Stage::Row Stage::operator[](std::vector<Tile>::size_type r)
{
    return &data[r*width];
}

int Stage::getWidth() const
{
    return width;
}

int Stage::getHeight() const
{
    return height;
}

void Level::loadFile(std::string filename)
{
    logger->log("Loading level: ", filename);

    std::ifstream in (filename.c_str());

    getline(in, name);

    string line;
    string word;

    while (getline(in, line))
    {
        istringstream ss (line);

        ss >> word;

        if (word == "stage")
        {
            Stage stg;
            string stgname;
            int w;
            int h;

            ss >> stgname >> w >> h;
            stg.name = stgname;

            logger->log("Found stage ", stg.name);

            stg.resize(w, h);
            for (int r=0; r<h; ++r)
            {
                getline(in, line);
                istringstream sst (line);
                for (int c=0; c<w; ++c)
                {
                    sst >> stg[r][c];
                }
            }

            stages[stgname] = move(stg);
        }
        else if (word == "link")
        {
            Link lnk1;
            Link lnk2;

            ss >> lnk2.to_name >> lnk1.r >> lnk1.c >> lnk1.to_name >> lnk1.to_r >> lnk1.to_c;
            lnk2.r = lnk1.to_r;
            lnk2.c = lnk1.to_c;
            lnk2.to_r = lnk1.r;
            lnk2.to_c = lnk1.c;

            logger->log("Found link from ", lnk2.to_name, " to ", lnk1.to_name);

            stages.at(lnk2.to_name).links.push_back(lnk1); // Don't move!
            stages.at(lnk1.to_name).links.push_back(lnk2);
        }
        else if (word == "switch")
        {
            Switch swtch;

            std::string stgname;
            std::string actname;

            ss >> stgname >> swtch.r >> swtch.c >> actname;

            if (actname == "toggledoor")
            {
                swtch.action = Switch::TOGGLEDOOR;
                ss >> swtch.to_name >> swtch.to_r >> swtch.to_c;
            }

            stages.at(stgname).switches.push_back(move(swtch));
        }
        else
        {
            throw runtime_error("Invalid level file!");
        }
    }
}
