// Used to send the scores written in 'currentScores.json' to a connected RGB LED matrix display. 

#include "led-matrix.h"
#include "graphics.h"

#include <jsoncpp/json/json.h>
#include <fstream>
#include <iostream>

#include <signal.h>

using namespace rgb_matrix;

// Global flag for interrupt handling
volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

// Usage function for displaying flags and syntax
static int usage(const char *progname) {
  fprintf(stderr, "usage: %s [options]\n", progname);
  fprintf(stderr, "Formats and displays info contained in ../currentScores.json\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr,
        "\t-f <font-file>    : Use given font.\n"
        "\n"
        );
  rgb_matrix::PrintMatrixFlags(stderr);
  return 1;
}

// Read in the team config JSON file and return the corresponding object
Json::Value readTeamConfig() {
    std::string config_path = "../team_config.json";
    std::ifstream file(config_path, std::ifstream::binary);
    Json::Value config;
    Json::CharReaderBuilder builder;
    std::string errs;
    Json::parseFromStream(builder, file, &config, &errs);
    file.close();
    return config;
}

// Read in the current scores JSON file and return the corresponding object
Json::Value readScores() {
    std::string scores_path = "../currentScores.json";
    std::ifstream file(scores_path, std::ifstream::binary);
    Json::Value scores;
    Json::CharReaderBuilder builder;
    std::string errs;
    Json::parseFromStream(builder, file, &scores, &errs);
    file.close();
    return scores;
}


