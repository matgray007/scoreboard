// Used to send the scores written in 'currentScores.json' to a connected RGB LED matrix display. 

#include "led-matrix.h"
#include "gameDetails.h"

#include <jsoncpp/json/json.h>
#include <fstream>
#include <iostream>

#include "graphics.h"
#include <signal.h>
#include <Magick++.h>
#include <magick/image.h>
#include <curl/curl.h>
#include <vector>

using namespace rgb_matrix;
// CONSTANTS
// Geometry values
int width = 64;
int height = 32;


int team1_x = 1;
int team2_x = width - (3 * 5); // 4 characters max, 6 pixels each
int team_y = 8;
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
int score1_x = team1_x - 1;
int score2_x = team2_x - 1;
int score_y = height / 2 + 4;

// Helper function to return all relevant details about a game
GameDetails extractGameDetails(const Json::Value& game, const Json::Value& config) {
    GameDetails details;
    details.period = game["period"].asString();
    details.displayClock = game["displayClock"].asString();
    details.status = game["status"].asString();

    int firstTeamInd = game["competitors"][0]["homeAway"].asString() == "home" ? 1 : 0;
    int secondTeamInd = firstTeamInd == 0 ? 1 : 0;
    std::string firstTeamLong = game["competitors"][firstTeamInd]["displayName"].asString();
    std::string secondTeamLong = game["competitors"][secondTeamInd]["displayName"].asString();
    details.firstTeam = config[firstTeamLong]["shortName"].asString();
    details.secondTeam = config[secondTeamLong]["shortName"].asString();
    details.firstScore = game["competitors"][firstTeamInd]["score"].asString();
    details.secondScore = game["competitors"][secondTeamInd]["score"].asString();

    details.firstPrimaryColor = Color(config[firstTeamLong]["colors"]["primary"][0].asInt(),
                                      config[firstTeamLong]["colors"]["primary"][1].asInt(),
                                      config[firstTeamLong]["colors"]["primary"][2].asInt());
    details.firstSecondaryColor = Color(config[firstTeamLong]["colors"]["secondary"][0].asInt(),
                                        config[firstTeamLong]["colors"]["secondary"][1].asInt(),
                                        config[firstTeamLong]["colors"]["secondary"][2].asInt());
    details.secondPrimaryColor = Color(config[secondTeamLong]["colors"]["primary"][0].asInt(),
                                       config[secondTeamLong]["colors"]["primary"][1].asInt(),
                                       config[secondTeamLong]["colors"]["primary"][2].asInt());
    details.secondSecondaryColor = Color(config[secondTeamLong]["colors"]["secondary"][0].asInt(),
                                         config[secondTeamLong]["colors"]["secondary"][1].asInt(),
                                         config[secondTeamLong]["colors"]["secondary"][2].asInt());
    return details;
}

// Copied from led-image-viewer.cc (not present in header files)
using ImageVector = std::vector<Magick::Image>;

// Given the filename, load the image and scale to the size of the
// matrix.
// // If this is an animated image, the resutlting vector will contain multiple.
static ImageVector LoadImageAndScaleImage(const char *filename,
                                          int target_width,
                                          int target_height) {
  ImageVector result;

  ImageVector frames;
  try {
    readImages(&frames, filename);
  } catch (std::exception &e) {
    if (e.what())
      fprintf(stderr, "%s\n", e.what());
    return result;
  }

  if (frames.empty()) {
    fprintf(stderr, "No image found.");
    return result;
  }

  // Animated images have partial frames that need to be put together
  if (frames.size() > 1) {
    Magick::coalesceImages(&result, frames.begin(), frames.end());
  } else {
    result.push_back(frames[0]); // just a single still image.
  }

  for (Magick::Image &image : result) {
    image.scale(Magick::Geometry(target_width, target_height));
  }

  return result;
}


