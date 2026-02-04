    #ifndef GAMEDETAILS_H
    #define GAMEDETAILS_H

    #include <string>
    #include "led-matrix.h"
    #include "graphics.h"
    #include <jsoncpp/json/json.h>

    using rgb_matrix::Color;

    struct GameDetails {
        std::string period;
        std::string displayClock;
        std::string status;
        std::string firstTeam;
        std::string secondTeam;
        std::string firstScore;
        std::string secondScore;
        Color firstPrimaryColor;
        Color firstSecondaryColor;
        Color secondPrimaryColor;
        Color secondSecondaryColor;
        Color white = Color(255, 255, 255);
    };

    GameDetails extractGameDetails(const Json::Value& game, const Json::Value& config);

    #endif // GAMEDETAILS_H