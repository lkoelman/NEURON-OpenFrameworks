#include "ofApp.h"
#include "cpptoml.h"


void ofApp::setup()
{
    // TODO: setup my app, see examples folder
    ofSetWindowTitle("Neuron MIDI Control");

    // Parse TOML config file
    auto config = cpptoml::parse_file(config_file);

    // =========================================================================
    // Set up socket connection to NEURON simulator

    // Get connection options for config
    auto opt_protocol = config->get_qualified_as<std::string>("connection.protocol");
    auto opt_host = config->get_qualified_as<std::string>("connection.host");
    auto opt_port = config->get_qualified_as<unsigned int>("connection.port");
    string protocol = opt_protocol ? *opt_protocol : "tcp";
    string host = opt_host ? *opt_host : "localhost";
    int port = opt_port ? *opt_port : 8889;


    // TODO: setup the socket
    this->_setup_socket(protocol, host, port);

    // =========================================================================
    // Set up MIDI interface
    // see https://github.com/danomatika/ofxMidi/ -> examples

    // Get MIDI options from config
    auto midi_portnumber = config->get_qualified_as<string>("midi.portnumber");
    auto midi_portname = config->get_qualified_as<unsigned int>("midi.portname");

    bool midi_connected = false;
    if (midi_portnumber) {
        midi_connected = midiIn.openPort(*midi_portnumber);
    } else if (midi_portname) {
        midi_connected = midiIn.openPort(*midi_portname);
        //midiIn.openVirtualPort("ofxMidiIn Input"); // open a virtual port ?
    } else {
        std::cout << LOG_PREFIX << "No MIDI port provided: use mouse & keyboard input";
    }

    if (!midi_connected) {
        std::cout << LOG_PREFIX << "You could use following MIDI ports:\n";
        midiIn.listPorts();
    } else {
        // don't ignore sysex, timing, & active sense messages,
        // these are ignored by default
        midiIn.ignoreTypes(false, false, false);
        
        // add ofApp as a listener
        midiIn.addListener(this);
        
        // print received messages to the console
        midiIn.setVerbose(true);
    }
	
	// =========================================================================
    // Create graphed variables

    auto var_descriptions = config->get_table_array("variable");

    for (const auto& descr : *var_descriptions)
    {
        // *descr is a cpptoml::table
        auto p_varname = descr->get_as<std::string>("name");
        auto p_varid = descr->get_as<uint32_t>("id");

        if (!(p_varname && p_varid)) {
            std::cout << "Each variable description must contain at least "
                      << "a name and identifier number.";
            continue;
        }

        variables[*p_varid] = std::make_shared<GraphedVariable>(
                                        *p_varid, *p_varname);
    }
}


/**
 * Update application state.
 * 
 * DOCUMENTATION
 * -------------
 * 
 * Update and draw are called in an infinite loop one after another in that order,
 * until we finish the application.
 *
 * Update is meant to be used for updating the state of our application, 
 * do any calculations we need to do and update other objects like video players,
 * grabbers, or any computer vision analysis we might be doing.
 * In draw() we only draw to the screen.
 */
void ofApp::update()
{
    // TODO: receive points from socket and add them to graphed_var
    //  - make sure data parsed correctly
    //  - move to separate thread for continuous receive into buffer
    auto& subscriber = *(this->_p_socket);

    //  receive raw data
    zmq::message_t update;
    subscriber.recv(&update);

    // Parse data into workable types
    // See NEURON-sockets/ZmqOutputVars.mod: we got sequence of triples
    // (float, float, float) all concatenated into arbitrary size message.
    double t, v, gid;
    std::istringstream iss(static_cast<char*>(update.data()));

    // Read stream of samples and append to variable sampled if it is being graphed
    // TODO: check that this is not parsing strings to floats, if so find a
    //       float-stream object to use instead
    while (iss) {
        iss >> gid >> t >> v;
        unsigned int iid = (unsigned int) gid;
        // Look up the variable in map by identifier and append sample
        if (this->variables.count(iid) > 0) {
            this->variables[iid]->samples->push_back(ofPoint(t, v));
        }
    }
    std::cout << "Received (gid, t, v): " << gid << ", " << t << ", " << v;
    
    // TODO: Pop values that are outside of the variable's plotting range
}


//==============================================================================
// Plotting Signals

void ofApp::draw()
{
    //TODO: fix this method,

    // Draw all our graphed lines
    for (auto const& id_and_var: variables)
    {
        auto var = id_and_var.second;

        // Draw all the line segments of remaining points
        const ofPoint& last_pt = var->samples->front();
        ofPoint last_xy;
        sample_to_screen(*var, last_pt, last_xy);

        auto it = ++var->samples->begin();
        for (; it != var->samples->end(); ++it)
        {
            // Map sample point to screen position based on axes origin
            auto sample = *it;
            ofPoint next_xy;
            sample_to_screen(*var, sample, next_xy);
            ofDrawLine(last_xy, next_xy);
            last_xy = next_xy;
        }
    }
}

/**
 * Transform sample point (t, v) to screen coordinates.
 * 
 * @param   point
 *          ofPoint that will contain transformed coordinates
 */
void ofApp::sample_to_screen(const GraphedVariable &var, const ofPoint &sample,
                             ofPoint &point)
{
    float x = var.ax_origin.x + (sample.x - var.t_lim_lower) * var.x_per_t;
    float v_to_y = (sample.y - var.v_lim_lower) * var.y_per_v;
    float y = var.ax_origin.y + v_to_y;
    point.set(x, y);
}

//==============================================================================
// Socket (ZMQ) Interface

/**
 * Set up ZMQ SUB socket (subscriber) and subscribe to variable updates.
 */
int ofApp::_setup_socket(string protocol, string host, unsigned int port) {
    // zmq::context_t context(1);
    this->_p_context = std::make_unique<zmq::context_t>(1);
    zmq::context_t& context = *(this->_p_context);

    //  Socket to talk to server
    this->_p_socket = std::make_unique<zmq::socket_t>(context, ZMQ_SUB);
    zmq::socket_t& subscriber = *(this->_p_socket);

    std::ostringstream addr_buffer;
    addr_buffer << protocol << "://" << host << ":" << port;
    string addr = addr_buffer.str();
    std::cout << "\n[NeuroControl] Connecting to address " << addr << "\n";
    subscriber.connect(addr);

    // SUB socket filters out all messages initially -> need to add filters
    // However, an empty filter value with length argument zero subscribes to all messages
    subscriber.setsockopt(ZMQ_SUBSCRIBE, NULL, 0);

    
    return 0;
}


// zmq::socket_t& ofApp::_get_socket()
// {
//     return *(this->_p_socket);
// }

//==============================================================================
// MIDI Inteface

/**
 * Handle MIDI messages (required by ofxMidiListener interface).
 * 
 * @effect  Save the midi message in member variable 'midiMessage'
 *          for later use.
 */
void ofApp::newMidiMessage(ofxMidiMessage& msg) {

	// make a copy of the latest message
	midiMessage = msg;

    // TODO: see all attributes of a MIDI message in
    // https://github.com/danomatika/ofxMidi/blob/master/midiInputExample/src/ofApp.cpp
    // and decide which attributes to use for storing control info
    // based on which are easy to manipulate using MIDI controller.
}

//==============================================================================
// Keyboard & Mouse

void ofApp::keyPressed(int key)
{
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key)
{
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y)
{
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button)
{
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button)
{
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button)
{
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y)
{
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y)
{
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h)
{
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg)
{
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo)
{
}
