#include "ofMain.h"
#include "ofApp.h"
#include "cxxopts.hpp"

//========================================================================
int main(int argc, char* argv[]){
	ofSetupOpenGL(1024,768,OF_WINDOW);			// <-------- setup the GL context


	cxxopts::Options options(argv[0], " - example command line options");
    
	options
		.positional_help("[optional args]")
		.show_positional_help();

	options
		.allow_unrecognised_options()
		.add_options()
		("c, config", "Config", cxxopts::value<std::string>())
		("m, morphology", "Morphology", cxxopts::value<std::string>())
		("help", "Print help");

	auto result = options.parse(argc, argv);
	
	std::string config_file;
	if (result.count("config"))
    {
		const string& config = result["config"].as<std::string>();
		std::cout << "Got config = " << config << std::endl;
		config_file = std::string(config);
	} else {
		std::cout << "Must specify a configuration file!" << std::endl;
		exit(1);
	}

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(new ofApp(config_file));

}
