#include <state/state.hpp>

#include <nlohmann/json.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <filesystem>

using json = nlohmann::json;

#define FILE_PATH "data/mirror-metrics-state.json"
#define BACKUP_FILE_PATH "data/mirror-metrics-state.json.bak"

namespace mirror {

    /* ----- Public ----- */

    State::State() {
        this->logger = Logger::getInstance();
        logger->info("Creating application state...");
        std::ifstream f;
        f.open(FILE_PATH);
        if(f.fail()) {
            logger->warn("Failed to load application state from disk.");
            this->last_event = "";
            return;
        }
        json state = json::parse(f);
        state.items();
        f.close();
        this->last_event = state["last_event"].template get<std::string>();
    }

    void State::save() {
        // logger->debug("Saving application state...");
        json j;
        j["last_event"] = last_event;
        std::ofstream f;
        std::rename(FILE_PATH, BACKUP_FILE_PATH);
        f.open(FILE_PATH, std::ofstream::trunc);
        if(f.fail()) {
            logger->error("Failed to save application state.");
        } else {
            f << j;
            f.close();
        }
        // std::stringstream s;
        // s << "Saved state: \"" << j << "\"";
        // logger->debug(s.str());   
    }
}
