// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "score-file.hpp"

#include "check-macros.hpp"

#include <fstream>

namespace snake
{

    void ScoreFile::writeHighScore(const int score)
    {
        try
        {
            std::ofstream fStream(m_fileName, std::ios_base::trunc);

            M_CHECK_SS(
                ((fStream.is_open()) && fStream.good()),
                "Failed to open high score file for writing: \"" << m_fileName << "\"");

            fStream << score;

            M_CHECK_SS(
                ((fStream.is_open()) && fStream.good()),
                "Failed to write high score to file: \"" << m_fileName << "\"");
        }
        catch (...)
        {
            M_LOG_SS("Failed to save high score into file: \"" << m_fileName << "");
        }
    }

    int ScoreFile::readHighScore()
    {
        try
        {
            std::ifstream fStream(m_fileName);

            if (!fStream.is_open() || !fStream.good())
            {
                return 0;
            }

            std::string highScoreStr;
            std::getline(fStream, highScoreStr);

            if (highScoreStr.empty())
            {
                return 0;
            }

            return std::stoi(highScoreStr);
        }
        catch (...)
        {
            M_LOG_SS("Failed to load high score from file: \"" << m_fileName << "");
            return 0;
        }
    }

} // namespace snake