// Copied from led-image-viewer.cc (not present in header files) and then altered
// Copy an image to a Canvas. Note, the RGBMatrix is implementing the Canvas
// interface as well as the FrameCanvas we use in the double-buffering of the
// animted image.
void CopyImageToCanvas(const Magick::Image &image, Canvas *canvas, int offset_x = 0, int offset_y = 0) {
  // Copy all the pixels to the canvas.
  for (size_t y = 0; y < image.rows(); ++y) {
    for (size_t x = 0; x < image.columns(); ++x) {
      const Magick::Color &c = image.pixelColor(x, y);
      if (c.alphaQuantum() < 65535  * 0.5) { // Changed from 256
        canvas->SetPixel(x + offset_x, y + offset_y,
                         ScaleQuantumToChar(c.redQuantum()),
                         ScaleQuantumToChar(c.greenQuantum()),
                         ScaleQuantumToChar(c.blueQuantum()));
      }
    }
  }
}

void CopyHalfImage(const Magick::Image &image, Canvas *canvas, int offset_x = 0, int offset_y = 0, bool left_half = true) {
    // Copy the left or right half of the image to the canvas.
    size_t start_x = left_half ? 0 :  2 * image.columns() / 5 + 1;
    size_t end_x = left_half ? 3 * image.columns() / 5 : image.columns();
    for (size_t y = 0; y < image.rows(); ++y) {
        for (size_t x = start_x; x < end_x; ++x) {
            const Magick::Color &c = image.pixelColor(x, y);
            if (c.alphaQuantum() < 65535  * 0.5) { // Changed from 256
            canvas->SetPixel(x + offset_x - start_x, y + offset_y,
                                ScaleQuantumToChar(c.redQuantum()),
                                ScaleQuantumToChar(c.greenQuantum()),
                                ScaleQuantumToChar(c.blueQuantum()));
            }
        }
    }
}

size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    std::vector<uint8_t>* buffer = static_cast<std::vector<uint8_t>*>(userp);
    buffer->insert(buffer->end(), (uint8_t*)contents, (uint8_t*)contents + total_size);
    return total_size;
}

Magick::Image load_image_from_url(const std::string& url) {
    // Download image data
    std::vector<uint8_t> buffer;
    CURL* curl = curl_easy_init();
    
    if (!curl) {
        throw std::runtime_error("Failed to initialize curl");
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // For HTTPS
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK || buffer.empty()) {
        throw std::runtime_error("Failed to download image");
    }
    
    // Load from memory blob into Magick::Image
    Magick::Blob blob(buffer.data(), buffer.size());
    Magick::Image image(blob);
    
    return image;
}

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

Json::Value readConfig() {
    std::string config_path = "../config.json";
    std::ifstream file(config_path, std::ifstream::binary);
    Json::Value config;
    Json::CharReaderBuilder builder;
    std::string errs;
    Json::parseFromStream(builder, file, &config, &errs);
    file.close();
    return config;
}

// Read in the current scores JSON file and return the corresponding object
Json::Value readScores(bool favorite_only = false) {
    std::string scores_path = "../currentScores.json";
    std::ifstream file(scores_path, std::ifstream::binary);
    Json::Value scores;
    Json::CharReaderBuilder builder;
    std::string errs;
    Json::parseFromStream(builder, file, &scores, &errs);
    file.close();
    return scores;
}

