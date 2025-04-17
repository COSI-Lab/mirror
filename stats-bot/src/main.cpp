#include <fstream>
#include <thread>
#include <dpp/dpp.h>
#include <bot.hpp>

int main()
{
    std::ifstream env{"/stats-bot/.env"};
    std::string token{};
    env >> token;

    mirror::stats_bot::start(token);
    std::unreachable();
}