#include "ofApp.h"
#include "cpptoml.h"
#include <cstring> // memcpy, ...


void ofApp::setup()
{
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


    // setup the socket
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
    DBGMSG(std::cerr, "Creating graphed variables...");

    int window_width = ofGetWindowWidth();
    int last_y_offset = 0.1 * ofGetWindowHeight();

    auto var_descriptions = config->get_table_array("variable");

    for (const auto& descr : *var_descriptions)
    {
        // *descr is a cpptoml::table
        auto p_varname = descr->get_as<std::string>("name");
        auto p_varid = descr->get_as<uint32_t>("id");

        if (!(p_varname && p_varid)) {
            std::cout << "Each variable description must contain at least "
                      << "a name and identifier number." << std::endl;
            continue;
        }

        auto variable = std::make_shared<GraphedVariable>(
                                                          *p_varid, *p_varname);

        // Position of graphed variable on screen
        variable->x_width_max = 0.8 * window_width;
        variable->ax_origin = ofPoint(0.1 * window_width,
                                      last_y_offset + variable->y_height_max());

        // Store in map<gid, variable>
        variables[*p_varid] = variable;

        last_y_offset += variable->y_height_max();
        DBGMSG(std::cerr, "Listening for var: " << *p_varname);
    }

    DBGMSG(std::cerr, "ofApp setup done!");
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
    size_t sample_size = sizeof(sample_t);
    size_t msg_size = update.size();
    void* data_addr = update.data();
    sample_t* sample;

    DBGMSG(std::cerr, "Received message of size " << msg_size);

    size_t bytes_processed = 0;
    unsigned int i_msg = 0;
    bool debug_var_encountered = false;

    while (bytes_processed < msg_size) {
        // memcpy(&sample, data_addr + (i_msg * sample_size), sample_size);
        sample = (sample_t*) data_addr + (i_msg * sample_size);
        i_msg++;
        bytes_processed += sample_size;

        // Look up the variable in map by identifier and append sample
        auto gid = (unsigned int) sample->gid;
        if (this->variables.count(gid) > 0) {
            this->variables[gid]->samples->push_back(ofPoint(sample->t, sample->v));
        }

        // One debug statement per message received
        if (sample->gid == 1.0 && !debug_var_encountered) {
            DBGMSG(std::cerr, "Received (gid, t, v): "
                   << sample->gid << ", "
                   << sample->t << ", "
                   << sample->v << std::endl);
            debug_var_encountered = true;
        }
    }

    // Forget all samples that are outside of plotting range.
    // I.e. samples where (t_newest - t) * x_per_t > x_width
    for (auto const& id_and_var: variables)
    {
        auto variable = id_and_var.second;
        float t_newest = variable->samples->back().x;
        float t_oldest = variable->samples->front().x;
        while (true) {
            if ((t_newest - t_oldest) * variable->x_per_t > variable->x_width_max) {
                variable->samples->pop_front(); // forget point
                t_oldest = variable->samples->front().x; // get new oldest point
                variable->t_lim_lower = t_oldest;
            } else {
                break;
            }
        }

        // Calculate arrival rate of samples
        uint64_t t_elapsed = ofGetElapsedTimeMillis();
        float dt_var_update = (float) t_elapsed - variable->tsys_last_update;
        variable->update_speed = (t_newest - variable->tmax_last_update) / dt_var_update;
        variable->tmax_last_update = t_newest;
        variable->tsys_last_update = t_elapsed;
    }
}


//==============================================================================
// Plotting Signals

/**
 * Draw to the screen (called after update()).
 */
void ofApp::draw()
{
    //TODO: match scroll speed to rate that new samples arrive
    // - with current t_lim_lower update by pop() => jittery scrolling
    // - instead: set constant scroll speed and match it to rate of arriving samples

    // Draw all our graphed lines
    for (auto const& id_and_var: variables)
    {
        auto var = id_and_var.second;

        // Scroll speed must track update speed (arrival rate) but as low-pass filter
        float draw_time = ofGetLastFrameTime() * 1e-3; // [ms] time elapsed since last frame drawn
        float d_scroll_speed = (var->update_speed - var->scroll_speed) / var->tau_scroll; // d(speed)/d(system time)
        var->scroll_speed += (d_scroll_speed * draw_time);

        // Update lower cutoff time based on scroll speed
        // [sys_time] * [sim_time / sys_time] * [pixels / sim_time]
        var->t_lim_lower += draw_time * var->scroll_speed * var->x_per_t;

        // Draw all the line segments of remaining points
        const ofPoint& last_pt = var->samples->front();
        ofPoint last_xy;
        sample_to_screen(*var, last_pt, last_xy);

        auto it = ++var->samples->begin(); // second point
        while (it != var->samples->end())
        {
            // Map sample point to screen position based on axes origin
            auto sample = *it;
            ofPoint next_xy;
            sample_to_screen(*var, sample, next_xy);
            ofDrawLine(last_xy, next_xy);
            last_xy = next_xy;

            ++it;
        }
    }
}

/**
 * Transform sample point (t, v) to screen coordinates based on
 * drawing-related properties of GraphedVariable.
 *
 * @param   var
 *          GraphedVariable to which the sample point belongs
 *
 * @param   point
 *          ofPoint that will contain transformed coordinates
 */
void ofApp::sample_to_screen(const GraphedVariable &var,
                             const ofPoint &sample,
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
    std::string addr = addr_buffer.str();
    std::cout << "\n[NeuroControl] Connecting to address " << addr << "\n";
    subscriber.connect(addr);

    // SUB socket filters out all messages initially -> need to add filters
    // However, an empty filter value with length argument zero subscribes to all messages
    subscriber.setsockopt(ZMQ_SUBSCRIBE, NULL, 0);

    DBGMSG(std::cerr, "Listening for samples on: " << addr << std::endl);
    return 0;
}


// zmq::socket_t& ofApp::_get_socket()
// {
//     return *(this->_p_socket);
// }

//==============================================================================
// MIDI Interface

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