// Given a team name and overall config, retrieve the path to the corresponding logo file
std::string retrieveLogoPath(std::string teamName, Json::Value overallConfig) {
    std::string temp = teamName;
    std::transform(temp.begin(), temp.end(), temp.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    std::string logoPath =
        "../logos/" + overallConfig["league"].asString() + "/" + temp + ".png";
    return logoPath;
}

// Writes the scores of the two teams on the bottom left and bottom right side of the matrix
void writeScores(FrameCanvas *offscreen, GameDetails details,
                rgb_matrix::Font &large_font) {
    // First team score bottom left
    if (details.firstScore.length() == 1) {
        details.firstScore = "0" + details.firstScore;
    }
    if (details.secondScore.length() == 1) {
        details.secondScore = "0" + details.secondScore;
    }
    rgb_matrix::DrawText(offscreen, large_font,
                    score1_x, score_y + large_font.baseline(),
                    details.firstSecondaryColor, NULL, details.firstScore.c_str(),
                    0);
    // Second team score middle left
    rgb_matrix::DrawText(offscreen, large_font,
                    score2_x - ((details.secondScore.length() - 2) * score_letter_width), score_y + large_font.baseline(),
                    details.secondSecondaryColor, NULL, details.secondScore.c_str(),
                    0); // x value adjusted for right alignment
}

// Writes the scores of the two teams on the bottom middle of the matrix
void writeSmallScores(FrameCanvas *offscreen, GameDetails details,
                rgb_matrix::Font &font) {
    // First team score bottom left
    if (details.firstScore.length() == 1) {
        details.firstScore = "0" + details.firstScore;
    }
    if (details.secondScore.length() == 1) {
        details.secondScore = "0" + details.secondScore;
    }
    // First team score
    rgb_matrix::DrawText(offscreen, font,
                    width / 2 - details.firstScore.length() * team_letter_width - 2, height - team_letter_height - 1 + font.baseline(),
                    details.firstSecondaryColor, NULL, details.firstScore.c_str(),
                    0);
    // Second team score
    rgb_matrix::DrawText(offscreen, font,
                    width / 2 + 2, height - team_letter_height - 1 + font.baseline(),
                    details.secondSecondaryColor, NULL, details.secondScore.c_str(),
                    0); // x value adjusted for right alignment
    offscreen->SetPixel(width / 2 - 2, height - team_letter_height - (team_letter_height / 2) - 1 + font.baseline(), 
            details.white.r, details.white.g, details.white.b);
    offscreen->SetPixel(width / 2 - 1, height - team_letter_height - (team_letter_height / 2) - 1 + font.baseline(), 
                           details.white.r, details.white.g, details.white.b);
    offscreen->SetPixel(width / 2, height - team_letter_height - (team_letter_height / 2) - 1 + font.baseline(), 
                           details.white.r, details.white.g, details.white.b);
    
    
}


// Writes the game time and period in the center of the matrix
void writeTimes(FrameCanvas *offscreen, GameDetails details,
                rgb_matrix::Font &small_font) {
    // Game clock top center
    // Live game
    if (details.status == "in") {
        // Appends a leading zero if 00:59.9 < time < 10:00 
        if (details.displayClock.length() == 4 && details.displayClock.find(":") != std::string::npos) {
            details.displayClock = "0" + details.displayClock;
        }
        // For NBA, when time is 0.0 <= time < 10.0, we need to shift it to the right by one character
        if (details.displayClock.length() == 3) {
            rgb_matrix::DrawText(offscreen, small_font,
                    time_x + clock_letter_width, time_y + small_font.baseline(),
                    details.white, NULL, details.displayClock.c_str(),
                    0); 
        } else {
            rgb_matrix::DrawText(offscreen, small_font,
                    time_x, time_y + small_font.baseline(),
                    details.white, NULL, details.displayClock.c_str(),
                    0); 
        }
        // Period middle center
        rgb_matrix::DrawText(offscreen, small_font,
                    period_x, period_y + small_font.baseline(),
                    details.white, NULL, details.period.c_str(),
                    0);
    } else if (details.status == "post") { // Ended game
        rgb_matrix::DrawText(offscreen, small_font,
                    time_x, time_y + small_font.baseline(),
                    details.white, NULL, "FINAL",
                    0);
    }
}

void drawField(FrameCanvas *offscreen, GameDetails details) {
    // This will be replaced with 

    // White for where the team has already been
    rgb_matrix::DrawLine(offscreen, 27, height - 1, 36, height - 1, details.white);
    rgb_matrix::DrawLine(offscreen, 27, height - 1, 36, height - 1, details.white);


}



// Cycle through games in currentScores.json and display them in a traditional scoreboard format
void writeScoreboard(RGBMatrix *matrix, FrameCanvas *offscreen, Json::Value config,
                 rgb_matrix::Font &large_font, rgb_matrix::Font &medium_font,
                 rgb_matrix::Font &small_font, bool favorite_only) {
    

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
            GameDetails details = extractGameDetails(game, config);

            // Display game details

            // Write times in center
            writeTimes(offscreen, details, small_font);

            // Write scores on bottom left and bottom right
            writeScores(offscreen, details, large_font);

            // First team name top left-ish
            rgb_matrix::DrawText(offscreen, medium_font,
                           team1_x, team_y + medium_font.baseline(),
                           details.firstPrimaryColor, NULL, details.firstTeam.c_str(),
                           0); // The 0s are x, y, and then letter_spacing

            // Second team name top right-ish
            rgb_matrix::DrawText(offscreen, medium_font,
                           team2_x + (std::abs(static_cast<int>(details.secondTeam.length() - 3)) * team_letter_width), team_y + medium_font.baseline(),
                           details.secondPrimaryColor, NULL, details.secondTeam.c_str(),
                           0);  // x value adjusted for right alignment when only 2 letter team names
            
            // Double buffer swap
            offscreen = matrix->SwapOnVSync(offscreen);
            usleep(8 * 1000000); // Display for 5 seconds 5000000

        }

    }
}

