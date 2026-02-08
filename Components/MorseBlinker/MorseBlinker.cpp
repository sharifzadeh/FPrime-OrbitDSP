// =======================================================================
//  MORSE BLINKER IMPLEMENTATION (libgpiod v2)
// =======================================================================

#include "OrbitDSP/Components/MorseBlinker/MorseBlinker.hpp"

#include <Fw/Cmd/CmdString.hpp>
#include <Fw/Logger/LogString.hpp>
#include <Fw/Types/BasicTypes.hpp>

#include <gpiod.h>

#include <cctype>    // std::toupper
#include <cerrno>    // errno
#include <cstring>   // strerror
#include <iostream>  // std::cout, std::cerr
#include <string>    // std::string
#include <unistd.h>  // usleep

namespace {

// Simple RAII wrapper around a single output line using libgpiod v2
class OutputLine {
public:
    OutputLine(const char* chip_path,
               unsigned int line_offset,
               const char* consumer)
    : request(nullptr),
      offset(line_offset),
      valid(false)
    {
        struct gpiod_chip* chip = gpiod_chip_open(chip_path);
        if (!chip) {
            std::cerr << "[MorseBlinker] gpiod_chip_open failed: "
                      << std::strerror(errno) << std::endl;
            return;
        }

        struct gpiod_line_settings* settings = gpiod_line_settings_new();
        if (!settings) {
            std::cerr << "[MorseBlinker] gpiod_line_settings_new failed" << std::endl;
            gpiod_chip_close(chip);
            return;
        }

        // Output, start inactive (LED off)
        gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
        gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);

        struct gpiod_line_config* line_cfg = gpiod_line_config_new();
        if (!line_cfg) {
            std::cerr << "[MorseBlinker] gpiod_line_config_new failed" << std::endl;
            gpiod_line_settings_free(settings);
            gpiod_chip_close(chip);
            return;
        }

        int ret = gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);
        if (ret < 0) {
            std::cerr << "[MorseBlinker] gpiod_line_config_add_line_settings failed: "
                      << std::strerror(errno) << std::endl;
            gpiod_line_config_free(line_cfg);
            gpiod_line_settings_free(settings);
            gpiod_chip_close(chip);
            return;
        }

        struct gpiod_request_config* req_cfg = gpiod_request_config_new();
        if (!req_cfg) {
            std::cerr << "[MorseBlinker] gpiod_request_config_new failed" << std::endl;
            gpiod_line_config_free(line_cfg);
            gpiod_line_settings_free(settings);
            gpiod_chip_close(chip);
            return;
        }

        gpiod_request_config_set_consumer(req_cfg, consumer);

        request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
        if (!request) {
            std::cerr << "[MorseBlinker] gpiod_chip_request_lines failed: "
                      << std::strerror(errno) << std::endl;
        } else {
            valid = true;
        }

        gpiod_request_config_free(req_cfg);
        gpiod_line_config_free(line_cfg);
        gpiod_line_settings_free(settings);
        gpiod_chip_close(chip);
    }

    ~OutputLine() {
        if (request != nullptr) {
            gpiod_line_request_release(request);
            request = nullptr;
        }
    }

    bool isValid() const {
        return valid;
    }

    void set(bool on) {
        if (!valid || request == nullptr) {
            return;
        }
        enum gpiod_line_value value =
            on ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE;

        int ret = gpiod_line_request_set_value(request, offset, value);
        if (ret < 0) {
            std::cerr << "[MorseBlinker] gpiod_line_request_set_value failed: "
                      << std::strerror(errno) << std::endl;
        }
    }

private:
    struct gpiod_line_request* request;
    unsigned int offset;
    bool valid;
};

