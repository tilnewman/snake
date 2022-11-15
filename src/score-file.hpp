#ifndef SNAKE_SCORE_FILE_HPP_INCLUDED
#define SNAKE_SCORE_FILE_HPP_INCLUDED
//
// score-file.hpp
//
#include <cstddef>
#include <string>

//

namespace snake
{

    class ScoreFile
    {
      public:
        void writeHighScore(const int score);
        int readHighScore();

      private:
        const std::string m_fileName{ "score" };
    };

} // namespace snake

#endif // SNAKE_SCORE_FILE_HPP_INCLUDED
