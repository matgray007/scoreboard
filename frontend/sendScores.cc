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
#include <cstdlib>
#include <atomic>
#include <sys/stat.h>

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
    if (config.isMember(firstTeamLong) && config.isMember(secondTeamLong)) {
        // Using the dictionary created in team_config.json
        details.firstTeam = config[firstTeamLong]["shortName"].asString();
        details.secondTeam = config[secondTeamLong]["shortName"].asString();
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
    } else {
        // Only using information gathered from the API
        details.firstTeam =  game["competitors"][firstTeamInd]["abbreviation"].asString();
        details.secondTeam = game["competitors"][secondTeamInd]["abbreviation"].asString();
        details.firstTeamLogoURL = game["competitors"][firstTeamInd]["logo"].asString();
        details.secondTeamLogoURL = game["competitors"][secondTeamInd]["logo"].asString();
        details.firstPrimaryColor = Color(255, 255, 255);
        details.firstSecondaryColor = Color(255, 255, 255);
        details.secondPrimaryColor = Color(255, 255, 255);
        details.secondSecondaryColor = Color(255, 255, 255);
    }
    std::cout << "First team " << details.firstTeam << std::endl;
    details.firstScore = game["competitors"][firstTeamInd]["score"].asString();
    details.secondScore = game["competitors"][secondTeamInd]["score"].asString();

    
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

bool IsImageAllBlack(const Magick::Image& image, double threshold = 0.05) {
    Magick::Image grayscale = image;
    grayscale.type(Magick::GrayscaleType);
    grayscale.modifyImage();

    int width = grayscale.columns();
    int height = grayscale.rows();

    const Magick::PixelPacket* pixels = grayscale.getConstPixels(0, 0, width, height);

    double totalBrightness = 0.0;
    int opaqueCount = 0;

    for (int i = 0; i < width * height; i++) {
        if (pixels[i].opacity == TransparentOpacity) continue;
        totalBrightness += (double)pixels[i].red / MaxRGB;
        opaqueCount++;
    }

    if (opaqueCount == 0) return false; // Skip transparent

    double meanBrightness = totalBrightness / opaqueCount;
    return meanBrightness < threshold;
}

Magick::Image InvertNonTransparentPixels(Magick::Image image) {
    image.modifyImage();
    int width = image.columns();
    int height = image.rows();
    Magick::Pixels pixelCache(image);
    Magick::PixelPacket* pixels = pixelCache.get(0, 0, width, height);
    
    for (int i = 0; i < width * height; i++) {
        // Skip fully transparent pixels
        if (pixels[i].opacity == TransparentOpacity) continue;
        
        pixels[i].red   = MaxRGB - pixels[i].red;
        pixels[i].green = MaxRGB - pixels[i].green;
        pixels[i].blue  = MaxRGB - pixels[i].blue;
    }
    
    pixelCache.sync();
    return image;
}

// Global flag for interrupt handling
volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