// Cycle through games in currentScores.json and display them with team logos
void writeLogos(RGBMatrix *matrix, FrameCanvas *offscreen, Json::Value config,
                 rgb_matrix::Font &large_font, rgb_matrix::Font &medium_font,
                 rgb_matrix::Font &small_font, bool favorite_only) {
    // Geometry values
    int width = offscreen->width();
    int height = offscreen->height();
    Json::Value overallConfig = readConfig();
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
            GameDetails details = extractGameDetails(game, config);
            
            // Draw times in center
            writeTimes(offscreen, details, small_font);

            // Write scores on bottom left and bottom right
            writeScores(offscreen, details, large_font);

            // Draw the field
            // drawField(offscreen, details);

            // Retrieve logo file names
            std::string firstLogo = retrieveLogoPath(details.firstTeam, overallConfig);
            std::string secondLogo = retrieveLogoPath(details.secondTeam, overallConfig);

            // Display logos
            ImageVector firstImage = LoadImageAndScaleImage(firstLogo.c_str(),
                                              height / 2,
                                              height / 2);
            ImageVector secondImage = LoadImageAndScaleImage(secondLogo.c_str(),
                                              height / 2,
                                              height / 2);
            if (firstImage.size() > 0 && secondImage.size() > 0) {
                CopyImageToCanvas(firstImage[0], offscreen, 1, 1);
                CopyImageToCanvas(secondImage[0], offscreen, width - (height / 2) - 1, 1);
                offscreen = matrix->SwapOnVSync(offscreen);
                usleep(8 * 1000000); // Display for 5 seconds 5000000
            }
        }
    }
}


// Cycle through games in currentScores.json and display them with the logos large
void writeLargeLogos(RGBMatrix *matrix, FrameCanvas *offscreen, Json::Value config,
                 rgb_matrix::Font &large_font, rgb_matrix::Font &medium_font,
                 rgb_matrix::Font &small_font, bool favorite_only) {
    // Geometry values
    int width = offscreen->width();
    int height = offscreen->height();
    Json::Value overallConfig = readConfig();
    while (!interrupt_received) {
        // Read in currentScores.json
        Json::Value current_scores = readScores();
        // Loop through each game in current_scores
        for (const auto& game : current_scores["games"]) {
            if (interrupt_received) {
                break;
            }
            // Extract game details
            GameDetails details = extractGameDetails(game, config);
            if (favorite_only) {
                if (details.firstTeam != overallConfig["favoriteTeam"][overallConfig["league"].asString()].asString() &&
                    details.secondTeam != overallConfig["favoriteTeam"][overallConfig["league"].asString()].asString()) {
                    continue;
                }
            }

            offscreen->Fill(0, 0, 0);
            
            // Draw times in center
            writeTimes(offscreen, details, small_font);

            // Write scores on bottom left and bottom right
            writeSmallScores(offscreen, details, medium_font);

            // Draw the field
            // drawField(offscreen, details);

            // Retrieve logo file names
            std::string firstLogo = retrieveLogoPath(details.firstTeam, overallConfig);
            std::string secondLogo = retrieveLogoPath(details.secondTeam, overallConfig);


            // Display logos
            ImageVector firstImage = LoadImageAndScaleImage(firstLogo.c_str(),
                                              height,
                                              height);
            ImageVector secondImage = LoadImageAndScaleImage(secondLogo.c_str(),
                                              height,
                                              height);
            if (firstImage.size() > 0 && secondImage.size() > 0) {
                CopyHalfImage(firstImage[0], offscreen, 0, 0, false);
                CopyHalfImage(secondImage[0], offscreen, width - (3 * height / 5), 0, true);
                offscreen = matrix->SwapOnVSync(offscreen);
                usleep(8 * 1000000); // Display for 5 seconds 5000000
            }



            

        }
    }
}

