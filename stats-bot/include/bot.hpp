#ifndef BOT_HPP
#define BOT_HPP

#include <string>
#include <dpp/dpp.h>

namespace mirror::stats_bot
{
extern dpp::cluster bot;
void start(std::string token);
void init_logging(dpp::cluster& bot);
void register_commands(dpp::cluster& bot);
void handle_command(dpp::cluster& bot, dpp::slashcommand_t& interaction);
enum class SubscriptionState { SUBSCRIBED, UNSUBSCRIBED };
void set_subscription_state(dpp::cluster& bot, dpp::slashcommand_t& slash, SubscriptionState state);
void listen(dpp::cluster& bot);
void broadcast(dpp::cluster& bot, std::string_view msg);
void stats_thread(dpp::cluster& bot);
std::string get_stats_string();
void sync_listener(dpp::cluster& bot);

}

#endif