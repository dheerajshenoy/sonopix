#include "MainWindow.hpp"
#include "thirdparty/argparse.hpp"

static void
init_args(argparse::ArgumentParser &parser)
{
    parser.add_description("A brief description of your application.");
    parser.add_argument("-v", "--version")
        .help("Show the application version.")
        .default_value(false)
        .implicit_value(true);

    // Add input file argument
    parser.add_argument("-i", "--input")
        .help("Input file to process.")
        .default_value(std::string(""))
        .metavar("FILE");
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