// Spotify mode
void writeSpotify(RGBMatrix *matrix, FrameCanvas *offscreen, Json::Value config,
                 rgb_matrix::Font &large_font, rgb_matrix::Font &medium_font,
                 rgb_matrix::Font &small_font) {
    int scrolling_speed = 3; // Adjust this value to increase/decrease scrolling speed (letters per second)
    int delay_speed_usec = 1000000 / scrolling_speed / medium_font.CharacterWidth('W');

    while (!interrupt_received) {
        offscreen->Fill(0, 0, 0);
        Json::Value current_scores = readScores();
        if (!current_scores.empty() && current_scores["currently_playing"]["song"].asString() != "") {
            std::string song = current_scores["currently_playing"]["song"].asString();
            std::string artist = current_scores["currently_playing"]["artist"].asString();
            std::string album_art = current_scores["currently_playing"]["album_art"].asString();
            Magick::Image image = load_image_from_url(album_art);
            image.scale(Magick::Geometry(height, height));
            int x = width;
            int song_length = 0;
            int artist_length = 0;
            

            while (!interrupt_received && (--x + song_length >= (width / 2) || x + artist_length >= (width / 2))) {
                offscreen->Fill(0, 0, 0);
                song_length = rgb_matrix::DrawText(offscreen, medium_font,
                           x, 1 + medium_font.baseline(),
                           Color(255, 255, 255), NULL, song.c_str(),
                           0);
                artist_length = rgb_matrix::DrawText(offscreen, medium_font,
                           x, height / 2 + 1 + medium_font.baseline(),
                           Color(255, 255, 255), NULL, artist.c_str(),
                           0);
                CopyImageToCanvas(image, offscreen);
                offscreen = matrix->SwapOnVSync(offscreen);
                usleep(delay_speed_usec);
            }
        }
    }
        
}


int main(int argc, char *argv[]) {
    std::string mode = "logos";
    // Arg parsing.
    RGBMatrix::Options matrix_options;
    rgb_matrix::RuntimeOptions runtime_opt;
    if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv,
                                            &matrix_options, &runtime_opt)) {
        return usage(argv[0]);
    }

    // Set up defaults
    const char *large_bdf_font_file = "../matrix/fonts/8x13B.bdf";
    const char *medium_bdf_font_file = "../matrix/fonts/5x8.bdf";
    const char *small_bdf_font_file = "../matrix/fonts/4x6.bdf";
    int opt;
    bool favorite_only = false;
    while ((opt = getopt(argc, argv, "d:o")) != -1) {// Within this empty string, add any command line options you want to support. i.e. "a:b:cd" options a and b require an associated value, c and do do not
        switch (opt) {
        case 'd':
            mode = optarg;
            break;
        case 'o':
            favorite_only = true;
            break;
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

    // writeScoreboard(matrix, offscreen, config, large_font, medium_font, small_font);
    if (mode == "scoreboard") {
        writeScoreboard(matrix, offscreen, config, large_font, medium_font, small_font, favorite_only);
    } else if (mode == "logos") {
        Magick::InitializeMagick(*argv);
        writeLogos(matrix, offscreen, config, large_font, medium_font, small_font, favorite_only);
    } else if (mode == "large-logos") {
        Magick::InitializeMagick(*argv);
        writeLargeLogos(matrix, offscreen, config, large_font, medium_font, small_font, favorite_only);
    } else if (mode == "spotify") {
        Magick::InitializeMagick(*argv);
        writeSpotify(matrix, offscreen, config, large_font, medium_font, small_font);
    } else {
        std::cerr << "Invalid mode selected. Use 'scoreboard' or 'logos'." << std::endl;
        return 1;
    }
    
    delete matrix;

    std::cout << std::endl;
    return 0;
}
