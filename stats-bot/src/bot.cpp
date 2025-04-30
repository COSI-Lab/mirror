#include <bot.hpp>
#include <stats.hpp>

#include <string>
#include <vector>
#include <iostream>
#include <numeric>

#include <dpp/dpp.h>
#include <spdlog/spdlog.h>
#include <zmq.hpp>
#include <dpp/appcommand.h>

constexpr std::string_view CHANNELS_FILE = "/stats-bot/channels.txt";
constexpr int MANUAL_SYNC_PORT = 9281;

void mirror::stats_bot::start(std::string token)
{
    dpp::cluster bot{token};
    init_logging(bot);

    bot.on_ready([&bot] (const dpp::ready_t& event)
    {
        register_commands(bot);
    });

    bot.on_slashcommand([&bot](dpp::slashcommand_t command)
    {
        handle_command(bot, command);
    });

    std::thread stats{stats_thread, std::ref(bot)};
    stats.detach();

    std::thread sync{sync_listener, std::ref(bot)};
    sync.detach();

    spdlog::info("Starting bot");
    bot.start(dpp::start_type::st_wait);
}

void mirror::stats_bot::init_logging(dpp::cluster& bot)
{
    bot.on_log([&bot](const dpp::log_t& event) {
        switch(event.severity)
        {
        case dpp::loglevel::ll_trace:
            spdlog::trace(event.message);
            break;
        case dpp::loglevel::ll_debug:
            spdlog::debug(event.message);
            break;
        case dpp::loglevel::ll_info:
            spdlog::info(event.message);
            break;
        case dpp::loglevel::ll_warning:
            spdlog::warn(event.message);
            break;
        case dpp::loglevel::ll_error:
            spdlog::error(event.message);
            break;
        case dpp::loglevel::ll_critical:
            spdlog::critical(event.message);
            break;
        }
    });
}

void mirror::stats_bot::register_commands(dpp::cluster& bot)
{
    dpp::slashcommand stats{"stats", "Subscribe to daily mirror statistics updates", bot.me.id};
    stats.add_option(
        dpp::command_option{
            dpp::co_sub_command,
            "subscribe",
            "Subscribe to daily statistics updates"
        }
    );
    stats.add_option(
        dpp::command_option{
            dpp::co_sub_command,
            "unsubscribe",
            "Unsubscribe from daily statistics updates"
        }
    );
    stats.add_option(
        dpp::command_option{
            dpp::co_sub_command,
            "view",
            "View current statistics (starting midnight UTC)"
        }
    );
    
    bot.global_command_create(stats, [](const dpp::confirmation_callback_t& callback)
    {
        if(callback.is_error())
        {
            spdlog::error("Failed creating global command.");
        }
        else
        {
            spdlog::info("Created commands");
        }
    });
}

void mirror::stats_bot::handle_command(dpp::cluster& bot, dpp::slashcommand_t& slash)
{
    dpp::interaction interaction = slash.command;
    std::string command_name = interaction.get_command_name();
    if(command_name == "stats")
    {
        std::string subcommand_name = interaction.get_command_interaction().options[0].name;
        if(subcommand_name == "subscribe" || subcommand_name == "unsubscribe")
        {
            set_subscription_state(bot, slash, (subcommand_name == "subscribe") ? SubscriptionState::SUBSCRIBED : SubscriptionState::UNSUBSCRIBED);
        } else if(subcommand_name == "view")
        {
            slash.reply(get_stats_string());
        }
    }

}

std::vector<std::string> get_subscribed_channels()
{
    std::ifstream file{std::string{CHANNELS_FILE}};
    std::vector<std::string> channels{};
    std::string channel;
    while(std::getline(file, channel)) {
        channels.push_back(channel);
    }
    file.close();
    return channels;
}

bool is_subscribed(std::string channel_id)
{
    std::vector<std::string> channels = get_subscribed_channels();
    for(int i = 0; i < channels.size(); i++) {
        if(channels[i] == channel_id) return true;
    }
    return false;
}

void add_channel(std::string channel_id)
{
    std::ofstream file{std::string{CHANNELS_FILE}, std::ios::app};
    file << channel_id << "\n";
    file.close();
}

void remove_channel(std::string channel_id)
{
    std::vector<std::string> channels = get_subscribed_channels();
    std::ofstream file{std::string{CHANNELS_FILE}};
    for(int i = 0; i < channels.size(); i++) {
        if(channels[i] != channel_id) {
            file << channels[i] << "\n";
        }
    }
}

// let's reinvent the wheel
void mirror::stats_bot::set_subscription_state(dpp::cluster& bot, dpp::slashcommand_t& slash, SubscriptionState state)
{
    std::string channel_id = slash.command.get_channel().id.str();
    
    if(state == SubscriptionState::SUBSCRIBED)
    {
        if(is_subscribed(channel_id))
        {
            slash.reply("This channel is already subscribed to statistics updates.");
        }
        else
        {
            add_channel(channel_id);
            slash.reply("This channel is now subscribed to statistics updates.");
        }
    }
    else if(state == SubscriptionState::UNSUBSCRIBED)
    {
        if(!is_subscribed(channel_id))
        {
            slash.reply("This channel is not receiving statistics updates.");
        }
        else
        {
            remove_channel(channel_id);
            slash.reply("This channel will no longer receive statistics updates.");
        }
    }
}

void mirror::stats_bot::broadcast(dpp::cluster& bot, std::string_view msg)
{
    std::vector<std::string> channels = get_subscribed_channels();

    for(int i = 0; i < channels.size(); i++) {
        bot.message_create(
            dpp::message{
                dpp::snowflake{channels[i]},
                msg
            }
        );
    }
}

void mirror::stats_bot::stats_thread(dpp::cluster& bot)
{
    while(true)
    {

        std::this_thread::sleep_until(
            std::chrono::ceil<std::chrono::days>
            (std::chrono::system_clock::now()));
        broadcast(bot, get_stats_string());
    }
}

std::string mirror::stats_bot::get_stats_string()
{
    spdlog::info("Pulling statistics...");
        
    std::vector<StatsEntry> stats = get_stats();

    if(stats.empty()) return "No projects were accessed :(";

    auto top_project = std::max_element(stats.begin(), stats.end());
    long total_transfer = std::accumulate(stats.begin(), stats.end(), 0);

    return std::string{"A total of "} + std::to_string(total_transfer) +
        " bytes were transferred. The most popular project was " +
        top_project->name + " (" +
        std::to_string(top_project->bytes_transferred) + ").";
}

void mirror::stats_bot::sync_listener(dpp::cluster& bot)
{
    zmq::context_t context{};
    zmq::socket_t socket{context, zmq::socket_type::rep};
    socket.bind("tcp://*:" + std::to_string(MANUAL_SYNC_PORT));

    while(true)
    {
        zmq::message_t request;
        auto res = socket.recv(request);
        socket.send(zmq::message_t{"okay!"});
        // if(res)
        std::string sync_info = "Manual sync requested for `" + request.to_string() + "`.";
        broadcast(bot, std::string_view{sync_info});
    }
}
