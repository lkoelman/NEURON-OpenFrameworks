#include "ofApp.h"
#include "cpptoml.h"

// #############################################################################
// CUSTOM METHODS
// #############################################################################


// zmq::socket_t& ofApp::_get_socket()
// {
//     return *(this->_p_socket);
// }


/**
 * Add a new variable that will be graphed.
 */
void ofApp::add_graphed_var(string var_name, ofPoint ax_origin)
{
    // Allocate space for new graphed vars
    GraphedVar var; // TODO write constructor to check all vars
    var.name = string("myvar");
    var.var_id = 0;
    var.ax_origin = ofPoint(0.0);
	var.y_height = 50.0;
	var.y_per_v = 1;
	var.x_per_t = 0.5;
	var.v_lim_upper = 40.0;
	var.v_lim_lower = -80.0;
	var.t_lim_lower = 0.0;
	var.samples = {ofPoint(0.0)};
    this->graphed_vars[0] = var;
}

/**
 * Transform sample point (t, v) to screen coordinates.
 * 
 * @param   point
 *          ofPoint that will contain transformed coordinates
 */
void ofApp::sample_to_screen(const GraphedVar &var, const ofPoint &sample,
                             ofPoint &point)
{
    float x = var.ax_origin.x + (sample.x - var.t_lim_lower) * var.x_per_t;
    float v_to_y = (sample.y - var.v_lim_lower) * var.y_per_v;
    float y = var.ax_origin.y + v_to_y;
    point.set(x, y);
}

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
    std::cout << "Connecting to address " << addr;
    subscriber.connect(addr);

    // SUB socket filters out all messages initially -> need to add filters
    // However, an empty filter value with length argument zero subscribes to all messages
    subscriber.setsockopt(ZMQ_SUBSCRIBE, NULL, 0);

    
    return 0;
}

// #############################################################################
// OF METHODS
// #############################################################################


void ofApp::setup()
{
    // TODO: setup my app, see examples folder
    ofSetWindowTitle("Neural Control Center");

    // TODO: initialize graphed_vars based on config file
    auto config = cpptoml::parse_file(config_file);

    auto opt_protocol = config->get_qualified_as<string>("connection.protocol");
    auto opt_host = config->get_qualified_as<string>("connection.host");
    auto opt_port = config->get_qualified_as<unsigned int>("connection.prt");
    string protocol = opt_protocol ? *opt_protocol : "tcp";
    string host = opt_host ? *opt_host : "localhost";
    int port = opt_port ? *opt_port : 8889;


    // TODO: setup the socket
    this->_setup_socket(protocol, host, port);
}


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
    int gid;
    double t, v;
    std::istringstream iss(static_cast<char*>(update.data()));
    iss >> gid >> t >> v ;
}


void ofApp::draw()
{
    // Draw all our graphed lines
    for (auto const& id_var: graphed_vars)
    {
        const GraphedVar& var = id_var.second;

        // Draw all the line segments of remaining points
        const ofPoint& last_pt = var.samples.front();
        ofPoint last_xy;
        sample_to_screen(var, last_pt, last_xy);

        auto it = ++var.samples.begin();
        for (; it != var.samples.end(); ++it)
        {
            // Map sample point to screen position based on axes origin
            auto sample = *it;
            ofPoint next_xy;
            sample_to_screen(var, sample, next_xy);
            ofDrawLine(last_xy, next_xy);
            last_xy = next_xy;
        }
    }
}

//--------------------------------------------------------------
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
