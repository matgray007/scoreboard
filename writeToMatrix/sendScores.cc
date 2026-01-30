// Used to send the scores written in 'currentScores.json' to a connected RGB LED matrix display. 

#include <jsoncpp/json/json.h>
#include <fstream>

// Read in the team config JSON file and return the corresponding object
Json::value readTeamConfig() {
    std::string config_path = "../team_config.json";
    std::ifstream file(config_path, std::ifstream::binary);
    Json::Value config;
    Json::CharReaderBuilder builder;
    std::string errs;
    Json::parseFromStream(builder, file, &config, &errs);
    file.close();
    return config;
}


int main(int argc, char *argv[]) {
    // TODO: arg parsing

    // TODO: sigterm handling

    Json::Value config = readTeamConfig(); // Read in the team config file
    // i.e. config["Arizona Cardinals"]["shortName"]

    // TODO: setup rgb matrix

    // TODO: infinite loop to read scores and display them. Pass in config
    // Within this loop, loop through the scores and display each

    // TODO: Cleanup and exit
    return 0;
}