// Convert ASCII character to Morse code string (., -)
std::string char_to_morse(char c) {
    switch (std::toupper(static_cast<unsigned char>(c))) {
    case 'A': return ".-";
    case 'B': return "-...";
    case 'C': return "-.-.";
    case 'D': return "-..";
    case 'E': return ".";
    case 'F': return "..-.";
    case 'G': return "--.";
    case 'H': return "....";
    case 'I': return "..";
    case 'J': return ".---";
    case 'K': return "-.-";
    case 'L': return ".-..";
    case 'M': return "--";
    case 'N': return "-.";
    case 'O': return "---";
    case 'P': return ".--.";
    case 'Q': return "--.-";
    case 'R': return ".-.";
    case 'S': return "...";
    case 'T': return "-";
    case 'U': return "..-";
    case 'V': return "...-";
    case 'W': return ".--";
    case 'X': return "-..-";
    case 'Y': return "-.--";
    case 'Z': return "--..";
    case '0': return "-----";
    case '1': return ".----";
    case '2': return "..---";
    case '3': return "...--";
    case '4': return "....-";
    case '5': return ".....";
    case '6': return "-....";
    case '7': return "--...";
    case '8': return "---..";
    case '9': return "----.";
    default:  return "";  // unsupported -> skip
    }
}

// Blink a full string as Morse code on GPIO 17
void blink_morse_string(const std::string& text) {
    // Keep GPIO line requested once for the whole process lifetime
    static OutputLine led("/dev/gpiochip0", 17, "MorseBlinker");

    if (!led.isValid()) {
        std::cerr << "[MorseBlinker] Failed to initialize GPIO line 17" << std::endl;
        return;
    }

    const useconds_t DOT          = 200000;        // 0.2 s
    const useconds_t DASH         = 3 * DOT;       // 0.6 s
    const useconds_t INTRA_SYMBOL = DOT;           // between dots/dashes
    const useconds_t INTER_LETTER = 3 * DOT;       // between letters
    const useconds_t INTER_WORD   = 7 * DOT;       // between words

    for (std::size_t i = 0; i < text.size(); ++i) {
        char c = text[i];

        if (c == ' ') {
            usleep(INTER_WORD);
            continue;
        }

        std::string code = char_to_morse(c);
        if (code.empty()) {
            continue; // skip unsupported characters
        }

        for (std::size_t j = 0; j < code.size(); ++j) {
            if (code[j] == '.') {
                led.set(true);
                usleep(DOT);
            } else if (code[j] == '-') {
                led.set(true);
                usleep(DASH);
            }
            led.set(false);

            if (j + 1 < code.size()) {
                usleep(INTRA_SYMBOL);
            }
        }

        usleep(INTER_LETTER);
    }

    led.set(false);
}

// Map OrbitDSP status code -> one-letter Morse message
const char* status_to_letter(U8 status) {
    switch (status) {
        case 0U: return "F"; // fault/stopped
        case 1U: return "T"; // tracking/ok
        case 2U: return "N"; // noise/degraded
        case 3U: return "E"; // error/out-of-envelope
        default: return "?";
    }
}

} // anonymous namespace

namespace Components {

MorseBlinker::MorseBlinker(const char* const compName)
: MorseBlinkerComponentBase(compName)
{}

MorseBlinker::~MorseBlinker() {}

// Command handler: BLINK_STRING
void MorseBlinker::BLINK_STRING_cmdHandler(
    FwOpcodeType opCode,
    U32 cmdSeq,
    const Fw::CmdStringArg& message
) {
    const std::string text(message.toChar());

    std::cout << "[MorseBlinker] BLINK_STRING: \"" << text << "\"" << std::endl;

    // Event
    Fw::LogStringArg eventMsg(text.c_str());
    this->log_ACTIVITY_HI_Blinking(eventMsg);

    // Blink
    blink_morse_string(text);

    // Normal command response
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

// Port handler: status input from OrbitDSP
void MorseBlinker::imuStatusIn_handler(
    FwIndexType portNum,
    U8 status
) {
    (void) portNum;

    const char* msg = status_to_letter(status);

    std::cout << "[MorseBlinker] imuStatusIn: " << static_cast<unsigned>(status)
              << " -> \"" << msg << "\"" << std::endl;

    // Optional: log same Blinking event for visibility
    Fw::LogStringArg eventMsg(msg);
    this->log_ACTIVITY_HI_Blinking(eventMsg);

    // IMPORTANT: do NOT call BLINK_STRING_cmdHandler here (it would try to send cmdResponse)
    blink_morse_string(msg);
}

} // namespace Components