int main(int argc, char *argv[]) {
    // Arg parsing.
    RGBMatrix::Options matrix_options;
    rgb_matrix::RuntimeOptions runtime_opt;
    if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv,
                                            &matrix_options, &runtime_opt)) {
        return usage(argv[0]);
    }

    // Set up defaults
    const char *large_bdf_font_file = "../matrix/fonts/8x13.bdf";
    const char *medium_bdf_font_file = "../matrix/fonts/5x8.bdf";
    const char *small_bdf_font_file = "../matrix/fonts/4x6.bdf";
    int opt;
    while ((opt = getopt(argc, argv, "")) != -1) {// Within this empty string, add any command line options you want to support. i.e. "a:b:cd" options a and b require an associated value, c and do do not
        switch (opt) {
        default: /* '?' */
            return usage(argv[0]);
        }
    }

    // Load in team config
    Json::Value config = readTeamConfig();
    // i.e. config["Arizona Cardinals"]["shortName"]

    // Load font for displaying text
    rgb_matrix::Font large_font;
    rgb_matrix::Font medium_font;
    rgb_matrix::Font small_font;
    if (!large_font.LoadFont(large_bdf_font_file)) {
        fprintf(stderr, "Couldn't load font '%s'\n", large_bdf_font_file);
        return 1;
    }
    if (!medium_font.LoadFont(medium_bdf_font_file)) {
        fprintf(stderr, "Couldn't load font '%s'\n", medium_bdf_font_file);
        return 1;
    }
    if (!small_font.LoadFont(small_bdf_font_file)) {
        fprintf(stderr, "Couldn't load font '%s'\n", small_bdf_font_file);
        return 1;
    }


    // Setup RGB Matrix
    RGBMatrix *matrix = RGBMatrix::CreateFromOptions(matrix_options, runtime_opt);
    if (matrix == NULL)
        return 1;
    
    // Offscreen canvas for double buffering
    FrameCanvas *offscreen = matrix->CreateFrameCanvas();

    // Signal handling for graceful exit
    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    // Geometry values
    int width = offscreen->width();
    int height = offscreen->height();
    int clock_letter_width = 4;
    int clock_letter_height = 6;
    int team_letter_width = 5; // 5 pixels per letter in medium font
    int team_letter_height = 8;
    int score_letter_width = 8; // 8 pixels per letter in large font
    int score_letter_height = 13;

    int time_x = 22;
    int time_y = 4;
    int period_x = time_x + 8;
    int period_y = 8 + time_y;
    int team1_x = 1;
    int team2_x = offscreen->width() - (3 * 5); // 4 characters max, 6 pixels each
    int team_y = 0;
    int score1_x = team1_x - 1;
    int score2_x = team2_x - 1;
    int score_y = 12;
    

    // Main loop to keep the program running
    while (!interrupt_received) {
        // Read in currentScores.json
        Json::Value current_scores = readScores();

        // Loop through each game in current_scores
        for (const auto& game : current_scores["games"]) {
            if (interrupt_received) {
                break;
            }
            offscreen->Fill(0, 0, 0);

            // Extract game details
            std::string period = game["period"].asString();
            std::string displayClock = game["displayClock"].asString();
            std::string status = game["status"].asString();

            int firstTeamInd = game["competitors"][0]["homeAway"].asString() == "home" ? 1 : 0;
            int secondTeamInd = firstTeamInd == 0 ? 1 : 0;
            std::string firstTeamLong = game["competitors"][firstTeamInd]["displayName"].asString();
            std::string secondTeamLong = game["competitors"][secondTeamInd]["displayName"].asString();
            std::string firstTeam = config[firstTeamLong]["shortName"].asString();
            std::string secondTeam = config[secondTeamLong]["shortName"].asString();
            std::string firstScore = game["competitors"][0]["score"].asString();
            std::string secondScore = game["competitors"][1]["score"].asString();

            Color white(255, 255, 255);
            Color firstPrimaryColor(config[firstTeamLong]["colors"]["primary"][0].asInt(),
                                    config[firstTeamLong]["colors"]["primary"][1].asInt(),
                                    config[firstTeamLong]["colors"]["primary"][2].asInt());
            Color firstSecondaryColor(config[firstTeamLong]["colors"]["secondary"][0].asInt(),
                                        config[firstTeamLong]["colors"]["secondary"][1].asInt(),
                                        config[firstTeamLong]["colors"]["secondary"][2].asInt());
            Color secondPrimaryColor(config[secondTeamLong]["colors"]["primary"][0].asInt(),
                                        config[secondTeamLong]["colors"]["primary"][1].asInt(),
                                        config[secondTeamLong]["colors"]["primary"][2].asInt());
            Color secondSecondaryColor(config[secondTeamLong]["colors"]["secondary"][0].asInt(),
                                        config[secondTeamLong]["colors"]["secondary"][1].asInt(),
                                        config[secondTeamLong]["colors"]["secondary"][2].asInt());



            // Display game details

            // TODO: When team name is 2 letters, add 2 or 3 pixel offset
            // First team name top left
            rgb_matrix::DrawText(offscreen, medium_font,
                           team1_x, 0 + medium_font.baseline(),
                           firstPrimaryColor, NULL, firstTeam.c_str(),
                           0); // The 0s are x, y, and then letter_spacing

            
            // First team score middle left
            rgb_matrix::DrawText(offscreen, large_font,
                           score1_x, score_y + large_font.baseline(),
                           firstSecondaryColor, NULL, firstScore.c_str(),
                           0);

            // TODO: When team name is 2 letters, add 2 or 3 pixel offset
            // Second team name top right
            rgb_matrix::DrawText(offscreen, medium_font,
                           team2_x + (abs(secondTeam.length() - 3) * team_letter_width), team_y + medium_font.baseline(),
                           secondPrimaryColor, NULL, secondTeam.c_str(),
                           0);  // x value adjusted for right alignment when only 2 letter team names
            // Second team score middle left
            rgb_matrix::DrawText(offscreen, large_font,
                           score2_x - ((secondScore.length() - 2) * score_letter_width), score_y + large_font.baseline(),
                           secondSecondaryColor, NULL, secondScore.c_str(),
                           0); // x value adjusted for right alignment
                           
            // Game clock top center
            // Live game
            if (status == "in") {
                // Appends a leading zero if 00:59.9 < time < 10:00 
                if (displayClock.length() == 4 && displayClock.find(":") != std::string::npos) {
                    displayClock = "0" + displayClock;
                }
                // For NBA, when time is 0.0 <= time < 10.0, we need to shift it to the right by one character
                if (displayClock.length() == 3) {
                    rgb_matrix::DrawText(offscreen, small_font,
                            time_x + clock_letter_width, time_y + small_font.baseline(),
                            white, NULL, displayClock.c_str(),
                            0); 
                } else {
                    rgb_matrix::DrawText(offscreen, small_font,
                            time_x, time_y + small_font.baseline(),
                            white, NULL, displayClock.c_str(),
                            0); 
                }
                // Period middle center
                rgb_matrix::DrawText(offscreen, small_font,
                            period_x, period_y + small_font.baseline(),
                            white, NULL, period.c_str(),
                            0);
            } else if (status == "post") { // Ended game
                rgb_matrix::DrawText(offscreen, small_font,
                           time_x, time_y + small_font.baseline(),
                           white, NULL, "FINAL",
                           0);
            }
            
            
            

            
            
            // Double buffer swap
            offscreen = matrix->SwapOnVSync(offscreen);
            usleep(8 * 1000000); // Display for 5 seconds 5000000

        }

    }

    // TODO: Cleanup and exit
    delete matrix;

    std::cout << std::endl;
    return 0;
}
