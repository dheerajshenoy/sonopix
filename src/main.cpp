#include "MainWindow.hpp"
#include "sonify.hpp"
#include "thirdparty/argparse.hpp"

#include <SFML/Graphics.hpp>

static void
init_args(argparse::ArgumentParser &parser)
{
    parser.add_description("Convert images to audio by traversing them in "
                           "various ways and sonifying the pixel values.");

    parser.add_argument("-v", "--version")
        .help("Show the application version.")
        .default_value(false)
        .implicit_value(true)
        .flag();

    parser.add_argument("-h", "--help")
        .help("Show this help message and exit.")
        .default_value(false)
        .implicit_value(true)
        .flag();

    parser.add_argument("--verbose")
        .help("Enable verbose output for debugging.")
        .default_value(false)
        .implicit_value(true)
        .flag();

    parser.add_argument("-o", "--output")
        .help("Output file to write audio to (by default, saved as `wav' if no "
              "extension is specified).")
        .default_value(std::string(""))
        .nargs(1)
        .metavar("FILE");

    parser.add_argument("-r", "--sample-rate")
        .help("Sample rate for audio output.")
        .nargs(1)
        .scan<'f', float>()
        .metavar("RATE");

    parser.add_argument("-c", "--channels")
        .help("Number of audio channels.")
        .nargs(1)
        .scan<'i', int>()
        .metavar("CHANNELS");

    parser.add_argument("-u", "--secs-per-unit")
        .help("Seconds of audio to generate per unit of image traversal.")
        .nargs(1)
        .scan<'f', float>()
        .metavar("SPU");

    parser.add_argument("-s", "--freq-scale")
        .help("Frequency scale (linear/log/exponential).")
        .default_value(std::string("linear"))
        .nargs(1)
        .choices("linear", "log", "exponential")
        .metavar("SCALE");

    parser.add_argument("-f", "--frequency")
        .help("Frequency in Hz (FREQ or MIN:MAX).")
        .metavar("FREQ[:MAX]")
        .action([](const std::string &s)
    {
        float a, b;
        if (s.find(':') != std::string::npos)
        {
            auto pos = s.find(":");
            a        = std::stof(s.substr(0, pos));
            b        = std::stof(s.substr(pos + 1));
        }
        else
        {
            a = b = std::stof(s);
        }

        return std::make_pair(a, b);
    });

    parser.add_argument("--cursor-width")
        .help("Width of the cursor in pixels.")
        .nargs(1)
        .default_value(5.0f)
        .scan<'f', float>()
        .metavar("WIDTH");

    // Add input file argument
    parser.add_argument("-i", "--input")
        .help("Input file to process.")
        .default_value(std::string(""))
        .nargs(1)
        .metavar("FILE");

    parser.add_argument("-d", "--direction")
        .help("Direction to traverse the image (left-to-right, right-to-left, "
              "top-to-bottom, bottom-to-top, circle-outwards, circle-inwards).")
        .default_value(std::string("left-to-right"))
        .nargs(1)
        .choices("left-to-right", "right-to-left", "top-to-bottom",
                 "bottom-to-top", "circle-outwards", "circle-inwards")
        .metavar("DIRECTION");
}

int
main(int argc, char *argv[])
{
    argparse::ArgumentParser parser(APP_NAME, APP_VERSION);
    init_args(parser);

    try
    {
        parser.parse_args(argc, argv);
    }
    catch (const std::runtime_error &err)
    {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    MainWindow mw;
    mw.read_args(parser);
    mw.main_loop();

    return 0;
}
