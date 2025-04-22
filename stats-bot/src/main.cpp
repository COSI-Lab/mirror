#include <fstream>
#include <thread>
#include <dpp/dpp.h>
#include <bot.hpp>

int main()
{
    // the bot token is the only content in .env
    std::ifstream env{"/stats-bot/.env"};
    std::string token{};
    env >> token;

    mirror::stats_bot::start(token);
    std::unreachable();
}