// Global flag for SIGHUP handling. It will be used to reload the config without restarting the program
std::atomic<bool> sighup_received{false}; // TODO: Replace with volatile again to see if the issue is fixed
static void SighupHandler(int signo) {
    sighup_received = true;
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


Json::Value readMode() {
    std::string mode_path = "../mode.json";
    std::ifstream file(mode_path, std::ifstream::binary);
    Json::Value mode;
    Json::CharReaderBuilder builder;
    std::string errs;
    Json::parseFromStream(builder, file, &mode, &errs);
    file.close();
    return mode;
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

void noGames(RGBMatrix *matrix, FrameCanvas *offscreen, Json::Value config, rgb_matrix::Font &medium_font) {
    offscreen->Fill(0, 0, 0);
    rgb_matrix::DrawText(offscreen, medium_font,
                           0, medium_font.baseline(),
                           Color(255, 255, 255), NULL, "There are",
                           0);
    rgb_matrix::DrawText(offscreen, medium_font,
                          0, 8 + medium_font.baseline(),
                          Color(255, 255, 255), NULL, "no games",
                          0);
    rgb_matrix::DrawText(offscreen, medium_font,
                           0, 16 + medium_font.baseline(),
                           Color(255, 255, 255), NULL, "to display.",
                           0);
    // Double buffer swap
    offscreen = matrix->SwapOnVSync(offscreen);
    usleep(8 * 1000000);
}

// Cycle through games in currentScores.json and display them in a traditional scoreboard format
void writeScoreboard(RGBMatrix *matrix, FrameCanvas *offscreen, Json::Value config,
                 rgb_matrix::Font &large_font, rgb_matrix::Font &medium_font,
                 rgb_matrix::Font &small_font, bool favorite_only) {
    

    // Main loop to keep the program running
    while (!interrupt_received && !sighup_received) {
        // Read in currentScores.json
        Json::Value current_scores = readScores();

        // Loop through each game in current_scores
        for (const auto& game : current_scores["games"]) {
            if (interrupt_received || sighup_received) {
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
            std::cout << " About to write " << details.firstTeam << std::endl;
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
        if (current_scores["games"].size() == 0) {
            noGames(matrix, offscreen, config, medium_font);
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
    while (!interrupt_received && !sighup_received) {
        // Read in currentScores.json
        Json::Value current_scores = readScores();
        // Loop through each game in current_scores
        for (const auto& game : current_scores["games"]) {
            if (interrupt_received || sighup_received) {
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


            ImageVector firstImageVec, secondImageVec;
            Magick::Image firstImageMagick, secondImageMagick;
            bool usedVec = false;

            auto fileExists = [](const std::string& path) {
                struct stat buffer;
                return (stat(path.c_str(), &buffer) == 0);
            };

            try {
                std::string firstLogo = retrieveLogoPath(details.firstTeam, overallConfig);
                std::string secondLogo = retrieveLogoPath(details.secondTeam, overallConfig);

                if (!fileExists(firstLogo) || !fileExists(secondLogo)) {
                    throw std::runtime_error("Logo file not found");
                }

                firstImageVec = LoadImageAndScaleImage(firstLogo.c_str(), height / 2, height / 2);
                secondImageVec = LoadImageAndScaleImage(secondLogo.c_str(), height / 2, height / 2);
                usedVec = true;
            } catch (std::exception &e) {
                firstImageMagick = load_image_from_url(details.firstTeamLogoURL);
                firstImageMagick.scale(Magick::Geometry(height / 2, height / 2));
                if (IsImageAllBlack(firstImageMagick)) {
                    firstImageMagick = InvertNonTransparentPixels(firstImageMagick);
                }
                secondImageMagick = load_image_from_url(details.secondTeamLogoURL);
                secondImageMagick.scale(Magick::Geometry(height / 2, height / 2));
                if (IsImageAllBlack(secondImageMagick)) {
                    secondImageMagick = InvertNonTransparentPixels(secondImageMagick);
                }
            }

            if (usedVec) {
                if (firstImageVec.size() > 0 && secondImageVec.size() > 0) {
                    CopyImageToCanvas(firstImageVec[0], offscreen, 1, 1);
                    CopyImageToCanvas(secondImageVec[0], offscreen, width - (height / 2) - 1, 1);
                }
            } else {
                CopyImageToCanvas(firstImageMagick, offscreen, 1, 1);
                CopyImageToCanvas(secondImageMagick, offscreen, width - (height / 2) - 1, 1);
            }
            offscreen = matrix->SwapOnVSync(offscreen);
            usleep(8 * 1000000); // Display for 5 seconds 5000000
        }
        if (current_scores["games"].size() == 0) {
            noGames(matrix, offscreen, config, medium_font);
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
    while (!interrupt_received && !sighup_received) {
        // Read in currentScores.json
        Json::Value current_scores = readScores();
        // Loop through each game in current_scores
        for (const auto& game : current_scores["games"]) {
            if (interrupt_received || sighup_received) {
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

            // Load in images first to prevent slowdown after sending blank matrix to offscreen
            ImageVector firstImageVec, secondImageVec;
            Magick::Image firstImageMagick, secondImageMagick;
            bool usedVec = false;

            auto fileExists = [](const std::string& path) {
                struct stat buffer;
                return (stat(path.c_str(), &buffer) == 0);
            };

            try {
                std::string firstLogo = retrieveLogoPath(details.firstTeam, overallConfig);
                std::string secondLogo = retrieveLogoPath(details.secondTeam, overallConfig);

                if (!fileExists(firstLogo) || !fileExists(secondLogo)) {
                    throw std::runtime_error("Logo file not found");
                }

                firstImageVec = LoadImageAndScaleImage(firstLogo.c_str(), height, height);
                secondImageVec = LoadImageAndScaleImage(secondLogo.c_str(), height, height);
                usedVec = true;
            } catch (std::exception &e) {
                firstImageMagick = load_image_from_url(details.firstTeamLogoURL);
                firstImageMagick.scale(Magick::Geometry(height, height));
                if (IsImageAllBlack(firstImageMagick)) {
                    firstImageMagick = InvertNonTransparentPixels(firstImageMagick);
                }
                secondImageMagick = load_image_from_url(details.secondTeamLogoURL);
                secondImageMagick.scale(Magick::Geometry(height, height));
                if (IsImageAllBlack(secondImageMagick)) {
                    secondImageMagick = InvertNonTransparentPixels(secondImageMagick);
                }
            }

            offscreen->Fill(0, 0, 0);
            
            // Draw times in center
            writeTimes(offscreen, details, small_font);

            // Write scores on bottom left and bottom right
            writeSmallScores(offscreen, details, medium_font);

            // Draw the field
            // drawField(offscreen, details);

            

            if (usedVec) {
                if (firstImageVec.size() > 0 && secondImageVec.size() > 0) {
                    CopyHalfImage(firstImageVec[0], offscreen, 0, 0, false);
                    CopyHalfImage(secondImageVec[0], offscreen, width - (3 * height / 5), 0, true);
                }
            } else {
                CopyHalfImage(firstImageMagick, offscreen, 0, 0, false);
                CopyHalfImage(secondImageMagick, offscreen, width - (3 * height / 5), 0, true);
            }
            offscreen = matrix->SwapOnVSync(offscreen);
            usleep(8 * 1000000); // Display for 5 seconds 5000000
        }
        if (current_scores["games"].size() == 0) {
            noGames(matrix, offscreen, config, medium_font);
        }
    }
}

// Animation for when a team scores a field goald
void writeFieldGoal(RGBMatrix *matrix, FrameCanvas *offscreen, Json::Value config) {
    offscreen->Fill(0, 0, 0);
    // TODO: write an animation for when a team scores a field goal
    
}

// Spotify mode
void writeSpotify(RGBMatrix *matrix, FrameCanvas *offscreen, Json::Value config,
                 rgb_matrix::Font &large_font, rgb_matrix::Font &medium_font,
                 rgb_matrix::Font &small_font) {
    int scrolling_speed = 3; // Adjust this value to increase/decrease scrolling speed (letters per second)
    int delay_speed_usec = 1000000 / scrolling_speed / medium_font.CharacterWidth('W');
    while (!interrupt_received && !sighup_received) {
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
            

            while (!interrupt_received && !sighup_received && (--x + song_length >= (width / 2) || x + artist_length >= (width / 2))) {
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

                if (interrupt_received || sighup_received) break;
            }
            std::cout << "Inner loop exited (author/song done printing)" << std::endl;
        } else {
            // std::cout << "empty curr scores or missing song" << std::endl;
        }
    }
    std::cout << "Exiting spotify loop" << std::endl;
        
}

void updateClockBackgroundValues(int** values, int** newValues, int width, int height) {
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            newValues[x][y] = values[x][y];
        }
    }

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (values[x][y] == 3) {
                newValues[x][y] = 2;

                if (x < width - 1) {
                    newValues[x + 1][y] = 3;
                }
            } else if (values[x][y] == 2) {
                newValues[x][y] = 1;

                if (x < width - 1) {
                    newValues[x + 1][y] = 2;
                }
            } else if (values[x][y] == 1) {
                newValues[x][y] = 0;
            } else if (x == 0) {
                int randVal = rand() % 1000;
                if (randVal < 10) { // Adjust this value to increase/decrease the density of the "snow"
                    newValues[x][y] = 2;
                }
            }
        }
    }
    // The demo now loops through newValues and updates values with the vals.
    // This function intentionally does not return anything to avoid callers
    // accidentally aliasing the two buffers.
}

void writeClock(RGBMatrix *matrix, FrameCanvas *offscreen, Json::Value config, rgb_matrix::Font &large_font) {

    int width_ = offscreen->width();
    int height_ = offscreen->height();

        // Allocate memory (zero-initialize)
        int** values_ = new int*[width_];
        for (int x = 0; x < width_; ++x) {
            values_[x] = new int[height_]();
        }
        int** newValues_ = new int*[width_];
        for (int x = 0; x < width_; ++x) {
            newValues_[x] = new int[height_]();
        }
    // Main loop
    while (!interrupt_received && !sighup_received) {
        offscreen->Fill(0, 0, 0);
        // Update into the secondary buffer, then swap pointers so we always
        // read from `values_` and write into `newValues_` the next frame.
        updateClockBackgroundValues(values_, newValues_, width_, height_);
        int** tmp = values_;
        values_ = newValues_;
        newValues_ = tmp;
        // TODO: Instead of hardcoding the colors, have it slowly fade from one to another
        for (int x = 0; x < width_; ++x) {
            for (int y = 0; y < height_; ++y) {
                if (values_[x][y] == 3) {
                    offscreen->SetPixel(x, y, 0, 200, 200);
                } else if (values_[x][y] == 2) {
                    offscreen->SetPixel(x, y, 0, 100, 100);
                } else if (values_[x][y] == 1) {
                    offscreen->SetPixel(x, y, 0, 50, 50);
                }
            }
        }
        time_t now = time(0);
        struct tm *localtm = localtime(&now);
        char buffer[6];
        strftime(buffer, sizeof(buffer), "%H:%M", localtm);
        std::string time_str(buffer);
        rgb_matrix::DrawText(offscreen, large_font,
                    width / 3, height / 2 + large_font.baseline(),
                    Color(255, 255, 255), NULL, time_str.c_str(),
                    0); 
        offscreen = matrix->SwapOnVSync(offscreen);
        usleep(100000); // Update every 100 ms. This makes the streaks travel faster/slower (once per iteration)
    }
}


int main(int argc, char *argv[]) {
    std::cout << "Starting matrix" << std::endl;
    // TODO: Set the defaults for rows, cols, led limit refresh, led slowdown gpio, and gpio mapping. 
    // Arg parsing.
    RGBMatrix::Options matrix_options;
    rgb_matrix::RuntimeOptions runtime_opt;
    if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv,
                                            &matrix_options, &runtime_opt)) {
        return usage(argv[0]);
    }

    // Load in mode config
    Json::Value mode_file = readMode();
    // Load in team config
    Json::Value config = readTeamConfig();
    // i.e. config["Arizona Cardinals"]["shortName"]

    // Set up defaults
    std::string mode = mode_file["mode"].asString();
    const char *large_bdf_font_file = "../matrix/fonts/8x13B.bdf";
    const char *medium_bdf_font_file = "../matrix/fonts/5x8.bdf";
    const char *small_bdf_font_file = "../matrix/fonts/4x6.bdf";
    int opt;
    bool favorite_only = false;

    


    while ((opt = getopt(argc, argv, "d:o")) != -1) {// Within this empty string, add any command line options you want to support. i.e. "a:b:cd" options a and b require an associated value, c and do do not
        switch (opt) {
        case 'd':
            mode = optarg; // Leave this in as an option, but it will mostly be used for testing different modes. The default should be pulling the mode from "mode.json"
            break;
        case 'o':
            favorite_only = true;
            break;
        default: /* '?' */
            return usage(argv[0]);
        }
    }

    

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
    signal(SIGHUP, SighupHandler);
    while (!interrupt_received) {
        if (sighup_received) {
            offscreen->Fill(0, 0, 0);
            offscreen = matrix->SwapOnVSync(offscreen);
            std::cout << "We received the sighup" << std::endl;
            sighup_received = false;
            mode_file = readMode();
            mode = mode_file["mode"].asString();
            std::cout << "New mode: " << mode << std::endl;
            usleep(1 * 1000000); // Delay just a bit so hopefully the backend will respond in time
            // TODO: Loading animation instead of blank screen for 1 second
        }
        std::cout << "Starting mode " << mode << std::endl;
        if (mode == "scoreboard") {
            writeScoreboard(matrix, offscreen, config, large_font, medium_font, small_font, favorite_only);
        } else if (mode == "logos") {
            Magick::InitializeMagick(*argv);
            writeLogos(matrix, offscreen, config, large_font, medium_font, small_font, favorite_only);
        } else if (mode == "large-logos") {
            Magick::InitializeMagick(*argv);
            writeLargeLogos(matrix, offscreen, config, large_font, medium_font, small_font, favorite_only);
        } else if (mode == "news") {
            // News implementation  
        } else if (mode == "spotify") {
            Magick::InitializeMagick(*argv);
            writeSpotify(matrix, offscreen, config, large_font, medium_font, small_font);
        } else if (mode == "clock") {
            writeClock(matrix, offscreen, config, large_font);
        } else {
            std::cerr << "Invalid mode selected. Use 'scoreboard' or 'logos'." << std::endl;
            return 1;
        }
        

    }

    
    delete matrix;

    std::cout << std::endl;
    return 0;
}
